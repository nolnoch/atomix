/**
 * orbit.hpp
 *
 *    Created on: Oct 18, 2023
 *   Last Update: Oct 18, 2023
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

#ifndef ORBIT_H
#define ORBIT_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "configparser.hpp"

/* Math constants */
const double TWO_PI = 2.0 * M_PI;   // 2pi is used a lot
const double H = 6.626070e-34;      // Planck's constant
const double C = 299792458;         // Speed of massless particles
const double HC = 1.98644586e-25;   // Convenience product of above

using gvec = std::vector<glm::vec3>;
using dvec = std::vector<glm::vec2>;
using ivec = std::vector<uint>;
using vec = glm::vec3;


class Orbit {
    public:
        Orbit(WaveConfig cfg, Orbit *prior);
        virtual ~Orbit();

        void genOrbit();
        void updateOrbit(double time);
        void proximityDetect();
        void superposition();
        
        int vertexCount();
        int vertexSize();
        const float* vertexData();
        int indexCount();
        int indexSize();
        const uint* indexData();

    private:
        int idx = 0;
        WaveConfig config;
        gvec myVertices;
        ivec myIndices;
        dvec myComponents;
        Orbit *priorOrbit = nullptr;

        double amplitude = 0;
        double two_pi_L = 0;
        double two_pi_T = 0;
        double phase_const = 0;

        double deg_fac = 0;
};

/* Orbit config aliases */
#define WAVES   config.orbits        // Number of orbits
#define A       config.amplitude     // Wave-circle amplitude
#define T       config.period        // Wave-circle period
#define L       config.wavelength    // Wave-circle lambda
#define STEPS   config.resolution    // Wave-circle resolution
#define SUPER   config.superposition // Toggle superposition
#define SHADER  config.shader        // Vertex shader

#endif