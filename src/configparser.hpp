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
#include <string>
#include <vector>
#include <map>
#include <QMetaType>

const std::string EXT = ".wave";


/* Wave-circle config struct */
typedef struct {
    double wavelength = 2.0 * M_PI;
    float amplitude = 0.6f;
    float period = 1.0f * M_PI;
    int orbits = 4;
    int resolution = 360;
    bool superposition = false;
    std::string shader = "ortho_wave.vert";
} WaveConfig;
Q_DECLARE_METATYPE(WaveConfig);


class ConfigParser {
    public:
        ConfigParser();
        virtual ~ConfigParser();

        void populateConfig();

        std::vector<std::string> cfgFiles;
        WaveConfig *config;

    private:
        int findConfigFiles();
        int chooseConfigFile();
        void loadConfigFile(std::string filepath);
        void fillConfigFile();

        std::map<std::string, int> cfgValues;
};

#endif