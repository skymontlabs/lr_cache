
class AdaptSizeCache : public LRUCache
{
public: 
    AdaptSizeCache();
    virtual ~AdaptSizeCache()
    {
    }

    virtual void setPar(std::string parName, std::string parValue);
    virtual bool lookup(SimpleRequest*);
    virtual void admit(SimpleRequest*);

private: 
    double _cParam; //
    uint64_t statSize;
    uint64_t _maxIterations;
    uint64_t _reconfiguration_interval;
    uint64_t _nextReconfiguration;
    double _gss_v;  // golden section search book parameters
    // for random number generation 
    std::uniform_real_distribution<double> _uniform_real_distribution = 
        std::uniform_real_distribution<double>(0.0, 1.0); 

    struct ObjInfo {
        double requestCount; // requestRate in adaptsize_stub.h
        uint64_t objSize;

        ObjInfo() : requestCount(0.0), objSize(0) { }
    };
    std::unordered_map<CacheObject, ObjInfo> _longTermMetadata;
    std::unordered_map<CacheObject, ObjInfo> _intervalMetadata;

    void reconfigure();
    double modelHitRate(double c);

    // align data for vectorization
    vector<double> _alignedReqCount;
    vector<double> _alignedObjSize;
    vector<double> _alignedAdmProb;
};


AdaptSizeCache::AdaptSizeCache()
    : LRUCache()
    , _cParam(1 << 15)
    , statSize(0)
    , _maxIterations(15)
    , _reconfiguration_interval(500000)
    , _nextReconfiguration(_reconfiguration_interval)
{
    _gss_v=1.0-gss_r; // golden section search book parameters
}

void AdaptSizeCache::setPar(std::string parName, std::string parValue) {
    if(parName.compare("t") == 0) {
        const uint64_t t = stoull(parValue);
        assert(t>1);
        _reconfiguration_interval = t;
    } else if(parName.compare("i") == 0) {
        const uint64_t i = stoull(parValue);
        assert(i>1);
        _maxIterations = i;
    } else {
        std::cerr << "unrecognized parameter: " << parName << std::endl;
    }
}

bool AdaptSizeCache::lookup(SimpleRequest* req)
{
    reconfigure(); 

    CacheObject tmpCacheObject0(req); 
    if(_intervalMetadata.count(tmpCacheObject0)==0 
       && _longTermMetadata.count(tmpCacheObject0)==0) { 
        // new object 
        statSize += tmpCacheObject0.size;
    }
    // the else block is not necessary as webcachesim treats an object 
    // with size changed as a new object 
    /** 
	} else {
        // keep track of changing object sizes
        if(_intervalMetadata.count(id)>0 
        && _intervalMetadata[id].size != req.size()) {
        // outdated size info in _intervalMetadata
        statSize -= _intervalMetadata[id].size;
        statSize += req.size();
        }
        if(_longTermMetadata.count(id)>0 && _longTermMetadata[id].size != req.size()) {
        // outdated size info in ewma
        statSize -= _longTermMetadata[id].size;
        statSize += req.size();
        }
	}
    */

    // record stats
    auto& info = _intervalMetadata[tmpCacheObject0]; 
    info.requestCount += 1.0;
    info.objSize = tmpCacheObject0.size;

    return LRUCache::lookup(req);
}

void AdaptSizeCache::admit(SimpleRequest* req)
{
    double roll = _uniform_real_distribution(globalGenerator);
    double admitProb = std::exp(-1.0 * double(req->getSize())/_cParam); 

    if(roll < admitProb) 
        LRUCache::admit(req); 
}

