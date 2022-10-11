
#include "CppAD_Curve_Fitting.h"

const std::string ipopt_options =
"Integer print_level  1\n"
"String  sb           yes\n"
// maximum number of iterations
"Integer max_iter     100\n"
// approximate accuracy in first order necessary conditions;
// see Mathematical Programming, Volume 106, Number 1,
// Pages 25-57, Equation (6)
"Numeric tol          1e-12\n"
// derivative testing
"String  derivative_test            second-order\n"
// maximum amount of random pertubation; e.g.,
// when evaluation finite diff
"Numeric point_perturbation_radius 0.\n";

