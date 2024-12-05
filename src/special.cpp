/**
 * special.cpp
 *
 *    Created on: Dec 4, 2024
 *   Last Update: Dec 4, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2024 Wade Burch (GPLv3)
 * 
 *  This file is part of atomix.
 * 
 *  atomix is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software 
 *  Foundation, either version 3 of the License, or (at your option) any later 
 *  version.
 * 
 *  atomix is distributed in the hope that it will be useful, but WITHOUT ANY 
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License along with 
 *  atomix. If not, see <https://www.gnu.org/licenses/>.
 */

#include <cmath>
#include <limits>
#include <stdexcept>

#include "special.hpp"


double atomix::special::_a_poly_laguerre_recursion(unsigned int n, double alpha1, double x) {
    //   Compute l_0.
    double l_0 = double(1);
    if  (n == 0)
    return l_0;

    //  Compute l_1^alpha.
    double l_1 = -x + double(1) + double(alpha1);
    if  (n == 1)
    return l_1;

    //  Compute l_n^alpha by recursion on n.
    double l_n2 = l_0;
    double l_n1 = l_1;
    double l_n = double(0);
    for  (unsigned int nn = 2; nn <= n; ++nn)
    {
        l_n = (double(2 * nn - 1) + double(alpha1) - x)
                * l_n1 / double(nn)
                - (double(nn - 1) + double(alpha1)) * l_n2 / double(nn);
        l_n2 = l_n1;
        l_n1 = l_n;
    }

    return l_n;
}

double atomix::special::_a_poly_laguerre_hyperg(unsigned int n, double alpha1, double x) {
    const double b = double(alpha1) + double(1);
    const double mx = -x;
    const double tc_sgn = (x < double(0) ? double(1)
                        : ((n % 2 == 1) ? -double(1) : double(1)));
    //  Get |x|^n/n!
    double tc = double(1);
    const double ax = std::abs(x);
    for (unsigned int k = 1; k <= n; ++k)
    tc *= (ax / k);

    double term = tc * tc_sgn;
    double sum = term;
    for (int k = int(n) - 1; k >= 0; --k)
    {
        term *= ((b + double(k)) / double(int(n) - k))
                * double(k + 1) / mx;
        sum += term;
    }

    return sum;
}

double atomix::special::_a_poly_laguerre(unsigned int n, double alpha1, double x) {
    if (x < 0) {
        throw std::domain_error("Negative argument in poly_laguerre.");
    }
    //  Return NaN on NaN input.
    else if (std::isnan(x)) {
        return std::numeric_limits<double>::quiet_NaN();
    } else if (n == 0) {
        return 1;
    } else if (n == 1) {
        return 1 + alpha1 - x;
    } else if (x == 0.0) {
        double prod = double(alpha1) + double(1);
        for (unsigned int k = 2; k <= n; ++k)
            prod *= (double(alpha1) + double(k)) / double(k);
        return prod;
    // } else if (n > 10000000 && double(alpha1) > -double(1) && x < double(2) * (double(alpha1) + double(1)) + double(4 * n)) {
    //     return poly_laguerre_large_n(n, alpha1, x);
    } else if (double(alpha1) >= double(0) || (x > double(0) && double(alpha1) < -double(n + 1))) {
        return _a_poly_laguerre_recursion(n, alpha1, x);
    } else {
        return _a_poly_laguerre_hyperg(n, alpha1, x);
    }
}

double atomix::special::atomix_laguerre(unsigned int n, unsigned int m, double x) {
    return _a_poly_laguerre(n, m, x);
}

double atomix::special::_a_poly_legendre_p(unsigned int l, double x) {
    if (std::isnan(x)) {
        return std::numeric_limits<double>::quiet_NaN();
    } else if (x == +double(1)) {
        return +double(1);
    } else if (x == -double(1)) {
        return (l % 2 == 1 ? -double(1) : +double(1));
    } else {
        double p_lm2 = double(1);
        if (l == 0) {
            return p_lm2;
        }

        double p_lm1 = x;
        if (l == 1) {
            return p_lm1;
        }

        double p_l = 0;
        for (unsigned int ll = 2; ll <= l; ++ll) {
            //  This arrangement is supposed to be better for roundoff
            //  protection, Arfken, 2nd Ed, Eq 12.17a.
            p_l = double(2) * x * p_lm1 - p_lm2
                - (x * p_lm1 - p_lm2) / double(ll);
            p_lm2 = p_lm1;
            p_lm1 = p_l;
        }

        return p_l;
    }
}

double atomix::special::_a_assoc_legendre_p(unsigned int l, unsigned int m, double x, double phase) {
    if (m > l) {
        return double(0);
    } else if (std::isnan(x)) {
        return std::numeric_limits<double>::quiet_NaN();
    } else if (m == 0) {
        return _a_poly_legendre_p(l, x);
    } else {
        double p_mm = double(1);
        if (m > 0) {
            //  Two square roots seem more accurate more of the time
            //  than just one.
            double root = std::sqrt(double(1) - x) * std::sqrt(double(1) + x);
            double fact = double(1);
            for (unsigned int i = 1; i <= m; ++i)
            {
                p_mm *= phase * fact * root;
                fact += double(2);
            }
        }
        if (l == m) {
            return p_mm;
        }

        double p_mp1m = double(2 * m + 1) * x * p_mm;
        if (l == m + 1) {
            return p_mp1m;
        }

        double p_lm2m = p_mm;
        double P_lm1m = p_mp1m;
        double p_lm = double(0);
        for (unsigned int j = m + 2; j <= l; ++j) {
            p_lm = (double(2 * j - 1) * x * P_lm1m
                    - double(j + m - 1) * p_lm2m) / double(j - m);
            p_lm2m = P_lm1m;
            P_lm1m = p_lm;
        }

        return p_lm;
    }
}

double atomix::special::atomix_legendre(unsigned int l, unsigned int m, double x) {
    return _a_assoc_legendre_p(l, m, x);
}
