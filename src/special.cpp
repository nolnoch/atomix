/**
 * special.cpp
 *
 *    Created on: Dec 4, 2024
 *   Last Update: Dec 29, 2024
 *  Orig. Author: Wade Burch (dev@nolnoch.com)
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
#include <cassert>

#include "special.hpp"


double atomix::special::_a_poly_laguerre(unsigned int n, double m, double x) {
    assert(x > 0);
    if (n == 0) {
        return 1.0;
    } else if (n == 1) {
        return 1.0 + m - x;
    } else if (x == 0.0) {
        double prod = m + 1.0;
        for (unsigned int k = 2; k <= n; ++k)
            prod *= (m + double(k)) / double(k);
        return prod;
    } else if ((m >= 0.0) || ((x > 0.0) && (m < -double(n + 1)))) {
        double l_0 = 1.0;
        if  (n == 0)
        return l_0;

        //  Compute l_1^alpha.
        double l_1 = -x + 1.0 + m;
        if  (n == 1)
        return l_1;

        //  Compute l_n^alpha by recursion on n.
        double l_n2 = l_0;
        double l_n1 = l_1;
        double l_n = 0.0;
        for  (unsigned int nn = 2; nn <= n; ++nn)
        {
            l_n = (double(2 * nn - 1) + m - x)
                    * l_n1 / double(nn)
                    - (double(nn - 1) + m) * l_n2 / double(nn);
            l_n2 = l_n1;
            l_n1 = l_n;
        }

        return l_n;
    } else {
        const double b = m + 1.0;
        const double mx = -x;
        const double tc_sgn = ((x < 0.0) ? 1.0 : ((n % 2 == 1) ? -1.0 : 1.0));
        //  Get |x|^n/n!
        double tc = 1.0;
        const double ax = std::abs(x);
        for (unsigned int k = 1; k <= n; ++k) {
            tc *= (ax / k);
        }

        double term = tc * tc_sgn;
        double sum = term;
        for (int k = (int(n) - 1); k >= 0; --k) {
            term *= ((b + double(k)) / double(int(n) - k))
                    * double(k + 1) / mx;
            sum += term;
        }

        return sum;
    }
}

double atomix::special::_a_assoc_legendre_p(unsigned int l, unsigned int m, double x) {
    assert(l >= m);
    double phase = 1.0;
    if (m == 0) {
        if (x == +1.0) {
            return +1.0;
        } else if (x == -1.0) {
            return (((l % 2) == 1) ? -1.0 : +1.0);
        } else {
            double p_lm2 = 1.0;
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
    } else {
        double p_mm = 1.0;
        if (m > 0) {
            //  Two square roots seem more accurate more of the time than just one.
            double root = std::sqrt(1.0 - x) * std::sqrt(1.0 + x);
            double fact = 1.0;
            for (unsigned int i = 1; i <= m; ++i)
            {
                p_mm *= phase * fact * root;
                fact += 2.0;
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
        double p_lm = 0.0;
        for (unsigned int j = m + 2; j <= l; ++j) {
            p_lm = (double(2 * j - 1) * x * P_lm1m
                    - double(j + m - 1) * p_lm2m) / double(j - m);
            p_lm2m = P_lm1m;
            P_lm1m = p_lm;
        }

        return p_lm;
    }
}


