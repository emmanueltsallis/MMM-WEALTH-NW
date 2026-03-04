//**********C++ SUPPORT FUNCTIONS*********//

#include <algorithm>  // For std::sort (optimized inequality calculations)

/*
ROUND(value, "direction")
This MACRO returns the rouded value of a value specified by the user. 
The user can specify the direction wih the word UP and "DOWN", in quotes and capital letters, in which case the MACRO will round up or down, respectively.
If any other word or no word is specified, the MACRO will simply round the value.
*/
double ROUND( double x , string d = "none")
{
	double r = round(x);
	double y;
	if(d=="UP")
		y = r>x? r: r+1;
	else if(d=="DOWN")
		y = r<x? r: r-1;
	else
		y = r;
	return y;
}


/*
LAG_SUM(obj, "lab", lag1, lag2)
This MACRO returns the sum of lagged values of a specifed variable named "lab". 
The first lag defines how many lags to sum. The secong lag defines from which lag it will start summing. By default, the second lag is 1.
WARNING: make sure there are specified lagged values for the variable "lab".
EXAMPLE 1: LAG_SUM(p, "X", 4) will return VL("X",0) + VL("X",1) + VL("X",2) + VL("X",3).
EXAMPLE 2: LAG_SUM(p, "X", 3, 2) will return VL("X",2) + VL("X",3) + VL("X",4).
*/
double LAG_SUM( object *obj , const char *var , int lag = 0, int lag2 = 0)
{
	double x = 0;
	int i;
	for(i=lag2; i<=lag2+lag-1; i++)
		x = x + VLS( obj, var, i);
	return x;
}

/*
LAG_AVE(obj, "lab", lag1, lag2)
Same as LAG_SUM but this MACRO returns the average for the lag1 periods.
WARNING: make sure there are specified lagged values for the variable "lab".
EXAMPLE 1: LAG_AVE(p, "X", 4) will return (VL("X",0) + VL("X",1) + VL("X",2) + VL("X",3))/4.
EXAMPLE 2: LAG_AVE(p, "X", 3, 2) will return (VL("X",2) + VL("X",3) + VL("X",4))/3.
*/
double LAG_AVE( object *obj , const char *var , int lag = 0, int lag2 = 0)
{
	double x = 0;
	int i;
	for(i=lag2; i<=lag2+lag-1; i++)
		x = x + VLS( obj, var, i);
	return x/lag;
}

/*
LAG_GROWTH(obj, "lab", lag1, lag2)
This MACRO returns the growth rate of a variable named "lab" for the lag1 periods.
The first lag defines how many lags to sum. The secong lag defines from which lag it will start summing. By default, the second lag is 0.
WARNING: make sure there are specified lagged values for the variable "lab".
EXAMPLE 1: LAG_GROWTH(p, "X", 4) will return (V("X") - VL("X",4))/VL("X",4).
EXAMPLE 2: LAG_GROWTH(p, "X", 3, 2) will return (VL("X",2) - VL("X",5))/VL("X",5).
*/
double LAG_GROWTH( object *obj , const char *var , int lag = 0, int lag2 = 0)
{
	double x = VLS( obj, var, lag2);
	double y = VLS( obj, var, lag2+lag);
	double r = y!=0? (x-y)/y : 0;
	return r;
}


