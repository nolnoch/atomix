/**
 * special.hpp
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

#ifndef SPECIAL_H
#define SPECIAL_H


namespace atomix {
namespace special {

double _a_poly_laguerre_recursion(unsigned int n, double alpha1, double x);
double _a_poly_laguerre_hyperg(unsigned int n, double alpha1, double x);
double _a_poly_laguerre(unsigned int n, double alpha1, double x);
double atomix_laguerre(unsigned int n, unsigned int m, double x);

double _a_poly_legendre_p(unsigned int l, double x);
double _a_assoc_legendre_p(unsigned int l, unsigned int m, double x, double phase = double(+1));
double atomix_legendre(unsigned int l, unsigned int m, double x);

}}

#endif