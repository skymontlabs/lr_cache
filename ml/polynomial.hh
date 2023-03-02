int polynomial(float* x, float* y, uint32_t length)
{
	int num = variables * (variables-1) >> 1;
	
	for (int l = 0; l < length; ++l) {
		// multiply the variables by each other
		float* xr = x[i * (1 + num + variables)];

		for (int i = 0; i < num; ++i) {
			xr[idx] = 
		}

		// multiply the variables by each other
		for (int i = 0; i < variables; ++i) {
			for (int j = i; j < variables; ++i) {
				xr[idx] = xr[i] * xr[j];
			}
		}
	}
}


// 1 column multiply by all others (dot) each

(n)*(n-1)/2