void AdaptSizeCache::reconfigure() {
    --_nextReconfiguration;
    if (_nextReconfiguration > 0) {
        return;
    } else if(statSize <= getSize()*3) {
        // not enough data has been gathered
        _nextReconfiguration+=10000;
        return; 
    } else {
        _nextReconfiguration = _reconfiguration_interval;
    }

    // smooth stats for objects 
    for(auto it = _longTermMetadata.begin(); 
        it != _longTermMetadata.end(); 
        it++) {
        it->second.requestCount *= EWMA_DECAY; 
    } 

    // persist intervalinfo in _longTermMetadata 
    for (auto it = _intervalMetadata.begin(); 
         it != _intervalMetadata.end();
         it++) {
        auto ewmaIt = _longTermMetadata.find(it->first); 
        if(ewmaIt != _longTermMetadata.end()) {
            ewmaIt->second.requestCount += (1. - EWMA_DECAY) 
                * it->second.requestCount;
            ewmaIt->second.objSize = it->second.objSize; 
        } else {
            _longTermMetadata.insert(*it);
        }
    }
    _intervalMetadata.clear(); 

    // copy stats into vector for better alignment 
    // and delete small values 
    _alignedReqCount.clear(); 
    _alignedObjSize.clear();
    double totalReqCount = 0.0; 
    uint64_t totalObjSize = 0.0; 
    for(auto it = _longTermMetadata.begin(); 
        it != _longTermMetadata.end(); 
        /*none*/) {
        if(it->second.requestCount < 0.1) {
            // delete from stats 
            statSize -= it->second.objSize; 
            it = _longTermMetadata.erase(it); 
        } else {
            _alignedReqCount.push_back(it->second.requestCount); 
            totalReqCount += it->second.requestCount; 
            _alignedObjSize.push_back(it->second.objSize); 
            totalObjSize += it->second.objSize; 
            ++it;
        }
    }

    std::cerr << "Reconfiguring over " << _longTermMetadata.size() 
              << " objects - log2 total size " << std::log2(totalObjSize) 
              << " log2 statsize " << std::log2(statSize) << std::endl; 

    // assert(totalObjSize==statSize); 
    //
    // if(totalObjSize > cacheSize*2) {
    //
    // model hit rate and choose best admission parameter, c
    // search for best parameter on log2 scale of c, between min=x0 and max=x3
    // x1 and x2 bracket our current estimate of the optimal parameter range
    // |x0 -- x1 -- x2 -- x3|
    double x0 = 0; 
    double x1 = std::log2(getSize());
    double x2 = x1;
    double x3 = x1; 

    double bestHitRate = 0.0; 
    // course_granular grid search 
    for(int i=2; i<x3; i+=4) {
        const double next_log2c = i; // 1.0 * (i+1) / NUM_PARAMETER_POINTS;
        const double hitRate = modelHitRate(next_log2c); 
        // printf("Model param (%f) : ohr (%f)\n",
        // 	next_log2c,hitRate/totalReqRate);

        if(hitRate > bestHitRate) {
            bestHitRate = hitRate;
            x1 = next_log2c;
        }
    }

    double h1 = bestHitRate; 
    double h2;
    //prepare golden section search into larger segment 
    if(x3-x1 > x1-x0) {
        // above x1 is larger segment 
        x2 = x1+_gss_v*(x3-x1); 
        h2 = modelHitRate(x2);
    } else {
        // below x1 is larger segment 
        x2 = x1; 
        h2 = h1; 
        x1 = x0+_gss_v*(x1-x0); 
        h1 = modelHitRate(x1); 
    }
    assert(x1<x2); 

    uint64_t curIterations=0; 
    // use termination condition from [Numerical recipes in C]
    while(curIterations++<_maxIterations 
          && fabs(x3-x0)>tol*(fabs(x1)+fabs(x2))) {
        //NAN check 
        if((h1!=h1) || (h2!=h2)) 
            break; 
        // printf("Model param low (%f) : ohr low (%f) | param high (%f) 
        // 	: ohr high (    %f)\n",x1,h1/totalReqRate,x2,
        // 	h2/totalReqRate);

        if(h2>h1) {
            SHFT3(x0,x1,x2,gss_r*x1+_gss_v*x3); 
            SHFT2(h1,h2,modelHitRate(x2));
        } else {
            SHFT3(x3,x2,x1,gss_r*x2+_gss_v*x0);
            SHFT2(h2,h1,modelHitRate(x1));
        }
    }

    // check result
    if( (h1!=h1) || (h2!=h2) ) {
        // numerical failure
        std::cerr << "ERROR: numerical bug " << h1 << " " << h2 
                  << std::endl;
        // nop
    } else if (h1 > h2) {
        // x1 should is final parameter
        _cParam = pow(2, x1);
        std::cerr << "Choosing c of " << _cParam << " (log2: " << x1 << ")" 
                  << std::endl;
    } else {
        _cParam = pow(2, x2);
        std::cerr << "Choosing c of " << _cParam << " (log2: " << x2 << ")" 
                  << std::endl;
    }
}

double AdaptSizeCache::modelHitRate(double log2c) {
    // this code is adapted from the AdaptSize git repo
    // github.com/dasebe/AdaptSize
    double old_T, the_T, the_C;
    double sum_val = 0.;
    double thparam = log2c;

    for(size_t i=0; i<_alignedReqCount.size(); i++) {
        sum_val += _alignedReqCount[i] * (exp(-_alignedObjSize[i]/ pow(2,thparam))) * _alignedObjSize[i];
    }
    if(sum_val <= 0) {
        return(0);
    }
    the_T = getSize() / sum_val;
    // prepare admission probabilities
    _alignedAdmProb.clear();
    for(size_t i=0; i<_alignedReqCount.size(); i++) {
        _alignedAdmProb.push_back(exp(-_alignedObjSize[i]/ pow(2.0,thparam)));
    }
    // 20 iterations to calculate TTL
  
    for(int j = 0; j<10; j++) {
        the_C = 0;
        if(the_T > 1e70) {
            break;
        }
        for(size_t i=0; i<_alignedReqCount.size(); i++) {
            const double reqTProd = _alignedReqCount[i]*the_T;
            if(reqTProd>150) {
                // cache hit probability = 1, but numerically inaccurate to calculate
                the_C += _alignedObjSize[i];
            } else {
                const double expTerm = exp(reqTProd) - 1;
                const double expAdmProd = _alignedAdmProb[i] * expTerm;
                const double tmp = expAdmProd / (1 + expAdmProd);
                the_C += _alignedObjSize[i] * tmp;
            }
        }
        old_T = the_T;
        the_T = getSize() * old_T/the_C;
    }

    // calculate object hit ratio
    double weighted_hitratio_sum = 0;
    for(size_t i=0; i<_alignedReqCount.size(); i++) {
        const double tmp01= oP1(the_T,_alignedReqCount[i],_alignedAdmProb[i]);
        const double tmp02= oP2(the_T,_alignedReqCount[i],_alignedAdmProb[i]);
        double tmp;
        if(tmp01!=0 && tmp02==0)
            tmp = 0.0;
        else tmp= tmp01/tmp02;
        if(tmp<0.0)
            tmp = 0.0;
        else if (tmp>1.0)
            tmp = 1.0;
        weighted_hitratio_sum += _alignedReqCount[i] * tmp;
    }
    return (weighted_hitratio_sum);
}
