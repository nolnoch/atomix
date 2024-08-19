/**
 * global.hpp
 *
 *    Created on: Oct 19, 2023
 *   Last Update: Oct 19, 2023
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023 Wade Burch (GPLv3)
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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>

/* Orbit config aliases
#define WAVES   config.orbits        // Number of orbits
#define A       config.amplitude     // Wave-circle amplitude
#define T       config.period        // Wave-circle period
#define L       config.wavelength    // Wave-circle lambda
#define STEPS   config.resolution    // Wave-circle resolution
#define SUPER   config.superposition // Toggle superposition
#define SHADER  config.shader        // Vertex shader
#define GPU     config.gpu           // CPU or GPU calculation
#define PARA    config.parallel      // Parallel or Orthogonal waves
*/

inline constexpr std::string_view ROOT_DIR = "/home/braer/dev/atomix/";
inline constexpr std::string_view SHADERS = "shaders/";
inline constexpr std::string_view CONFIGS = "configs/";

#endif