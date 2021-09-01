#include "nre.h"

#define TARGET 2.03509033057	// The number to be "reverse engineered"

#define LIMIT (long)1e8		// Number of different expressions to try
#define THREADS 4			// Number of threads (may create duplicate results)
#define KEEP 40				// Number of results to record
#define DEFAULT 0			// Value to which an invalid expression evaluates

#define CLEANUP (long)1e7	// Frequency of clean-ups
#define PROGRESS (long)1e6	// Frequency of previews

typedef double scalar;	// This type can be changed (e.g.: double complex)
double magnitude(scalar x) {
	return fabs(x);	// Must be compatible with type scalar (e.g.: cabs(x))
}
char *format(scalar x) {
	char *buff = malloc(40);
	sprintf(buff, "%.12f", x);
	//// For complex numbers, use this instead:
	// sprintf(buff, "%.12f + %.12fi", creal(x), cimag(x));
	return buff;
}

#define COUNT0 4	// Must equal the length of FUNC0
#define COUNT1 1	// Must equal the length of FUNC1
#define COUNT2 5	// Must equal the length of FUNC2

#define FUNC0 {&zero, &one, &pi, &e}	// Must list all (void->scalar) funcs
#define FUNC1 {&sqt}					// Must list all (scalar->scalar) funcs
#define FUNC2 {&add, &sub, &mul, &dvn, &power} // "" (scalar,scalar->scalar) ""

scalar zero() { return 0; }								// A
scalar one() { return 1; }								// B
scalar pi() { return 3.1415926535; }					// C
scalar e() { return 2.7182818284; }						// D

scalar sqt(scalar x) { return sqrt(x); }				// E

scalar add(scalar x, scalar y) { return x + y; }		// F
scalar sub(scalar x, scalar y) { return y - x; }		// G
scalar mul(scalar x, scalar y) { return x * y; }		// H
scalar dvn(scalar x, scalar y) { return y / x; }		// I
scalar power(scalar x, scalar y) { return pow(y, x); }	// J
