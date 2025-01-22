/**
 * global.hpp
 *
 *    Created on: Oct 19, 2023
 *   Last Update: Dec 29, 2024
 *  Orig. Author: Wade Burch (dev@nolnoch.com)
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
#include <QMetaType>
#include <QStringList>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <string>


using harmap = std::map<int, std::vector<glm::ivec3>>;

#define SWIDTH 1280
#define SHEIGHT 720
#define SRATIO 0.80

extern int VK_MINOR_VERSION;
extern int VK_SPIRV_VERSION;
extern bool isDebug;
extern bool isMacOS;
extern bool isProfiling;
extern bool isTesting;


struct AtomixWaveConfig {
    // Wave-circle config values
    double wavelength = 2.0;                        // Wavelength as Multiples of PI [double]
    double amplitude = 0.4;                         // Amplitude [double]
    double period = 1.0;                            // Period as Multiples of PI [double]
    int waves = 6;                                  // Wave count [int]
    int resolution = 180;                           // Circle point resolution [int]
    uint visibleOrbits = 0;                         // Visible waves [uint]
    bool superposition = false;                     // Superposition on/off [bool]
    bool cpu = false;                               // GPU rendering on/off [bool]
    bool parallel = false;                          // Parallel waves on/off [bool]
    bool sphere = false;                            // Spherical waves on/off [bool]
    std::string type = "wave";                      // Wave type [string]
};
Q_DECLARE_METATYPE(AtomixWaveConfig);

struct AtomixCloudConfig {
    // Orbital cloud config values
    double cloudTolerance = 0.05;                   // Minimum probability for rendering [double]
    float cloudCull_x = 0.0f;                       // Culling slider -- theta [float]
    float cloudCull_y = 0.0f;                       // Culling slider -- phi [float]
    float cloudCull_rIn = 0.0f;                     // Culling slider -- radius-inward [float]
    float cloudCull_rOut = 0.0f;                    // Culling slider -- radius-outward [float]
    int cloudLayDivisor = 2;                        // Number of layers per radius [int]
    int cloudResolution = 180;                      // Number of points per circle [int]
    bool cpu = false;                               // GPU rendering on/off [bool]
    std::string type = "cloud";                      // Cloud type [string]
};
Q_DECLARE_METATYPE(AtomixCloudConfig);

/** 
 * BitFlag
 * 
 *  @brief A simple bitflag class.
 * 
 *  @details Simplifies the management of bitflags and their oft-forgotten usages.
 * 
*/
struct BitFlag {
    BitFlag() : bf(0) {};
    BitFlag(uint flag) : bf(flag) {};

    /**
     * @brief Set the specified flag.
     *
     * @param[in] flag The flag to set.
     */
    void set(uint flag) {
        bf |= flag;
    }
    /**
     * @brief Clear the specified flag.
     *
     * @param[in] flag The flag to clear.
     */
    void clear(uint flag) {
        bf &= ~flag;
    }
    /**
     * @brief Toggle the specified flag.
     *
     * @param[in] flag The flag to toggle.
     */
    void toggle(uint flag) {
        bf ^= flag;
    }
    /**
     * @brief Set the specified flag if the condition is met.
     *
     * @param[in] flag The flag to set.
     * @param[in] condition The condition to check.
     */
    void condSet(uint flag, bool condition) {
        if (condition) {
            set(flag);
        }
    }
    /**
     * @brief Toggle the specified flag if the condition is met.
     *
     * @param[in] flag The flag to toggle.
     * @param[in] condition The condition to check.
     */
    void condToggle(uint flag, bool condition) {
        if (condition) {
            toggle(flag);
        }
    }
    /**
     * @brief Advance the bitflag state.
     *
     * @param[in] flagA The flags that must be set before the advance.
     * @param[in] flagB The flags that must be clear before the advance.
     *
     * @details If all flags in flagA are set and all flags in flagB are clear,
     *          the function will toggle the flags in flagA and flagB.
     *          Otherwise, it will do nothing.
     */
    void advance(uint flagA, uint flagB) {
        assert(hasAll(flagA) && hasNone(flagB));
        toggle(flagA | flagB);
    }
    /**
     * @brief Check if all flags in the given flag are set.
     *
     * @param[in] flag The flag to check.
     * @return true if all flags in the given flag are set, false otherwise.
     */
    bool hasAll(uint flag) {
        return (bf & flag) == flag;
    }
    /**
     * @brief Check if any flag in the given flag is set.
     *
     * @param[in] flag The flag to check.
     * @return true if any flag in the given flag is set, false otherwise.
     */
    bool hasAny(uint flag) {
        return (bf & flag) > 0;
    }
    /**
     * @brief Check if some (but not none and not all) flags in the given flag are set.
     *
     * @param[in] flag The flag to check.
     * @return true if some (but not none and not all) flags in the given flag are set, false otherwise.
     */
    bool hasSomeNotAll(uint flag) {
        return (hasAny(flag)) && (!hasAll(flag));
    }
    /**
     * @brief Check if some or none (but not all) flags in the given flag are set.
     *
     * @param[in] flagA The flag to check.
     * @return true if some or none (but not all) flags in the given flag are set, false otherwise.
     */
    bool hasSomeOrNone(uint flag) {
        return (hasSomeNotAll(flag) || hasNone(flag));
    }
    /**
     * @brief Check if all flags in flagA are set and all flags in flagB are not set.
     *
     * @param[in] flagA The first flag to check.
     * @param[in] flagB The second flag to check.
     * @return true if all flags in `flagA` are set and all flags in `flagB` are not set, false otherwise.
     */
    bool hasFirstNotLast(uint flagA, uint flagB) {
        return (hasAll(flagA)) && (hasNone(flagB));
    }
    /**
     * @brief Check if all flags in the given flag are not set.
     *
     * @param[in] flag The flag to check.
     * @return true if all flags in `flag` are not set, false otherwise.
     */
    bool hasNone(uint flag) {
        return (bf & flag) == 0;
    }
    /**
     * @brief Set the current BitFlag to the given flag, clearing all other flags.
     *
     * @param[in] flag The flag to set.
     */
    void setTo(uint flag) {
        bf = flag;
    }
    /**
     * @brief Return the intersection of the current BitFlag with the given flag.
     *
     * @param[in] flag The flag to intersect with.
     *
     * @return The bitwise AND of the current BitFlag with the given flag.
     */
    uint intersection(uint flag) {
        return bf & flag;
    }
    /**
     * @brief Reset the BitFlag to 0.
     *
     * @details This sets all flags to 0, effectively clearing the BitFlag.
     */
    void reset() {
        bf = 0;
    }

    uint32_t bf = 0;
};

namespace atomix {
    /**
     * @brief Pretty print a harmap for debugging purposes.
     *
     * @param[in] map The harmap to print.
     *
     * Prints each key-value pair in the harmap, with the key followed by the
     * values of that key, separated by commas.
     */
    inline void printHarmap(harmap &map) {
        for (auto k : map) {
            std::cout << k.first << ": ";
            for (auto v : k.second) {
                std::cout << glm::to_string(v) << ", ";
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    }

    /**
     * @brief Convert a QStringList to a std::vector<std::string>.
     *
     * @param[in] list The QStringList to convert.
     *
     * @return A std::vector<std::string> containing the same elements as the
     * QStringList.
     */
    inline std::vector<std::string> stringlistToVector(QStringList list) {
        std::vector<std::string> vec;
        vec.reserve(list.size());

        for (auto str : list) {
            vec.push_back(str.toStdString());
        }
        return vec;
    }
}

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