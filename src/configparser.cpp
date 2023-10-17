/**
 * configparser.hpp
 *
 *    Created on: Oct 15, 2023
 *   Last Update: Oct 15, 2023
 *  Orig-> Author: Wade Burch (braernoch.dev@gmail.com)
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
    cfgValues["shader"] = 6;

    config = new WaveConfig;
}

ConfigParser::~ConfigParser() {
    cfgFiles.clear();
}

void ConfigParser::fillConfigFile() {
    /* This is broken and currently unnecessary anyway. */
    config->orbits = config->orbits >= 0 ?: 4;
    config->amplitude = config->amplitude > 0 ?: 0.6f;
    config->period = config->period ?: 1.0f;
    config->wavelength = config->wavelength > 0 ?: 2.0f * M_PI;
    config->resolution = config->resolution > 0 ?: 360;
}

int ConfigParser::findConfigFiles() {
    const std::string PATH = filesystem::current_path().string() + "/";
    for (auto &p: filesystem::recursive_directory_iterator(PATH)) {
        if (p.path().extension() == EXT)
            this->cfgFiles.push_back(p.path().string());
    }
    cout << "Found " << cfgFiles.size() << " candidate file(s)." << endl;

    return cfgFiles.size();
}

int ConfigParser::chooseConfigFile() {
    string s;
    int f, files;
    files = cfgFiles.size();
    f = -1;

    cout << "Please choose config file from available options [1-" << files + 1 << "]:\n\n";
    for (int i = 0; i < files; i++) {
        string fpath = cfgFiles[i];
        string fname = fpath.substr(fpath.find_last_of("/") + 1);
        string sname = fname.substr(0, fname.length() - 5);
        cout << "    [" << i + 1 << "] " << sname << "\n";
    }
    cout << "    [" << files + 1 << "] none (use default configuration)\n\n";
    cout << "Selection: ";
    
    if (cin.peek() == '\n') {
        cout << "Using default configuration." << endl;
    } else {
        cin >> s;
        if (isdigit(s[0])) {
            int c = stoi(s);

            if (c == files + 1) {
                cout << "Using default configuration." << endl;
            } else if (c < 0 || c > files) {
                cout << "Invalid selection. Proceeding with default." << endl;
            } else {
                f = c - 1;
            }
        } else {
            cout << "Invalid selection. Proceeding with default." << endl;
        }
    }

    return f;
}

void ConfigParser::loadConfigFile(string path) {
    string line, key, value, name;
    size_t colon;
    int changes;
    map<string, int>::iterator iter;

    name = path.substr(path.find_last_of("/") + 1);
    cout << "Using config file: " << name << endl;

    ifstream file(path);

    while (getline(file, line)) {
        if (!line.length() || line[0] == '#')
            continue;
        
        colon = line.find(":");
        key = line.substr(0, colon);
        iter = cfgValues.find(key);
        if (iter == cfgValues.end())
            continue;
        
        value = line.substr(colon + 2);

        switch(iter->second) {
            case 1:
                config->orbits = stoi(value);
                changes++;
                break;
            case 2:
                config->amplitude = stof(value);
                changes++;
                break;
            case 3:
                config->period = stof(value);
                changes++;
                break;
            case 4:
                config->wavelength = stod(value) * M_PI;
                changes++;
                break;
            case 5:
                config->resolution = stoi(value);
                changes++;
                break;
            case 6:
                config->shader = value;
                changes++;
                break;
            default:
                continue;
        }
    }
    if (changes < 6)
        cout << "Some configuration values not found; defaults were used instead." << endl;

    file.close();
}

void ConfigParser::populateConfig() {
    int cand = findConfigFiles();

    if (!cand)
        cout << "Using default configuration." << endl;
    else {
        int choice = chooseConfigFile();
        if (choice >= 0)
            loadConfigFile(cfgFiles[choice]);
    }
}
