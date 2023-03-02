#pragma once

// #include <assert.h>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <stack>
#include <queue>

template <class _Ty>
struct rph_node
{
    T data;
    
    rph_node* left;
    rph_node* next;
    rph_node* parent;

    int rank;

    rph_node(const _Ty& right):
    data(right),
    _Left(nullptr),
    _Next(nullptr),
    _Parent(nullptr),
    _Rank(0)
    {
        _Left = _Next = _Parent = nullptr;
        _Rank = 0;
    }
    rph_node(_Ty&& _Val):
    data(std::move(_Val)),
    _Left(nullptr),
    _Next(nullptr),
    _Parent(nullptr),
    _Rank(0)
    {}

private:

    _Node& operator=(const _Node&);
};

template <class _Myheap>
class _Iterator
{
public:
    friend _Myheap;
    typedef typename _Myheap::_Nodeptr _Nodeptr;
    typedef typename _Myheap::value_type value_type;
    typedef typename _Myheap::difference_type difference_type;
    typedef typename _Myheap::const_reference const_reference;
    typedef typename _Myheap::const_pointer const_pointer;

    _Iterator(_Nodeptr _Ptr = nullptr)
    {
        this->_Ptr = _Ptr;
    }
    const_reference operator*() const
    {
        return this->_Ptr->_Val;
    }
    const_pointer operator->() const
    {
        return &(operator*());
    }
    _Nodeptr _Ptr;
};

template <class _Ty, class _Pr = std::less<_Ty>, class _Alloc = std::allocator<_Ty>>
class rp_heap
{
public:
    typedef rp_heap<_Ty, _Pr, _Alloc> _Myt;
    typedef _Node<_Ty> _Node;
    typedef _Node* _Nodeptr;

    typedef _Pr key_compare;

    typedef _Alloc allocator_type;
    typedef typename _Alloc::template rebind<_Node>::other _Alty;
    typedef typename _Alloc::value_type value_type;
    typedef typename _Alloc::pointer pointer;
    typedef typename _Alloc::const_pointer const_pointer;
    typedef typename _Alloc::reference reference;
    typedef typename _Alloc::const_reference const_reference;
    typedef typename _Alloc::difference_type difference_type;
    typedef typename _Alloc::size_type size_type;

    typedef _Iterator<_Myt> const_iterator;

    rp_heap(const _Pr& _Pred = _Pr()) : comp(_Pred)
    {
        _Mysize = 0;
        _Myhead = nullptr;
    }

    ~rp_heap()
    {
        clear();
    }

    bool empty() const
    {
        return _Mysize == 0;
    }

    size_type size() const
    {
        return _Mysize;
    }

    const_reference top() const
    {
        return _Myhead->_Val;
    }

    const_iterator push(const value_type& _Val)
    {
        _Nodeptr _Ptr = _Alnod.allocate(1);
        _Alnod.construct(_Ptr, _Val);
        _Insert_root(_Ptr);
        _Mysize++;
        return const_iterator(_Ptr);
    }

    const_iterator push(value_type&& x)
    {
        _Nodeptr _Ptr = _Alnod.allocate(1);
        _Alnod.construct(_Ptr, std::forward<value_type>(x));
        _Insert_root(_Ptr);
        _Mysize++;
        return const_iterator(_Ptr);
    }

    void pop() //delete min
    {
        if (empty())
            return;
        
        vector<rph_node*> _Bucket(_Max_bucket_size(), nullptr);

        rph_node* ptr = _Myhead->left;
        while (ptr) {
            rph_node* next = ptr->next;
            ptr->next = nullptr;
            ptr->parent = nullptr;
            _multipass(_bucket, ptr);
            ptr = next;
        }

        ptr = _Myhead->next;
        while (ptr != _Myhead) {
            rph_node* next = ptr->next;
            ptr->next = nullptr;
            _multipass(_bucket, ptr);
            ptr = next;
        }

        _Freenode(_Myhead);
        _Myhead = nullptr;
        /*
        std::for_each(_Bucket.begin(), _Bucket.end(),
        [&](_Nodeptr _Ptr) {
            if (_Ptr) _Insert_root(_Ptr);
        });*/

        for (int i = 0; i < _Max_bucket_size(); ++i) {
            if (_Bucket[i])
                _Insert_root(_Bucket[i]);
        }
    }

    void pop(value_type& _Val)
    {
        if (empty())
            throw std::runtime_error("pop error: empty heap");
        
        _Val = std::move(_Myhead->_Val);
        
        pop();
    }

    void clear()
    {
        if (!empty()) {
            std::queue<_Nodeptr> _Queue;
            _Queue.push(_Myhead);
            while (!_Queue.empty()) {
                _Nodeptr _Ptr = _Queue.front();
                _Queue.pop();
                if (_Ptr->_Left)
                    _Queue.push(_Ptr->_Left);
                if (_Ptr->_Next && _Ptr->_Next != _Myhead)
                    _Queue.push(_Ptr->_Next);
                _Freenode(_Ptr);
            }
        }
        _Myhead = nullptr;
        //assert(empty());
    }