/*
QEXPONENTIAL(q, lambda)
This function generates a random draw from a q-exponential distribution (Tsallis statistics).
Used for generating heavy-tailed wealth/profit distributions.

Parameters:
- q: Entropic index controlling tail heaviness
  - q < 1: Compact support (bounded distribution)
  - q = 1: Standard exponential (falls back to exponential)
  - q > 1: Heavy tail (Pareto-like, used for wealth)
- lambda: Scale parameter (default 1.0)

Returns: A positive random draw from q-exponential distribution

Example: qexponential(1.5, 1.0) generates Pareto-like draws for wealth distribution
         Typical Gini: q=1.3 → ~0.5, q=1.5 → ~0.7, q=1.7 → ~0.85

Reference: Tsallis (1988) nonextensive statistical mechanics
*/
double qexponential(double q, double lambda = 1.0)
{
	double u = RND;  // Uniform [0,1)

	// Handle q ≈ 1 case (standard exponential)
	if(fabs(q - 1.0) < 1e-10)
		return -lambda * log(1.0 - u);

	// Q-exponential inverse CDF
	// F^{-1}(u) = (λ/(1-q)) × [(1-u)^{1-q} - 1]
	double exponent = 1.0 - q;
	double result = (lambda / exponent) * (pow(1.0 - u, exponent) - 1.0);

	// For q > 1, result should be positive
	return fabs(result);
}


/*
GINI_COEFFICIENT(values, n)
Computes the Gini coefficient for an array of values.
Used for validating wealth distribution inequality.

Parameters:
- values: Array of wealth/income values
- n: Number of elements

Returns: Gini coefficient in [0, 1], where 0 = perfect equality, 1 = maximum inequality

Note: This is a helper for diagnostics, not used in simulation equations.
*/
double gini_coefficient(double* values, int n)
{
	if(n <= 1) return 0.0;

	// Sort values (simple bubble sort - OK for small n)
	double* sorted = new double[n];
	for(int i = 0; i < n; i++) sorted[i] = values[i];
	for(int i = 0; i < n-1; i++) {
		for(int j = 0; j < n-i-1; j++) {
			if(sorted[j] > sorted[j+1]) {
				double temp = sorted[j];
				sorted[j] = sorted[j+1];
				sorted[j+1] = temp;
			}
		}
	}

	// Gini formula: G = (2Σ(i×x_i) - (n+1)Σx_i) / (n×Σx_i)
	double sum_ix = 0, sum_x = 0;
	for(int i = 0; i < n; i++) {
		sum_ix += (i + 1) * sorted[i];
		sum_x += sorted[i];
	}

	delete[] sorted;

	if(sum_x < 1e-10) return 0.0;
	return (2.0 * sum_ix - (n + 1) * sum_x) / (n * sum_x);
}


/*
HOUSEHOLD_PERCENTILE(parent, var, percentile, lag)
Computes the percentile of a household variable across ALL households in both CLASSES.

This function solves the LSD sibling-only limitation of PERCLS when households
are distributed across multiple parent objects (working_class, capitalist_class).

Parameters:
- parent: Parent object containing CLASSES (typically 'country' pointer)
- var: Variable name (e.g., "Household_Avg_Real_Income")
- percentile: Target percentile in [0,1] (e.g., 0.5 for median)
- lag: Lag value (0 = current, 1 = previous period)

Returns: The value at the specified percentile of the distribution

Example: household_percentile(country, "Household_Avg_Real_Income", 0.5, 1)
         returns the median income from the previous period
*/
double household_percentile(object* parent, const char* var, double percentile, int lag)
{
	// Count total households across both classes
	int n = 0;
	object *cur, *cur1;
	CYCLES(parent, cur1, "CLASSES")
	{
		CYCLES(cur1, cur, "HOUSEHOLD")
			n++;
	}

	if(n == 0) return 0.0;

	// Collect values from all households
	double* values = new double[n];
	int idx = 0;
	CYCLES(parent, cur1, "CLASSES")
	{
		CYCLES(cur1, cur, "HOUSEHOLD")
		{
			values[idx++] = VLS(cur, var, lag);
		}
	}

	// Sort values using std::sort
	std::sort(values, values + n);

	// Compute percentile index (linear interpolation)
	double pos = percentile * (n - 1);
	int lower = (int)floor(pos);
	int upper = (int)ceil(pos);
	double frac = pos - lower;

	double result;
	if(lower == upper || upper >= n)
		result = values[lower];
	else
		result = values[lower] * (1.0 - frac) + values[upper] * frac;

	delete[] values;
	return result;
}

