# Number Reverse Engineering

## Computes ways to obtain numbers with elementary operations

### Thomas Kaldahl, 2021

This C program brute forces the ways in which a user-defined set of elementary
operations can be combined to produce a user-defined number.

For example, given the number 2.03509033057 and the right set of operations, the
program can "reverse engineer" the number and quickly determine that it is a
representation of the square root of 1 plus pi.

### Description

This program combines three types of mathematical objects to produce the desired
number:

* Numbers (0 numbers as input)
* Functions (1 number as input)
* Combining operators (2 numbers as input)

Each such object is assigned a number (displayed as a capital letter from A-Z).
Using (Reversed) Polish Notation, every combination of increasing lengths is
evaluated.

* The expression may be invalid:
    * Does not output exactly 1 number
    * Uses a function without specifying a parameter
    * Performs a forbidden action, like division by 0
        * Actually, I don't check for this in the code. I'm surprised this
          hasn't caused any issues.
* The expression is otherwise valid and evaluates to a single number.

The resulting number is compared to the previous numbers considered. The
expressions resulting in the numbers closest to the desired number are kept and
displayed.

### Configuration

The file `config.h` is edited by the user.

At the top, there are several parameters:

* `TARGET`: The number the user wishes to "reverse engineer"
* `LIMIT`: Number of different expressions to try before halting and showing
  the results
    * A preview display will still allow results to be seen before the `LIMIT`
      is reached.
* `THREADS`: Number of threads to utilize
    * More threads may result in more redundantly equivalent expressions.
* `KEEP`: Number of candidate expressions to display at the end
* `CLEANUP`: (Inverse) frequency of clean-ups
    * Higher values result in less cleaning and more RAM usage.
    * The number of expressions stored in RAM during execution will
      never exceed `THREADS * (KEEP + CLEANUP)`.
    * Excessively frequent clean-ups slow the program down slightly.
* `PROGRESS`: (Inverse) frequency of progress reports
    * Lower values result in each thread displaying its best candidate on the
      screen more frequently during excecution.
    * Excessively low values may slow the program down significantly.

There are a few type-related settings:

* `typedef XXX scalar`: Declares the type of numbers to use in computation
    * Complex numbers are invoked via `typedef double complex scalar`.
* `double magnitude(scalar x)`: Defines the absolute value function for the
  type `scalar`.
    * For complex numbers, replace `fabs` with `cabs`.
    * Used for error computation
* `char *format(scalar x)`: Defines how a number is displayed.
    * For complex numbers, `*format` must be modified to display the real and
      imaginary components.
    * The size of the buffer may be reduced if seen fit.

The operations the program uses are defined by the user.
    * Numbers are specified as functions that each return a scalar.
        * E.g.: `scalar zero() { return 0; }`
    * Functions are defined from a scalar to a scalar.
        * E.g.: `scalar squareRoot(scalar x) { return sqrt(x); }`
    * Combining operations are defined from two scalars to a scalar.
        * E.g.: `scalar add(scalar x, scalar y) { return x + y; }`

The reference of each operation must be listed in the arrays `FUNC0`, `FUNC1`,
and `FUNC2` depending on how many inputs they have.
    * E.g.: `#define FUNC0 {&zero, &one, &pi}`

Furthermore, the lengths of each list must set the values of `COUNT0`,
`COUNT1`, and `COUNT2`.
    * E.g.: `#define COUNT0 3`

### Output

The expressions are outputted using strings of letters. `A` represents the first
element in `FUNC0`, and subsequent letters are used until `FUNC0` is exhausted,
at which point the subsequent letters are assigned to `FUNC1` and then `FUNC2`.

While running, the program displays, for each thread, the expression found that
is closest to the target number, as well as the evaluation of the expression as
well as the distance away from the target number.

Upon completion, the program displays as many as `KEEP` entries, the best
entries compiled from all threads.
