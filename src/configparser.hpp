/**
 * configparser.hpp
 *
 *    Created on: Oct 15, 2023
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
const std::string CFGEXT = ".wave";
const std::string VSHEXT = ".vert";
const std::string FSHEXT = ".frag";


/* Wave-circle config struct */
typedef struct {
    double wavelength = 2.0 * M_PI;                 // Wavelength as Multiples of PI
    double amplitude = 0.4;                         // Amplitude as Float
    double period = 1.0 * M_PI;                     // Period as Multiples of PI
    double cloudTolerance = 0.01;
    int cloudLayCount = 150;
    int cloudLayDiv = 1;
    int cloudRes = 12;
    int waves = 6;                                  // Wave count as Integer
    int resolution = 180;                           // Resolution as Integer
    bool superposition = false;                     // Superposition as Bool
    bool cpu = false;                               // GPU rendering as Bool
    bool parallel = false;                          // Parallel waves as Bool
    bool sphere = false;                            // Spherical waves as Bool
    std::string vert = "gpu_ortho_circle.vert";     // Vertex shader as String
    std::string frag = "wave.frag";                 // Fragment shader as String
} AtomixConfig;
Q_DECLARE_METATYPE(AtomixConfig);


class ConfigParser {
    public:
        ConfigParser();
        virtual ~ConfigParser();

        int findFiles(std::string loc, std::string type, std::vector<std::string>* fileList);
        int populateConfig();
        int loadConfigFileCLI(std::string filepath);
        int loadConfigFileGUI(std::string filepath);

        void printConfig();

        std::vector<std::string> cfgFiles;
        std::vector<std::string> vshFiles;
        std::vector<std::string> fshFiles;
        AtomixConfig *config;

    private:
        int chooseConfigFile();
        void fillConfigFile();

        std::map<std::string, int> cfgValues;
};

#endif