/**
 * configparser.hpp
 *
 *    Created on: Oct 15, 2023
 *   Last Update: Oct 15, 2023
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

#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <QMetaType>
#include "global.hpp"

const std::string WHITESPACE = " \n\r\t\f\v";
const std::string EXT = ".wave";
const std::string CONFIGS = std::string(ROOT_DIR) + "configs/";


/* Wave-circle config struct */
typedef struct {
    double wavelength = 2.0 * M_PI;
    float amplitude = 0.4f;
    float period = 1.0f * M_PI;
    int orbits = 4;
    int resolution = 360;
    bool superposition = false;
    bool gpu = true;
    bool parallel = false;
    bool sphere = false;
    std::string shader = "ortho_wave.vert";
} WaveConfig;
Q_DECLARE_METATYPE(WaveConfig);


class ConfigParser {
    public:
        ConfigParser();
        virtual ~ConfigParser();

        int populateConfig();

        std::vector<std::string> cfgFiles;
        WaveConfig *config;

    private:
        int findConfigFiles();
        int chooseConfigFile();
        int loadConfigFile(std::string filepath);
        void fillConfigFile();

        std::map<std::string, int> cfgValues;
};

#endif