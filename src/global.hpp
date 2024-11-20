/**
 * global.hpp
 *
 *    Created on: Oct 19, 2023
 *   Last Update: Sep 9, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 * 
 *  Copyright 2023, 2024 Wade Burch (GPLv3)
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

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <string>
#include <filesystem>

#define MACOS
#ifdef MACOS
    constexpr bool isMacOS = true;
#else
    constexpr bool isMacOS = false;
#endif

// #define DEBUG

#define SWIDTH 1280
#define SHEIGHT 720
#define SRATIO 0.80

extern uint VK_MINOR_VERSION;
extern uint VK_SPIRV_VERSION;

struct AtomixFiles {
    std::string rootDir;
    std::string shadersDir;
    std::string configsDir;
    std::string resourcesDir;

    bool setRoot(const std::string &_root) {
        if (!std::filesystem::exists(std::string(_root + "/shaders"))) {
            return false;
        }

        rootDir = _root + "/";
        shadersDir = rootDir + "shaders/";
        configsDir = rootDir + "configs/";
        resourcesDir = rootDir + "resources/";

        return true;
    }

    constexpr std::string& root() { return rootDir; }
    constexpr std::string& shaders() { return shadersDir; }
    constexpr std::string& configs() { return configsDir; }
    constexpr std::string& resources() { return resourcesDir; }
};

extern AtomixFiles atomixFiles;

/* Custom BitFlag struct */
struct BitFlag {
    void set(uint flag) {
        bf |= flag;
    }
    void clear(uint flag) {
        bf &= ~flag;
    }
    void toggle(uint flag) {
        bf ^= flag;
    }
    void setC(uint flag, bool condition) {
        if (condition) {
            set(flag);
        }
    }
    void toggleC(uint flag, bool condition) {
        if (condition) {
            toggle(flag);
        }
    }
    void advance(uint flagA, uint flagB) {
        assert(hasAll(flagA) && hasNone(flagB));
        toggle(flagA | flagB);
    }
    bool hasAll(uint flag) {
        return (bf & flag) == flag;
    }
    bool hasAny(uint flag) {
        return (bf & flag) > 0;
    }
    bool hasSomeNotAll(uint flag) {
        return (hasAny(flag)) && (!hasAll(flag));
    }
    bool hasSomeOrNone(uint flag) {
        return (hasSomeNotAll(flag) || hasNone(flag));
    }
    bool hasFirstNotLast(uint flagA, uint flagB) {
        return (hasAll(flagA)) && (hasNone(flagB));
    }
    bool hasNone(uint flag) {
        return (bf & flag) == 0;
    }
    void setTo(uint flag) {
        bf = flag;
    }
    uint intersection(uint flag) {
        return bf & flag;
    }
    void reset() {
        bf = 0;
    }

    uint32_t bf = 0;
};

/* Math constants */
const double TWO_PI = 2.0 * M_PI;   // 2pi is used a lot
const double PI_TWO = M_PI * 2.0;   // pi/2 is used sometimes
const double H = 6.626070e-34;      // Planck's constant
const double C = 299792458;         // Speed of massless particles (m/s)
const double HC = 1.98644586e-25;   // Convenience product of above

const int RENDORBS[] = {1, 2, 4, 8, 16, 32, 64, 128};

enum flagExit {A_OKAY = 0x0, A_ERR = 0x1};
enum bitsColors {ALPHA = 0, BLUE = 8, GREEN = 16, RED = 24};

#endif