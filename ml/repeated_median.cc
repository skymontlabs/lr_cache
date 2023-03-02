#include <vector>

double slope(point_t i, point_t j)
{
	double a = j.x - i.x;
	double b = j.y - i.y;

	return b/a;
}

double intercept(point_t i, point_t j)
{
	double a = j.x - i.x;

	return (i.y*j.x-i.x*j.y)/a;
}

void repeated_median_reg(point_t* points, uint32_t ct)
{
	vector<double> outer_slope;
	vector<double> outer_intercept;

	vector<double> inner_slope;
	vector<double> inner_intercept;

	for (int i=0;i<ct;++i) {
		for (int j=0;j<ct;++j) {
			auto p0 = points[i];
			auto p1 = points[j];

			if (p0.x != p1.x) {
				inner_slope.push_back(slope(p0, p1));
				inner_intercept.push_back(intercept(p0, p1));
			}
		}

		outer_slope.push_back(median(inner_slope));
		outer_intercept.push_back(median(inner_intercept));
		inner_slope.clear();
		inner_intercept.clear();
	}

	float m = median(outer_slope);
	float b = median(outer_intercept);
}
/*
int main(int argc, char const *argv[])
{
		
	return 0;
}*/