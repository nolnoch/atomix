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


#include "configparser.hpp"


using namespace std;


ConfigParser::ConfigParser() {
    /* Load key,value pairs for string lookup */
    cfgValues["orbits"] = 1;
    cfgValues["amplitude"] = 2;
    cfgValues["period"] = 3;
    cfgValues["wavelength"] = 4;
    cfgValues["resolution"] = 5;
}

ConfigParser::~ConfigParser() {
    cfgFiles.clear();
}

void ConfigParser::fillConfigFile() {
    /* Populate missing or invalid values with defaults in WaveConfig struct */
    config.orbits = config.orbits >= 0 ?: 4;
    config.amplitude = config.amplitude > 0 ?: 0.6f;
    config.period = config.period ?: 1.0f;
    config.wavelength = config.wavelength > 0 ?: 2.0f * M_PI;
    config.resolution = config.resolution > 0 ?: 360;
}

int ConfigParser::findConfigFiles() {
    for (auto &p: filesystem::recursive_directory_iterator(path)) {
        if (p.path().extension() == ext)
            this->cfgFiles.push_back(p.path());
    }
    cout << "Found " << cfgFiles.size() << " candidate file(s)." << endl;

    return cfgFiles.size();
}

void ConfigParser::loadConfigFile(string path) {
    string line, key, value;
    size_t colon;
    map<string, int>::iterator iter;

    ifstream file(path);

    while (getline(file, line)) {
        if (!line.length() || line[0] == '#')
            continue;
        
        colon = line.find(":");
        key = line.substr(0, colon);
        iter = cfgValues.find(key);
        if (iter == cfgValues.end())
            continue;
        
        value = line.substr(colon + 1);

        switch(iter->second) {
            case 1:
                config.orbits = stoi(value);
                break;
            case 2:
                config.amplitude = stof(value);
                break;
            case 3:
                config.period = stof(value);
                break;
            case 4:
                config.wavelength = stoi(value);
                break;
            case 5:
                config.resolution = stoi(value);
                break;
            default:
                continue;
        }
    }

    file.close();
}