    void decrease(const_iterator _It, const value_type& _Val)
    {
        _Nodeptr _Ptr = _It._Ptr;

        if (comp(_Val, _Ptr->_Val))
            _Ptr->_Val = _Val;
        
        if (_Ptr == _Myhead)
            return;
        
        if (_Ptr->_Parent == nullptr) {
            if (comp(_Ptr->_Val, _Myhead->_Val))
                _Myhead = _Ptr;
        } else {
            _Nodeptr _ParentPtr = _Ptr->_Parent;
            if (_Ptr == _ParentPtr->_Left) {
                _ParentPtr->_Left = _Ptr->_Next;
                if (_ParentPtr->_Left)
                    _ParentPtr->_Left->_Parent = _ParentPtr;
            } else {
                _ParentPtr->_Next = _Ptr->_Next;
                if (_ParentPtr->_Next)
                    _ParentPtr->_Next->_Parent = _ParentPtr;
            }
            // assert_children(_ParentPtr);
            _Ptr->_Next = _Ptr->_Parent = nullptr;
            _Ptr->_Rank = (_Ptr->_Left) ? _Ptr->_Left->_Rank + 1 : 0;
            // assert_half_tree(_Ptr);
            _Insert_root(_Ptr);
            //type-2 rank reduction
            if (_ParentPtr->_Parent == nullptr) { // is a root
                _ParentPtr->_Rank = (_ParentPtr->_Left) ? _ParentPtr->_Left->_Rank + 1 : 0;
            } else {
                while (_ParentPtr->_Parent) {
                    int i = _ParentPtr->_Left ? _ParentPtr->_Left->_Rank : -1;
                    int j = _ParentPtr->_Next ? _ParentPtr->_Next->_Rank : -1;
#ifdef TYPE1_RANK_REDUCTION
                    int k = (i != j) ? std::max(i, j) : i + 1; //type-1 rank reduction
#else
                    int k = (abs(i - j) > 1) ? std::max(i, j) : std::max(i, j) + 1; //type-2 rank reduction
#endif // TYPE1_RANK_REDUCTION
                    if (k >= _ParentPtr->_Rank)
                        break;
                    _ParentPtr->_Rank = k;
                    _ParentPtr = _ParentPtr->_Parent;
                }
            }
        }
    }

private:

    void _Freenode(rph_node* _Ptr)
    {
        _Alnod.destroy(_Ptr);
        _Alnod.deallocate(_Ptr, 1);
        _Mysize--;
    }

    void _Insert_root(rph_node* _Ptr)
    {
        if (_Myhead == nullptr) {
            _Myhead = _Ptr;
            _Ptr->_Next = _Ptr;
        } else {
            _Ptr->_Next = _Myhead->_Next;
            _Myhead->_Next = _Ptr;
            if (comp(_Ptr->_Val, _Myhead->_Val))
                _Myhead = _Ptr;
        }
    }

    rph_node* _Link(rph_node* _Left, rph_node* _Right)
    {
        if (_Right == nullptr)
            return _Left;

        rph_node* _Winner, _Loser;
        if (comp(_Right->_Val, _Left->_Val)) {
            _Winner = _Right;
            _Loser = _Left;
        } else {
            _Winner = _Left;
            _Loser = _Right;
        }

        _Loser->_Parent = _Winner;
        if (_Winner->_Left) {
            _Loser->_Next = _Winner->_Left;
            _Loser->_Next->_Parent = _Loser;
        }
        _Winner->_Left = _Loser;
        _Winner->_Rank = _Loser->_Rank + 1;
        // assert_children(_Winner);
        // assert_parent(_Loser);
        // assert_half_tree(_Winner);
        return _Winner;
    }

    inline size_type _Max_bucket_size() //ceil(log2(size)) + 1
    {
        size_type _Bit = 1, _Count = _Mysize;
        while (_Count >>= 1)
            _Bit++;
        return _Bit + 1;
    }

    template <class _Container>
    void _Multipass(_Container& _Bucket, rph_node* _Ptr)
    {
        // if ((size_t)_Ptr->_Rank >= _Bucket.size())
        // {
        //     _Bucket.resize(_Ptr->_Rank + 1, nullptr);
        // }
        // else
        // {
        while (_Bucket[_Ptr->_Rank] != nullptr)
        {
            uint32_t _Rank = _Ptr->_Rank;
            _Ptr = _Link(_Ptr, _Bucket[_Rank]);
            // assert_children(_Ptr);
            // assert_half_tree(_Ptr);
            _Bucket[_Rank] = nullptr;
            // if ((size_t)_Ptr->_Rank >= _Bucket.size())
            // {
            //     _Bucket.resize(_Ptr->_Rank + 1, nullptr);
            //     break;
            // }
        }
        // }
        _Bucket[_Ptr->_Rank] = _Ptr;
    }

    _Pr comp;
    rph_node* _Myhead;
    size_type _Mysize;
    _Alty _Alnod;

    vector<rph_node> allocator_;
};
