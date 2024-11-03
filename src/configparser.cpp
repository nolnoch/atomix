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


#include "configparser.hpp"

using namespace std;


ConfigParser::ConfigParser() {
    /* Load key,value pairs for string lookup */
    cfgValues["orbits"] = 1;
    cfgValues["amplitude"] = 2;
    cfgValues["period"] = 3;
    cfgValues["wavelength"] = 4;
    cfgValues["resolution"] = 5;
    cfgValues["vertex"] = 6;
    cfgValues["superposition"] = 7;
    cfgValues["orientation"] = 8;
    cfgValues["processor"] = 9;
    cfgValues["sphere"] = 10;
    cfgValues["fragment"] = 11;
    cfgValues["tolerance"] = 12;
    cfgValues["divisor"] = 13;
    cfgValues["cloudRes"] = 14;

    // config = new AtomixConfig;
}

ConfigParser::~ConfigParser() {
    cfgFiles.clear();
    vshFiles.clear();
    fshFiles.clear();
}

void ConfigParser::fillConfigFile() {
    /* This is broken and currently unnecessary anyway. */
    config.waves = config.waves >= 0 ?: 4;
    config.amplitude = config.amplitude > 0 ?: 0.6f;
    config.period = config.period ?: 1.0f;
    config.wavelength = config.wavelength > 0 ?: 2.0f * M_PI;
    config.resolution = config.resolution > 0 ?: 360;
}

int ConfigParser::findFiles(std::string loc, std::string type, std::vector<std::string>* fileList) {
    //const std::string PATH = filesystem::current_path().string() + "/";
    for (auto &p: filesystem::recursive_directory_iterator(loc)) {
        if (p.path().extension() == type)
            fileList->push_back(p.path().string());
    }
    cout << "Found " << fileList->size() << " candidate file(s) with extension " << type << "." << endl;

    return fileList->size();
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

int ConfigParser::loadConfigFileCLI(string path) {
    string line, key, value, name, answer;
    size_t colon, start, end;
    int changes, errors;
    bool custom_shader = false;
    map<string, int>::iterator iter;

    changes = 0;
    errors = 0;

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
        
        answer = line.substr(colon + 1);
        if (!answer.empty()) {
            start = answer.find_first_not_of(WHITESPACE);
            if (start == string::npos)
                value = "";
            else {
                end = answer.find_last_not_of(WHITESPACE);
                value = answer.substr(start, end - start + 1);
            }
        } else
            value = "";

        switch(iter->second) {
            case 1:
                config.waves = stoi(value);
                changes++;
                break;
            case 2:
                config.amplitude = stof(value);
                changes++;
                break;
            case 3:
                config.period = stof(value);
                changes++;
                break;
            case 4:
                config.wavelength = stod(value);
                changes++;
                break;
            case 5:
                config.resolution = stoi(value);
                changes++;
                break;
            case 6:
                config.vert = value;
                if (!value.empty())
                    custom_shader = true;
                changes++;
                break;
            case 7:
                config.superposition = string("true") == value;
                changes++;
                break;
            case 8:
                config.parallel = string("parallel") == value;
                changes++;
                break;
            case 9:
                config.cpu = string("cpu") == value;
                changes++;
                break;
            case 10:
                config.sphere = string("true") == value;
                changes++;
                break;
            case 11:
                config.frag = value;
                changes++;
                break;
            case 12:
                config.cloudTolerance = stod(value);
                changes++;
                break;
            case 13:
                config.cloudLayDivisor = stoi(value);
                changes++;
                break;
            case 14:
                config.cloudResolution = stoi(value);
                changes++;
                break;
            default:
                continue;
        }
    }
    if (changes < 14)
        cout << "Some configuration values not found; defaults were used instead." << endl;

    string ortho = "ortho_wave.vert";
    string para = "para_wave.vert";
    string super = "cpu_wave.vert";
    string sphere = "para_sphere.vert";
    string shad = config.vert;
    if (custom_shader) {
        if (shad == ortho) {
            if (config.parallel) {
                cout << "ERROR: Specified parallel (coplanar) waves with orthogonal wave shader." << endl;
                errors++;
            }
            if (config.cpu) {
                cout << "ERROR: \"ortho_wave.vert\" is only intended for GPU-based calculation." << endl;
                errors++;
            }
        } else if (shad == para) {
            if (!config.parallel) {
                cout << "ERROR: Specified orthogonal waves with parallel (coplanar) wave shader." << endl;
                errors++;
            }
            if (config.cpu) {
                cout << "ERROR: \"para_wave.vert\" is only intended for GPU-based calculation." << endl;
                errors++;
            }
        } else if (shad == super) {
            if (!config.cpu) {
                cout << "ERROR: \"cpu_wave.vert\" is only intended for CPU-based calculation." << endl;
                errors++;
            }
        } else {
            cout << "INFO: Custom shader in use. Disabling consistency checks." << endl;
            goto label_abort;
        }
    } else {
        if (!config.cpu) {
            if (config.superposition) {
                cout << "ERROR: Cannot calculate superposition on GPU." << endl;
                errors++;
            }
            if (config.parallel) {
                cout << "For parallel (coplanar) waves on GPU, auto-selecting shader \"para_wave.vert\"." << endl;
                config.vert = para;
            } else {
                cout << "For orthogonal waves on GPU, auto-selecting shader \"ortho_wave.vert\"." << endl;
                config.vert = ortho;
            }
        } else {
            cout << "CPU calculation requested; auto-selecting shader \"cpu_wave.vert\"." << endl;
            config.vert = super;
        }
    }
    if (config.sphere) {
        cout << "Special case \"sphere\" selected. Using \"para_sphere.vert\" on GPU." << endl;
        config.vert = sphere;
        config.cpu = false;
    }

label_abort:
    file.close();
    return errors;
}

int ConfigParser::loadConfigFileGUI(string path, AtomixConfig *inCfg) {
    string line, key, value, name, answer;
    size_t colon, start, end;
    int changes, errors;
    bool custom_shader = false;
    map<string, int>::iterator iter;

    changes = 0;
    errors = 0;

    name = path.substr(path.find_last_of("/") + 1);
    //cout << "Using config file: " << name << endl;

    ifstream file(path);

    while (getline(file, line)) {
        if (!line.length() || line[0] == '#')
            continue;
        
        colon = line.find(":");
        key = line.substr(0, colon);
        iter = cfgValues.find(key);
        if (iter == cfgValues.end())
            continue;
        
        answer = line.substr(colon + 1);
        if (!answer.empty()) {
            start = answer.find_first_not_of(WHITESPACE);
            if (start == string::npos)
                value = "";
            else {
                end = answer.find_last_not_of(WHITESPACE);
                value = answer.substr(start, end - start + 1);
            }
        } else
            value = "";

        switch(iter->second) {
            case 1:
                inCfg->waves = stoi(value);
                changes++;
                break;
            case 2:
                inCfg->amplitude = stof(value);
                changes++;
                break;
            case 3:
                inCfg->period = stof(value);
                changes++;
                break;
            case 4:
                inCfg->wavelength = stod(value);
                changes++;
                break;
            case 5:
                inCfg->resolution = stoi(value);
                changes++;
                break;
            case 6:
                inCfg->vert = value;
                if (!value.empty())
                    custom_shader = true;
                changes++;
                break;
            case 7:
                inCfg->superposition = string("true") == value;
                changes++;
                break;
            case 8:
                inCfg->parallel = string("parallel") == value;
                changes++;
                break;
            case 9:
                inCfg->cpu = string("cpu") == value;
                changes++;
                break;
            case 10:
                inCfg->sphere = string("true") == value;
                changes++;
                break;
            case 11:
                inCfg->frag = value;
                if (!value.empty())
                    custom_shader = true;
                changes++;
                break;
            case 12:
                inCfg->cloudTolerance = stod(value);
                changes++;
                break;
            case 13:
                inCfg->cloudLayDivisor = stoi(value);
                changes++;
                break;
            case 14:
                inCfg->cloudResolution = stoi(value);
                changes++;
                break;
            default:
                continue;
        }
    }
    if (changes < 14)
        cout << "Some configuration values not found; defaults were used instead." << endl;

    string ortho = "gpu_ortho_circle.vert";
    string para = "gpu_para_circle.vert";
    string super = "cpu_circle.vert";
    string gpu_sphere = "gpu_sphere.vert";
    string cpu_sphere = "cpu_sphere.vert";
    string path = std::string(ROOT_DIR) + std::string(SHADERS);
    string vshad = path + inCfg->vert;
    string fshad = path + inCfg->frag;
    if (custom_shader) {
        /* Here we check for valid shader files */
        if (std::find(vshFiles.begin(), vshFiles.end(), vshad) == vshFiles.end()) {
            cout << "Invalid vertex shader: no such file found." << endl;
            errors++;
        }
        if (std::find(fshFiles.begin(), fshFiles.end(), fshad) == fshFiles.end()) {
            cout << "Invalid fragment shader: no such file found." << endl;
            errors++;
        }
    }
    /* Here we check for Ortho/Super conflicts */
    if (!inCfg->parallel) {
        if (inCfg->superposition) {
            cout << "Invalid combo: Orthogonal waves and Superposition." << endl;
            errors++;
        }
        if (inCfg->sphere) {
            cout << "Invalid combo: Orthogonal waves and Spherical waves." << endl;
            errors++;
        }
    }
    if (inCfg->superposition) {
        if (!inCfg->cpu) {
            cout << "Invalid combo: Superposition and GPU rendering." << endl;
            errors++;
        }
        if (inCfg->sphere) {
            cout << "Invalid combo: Superposition and Spherical waves." << endl;
            errors++;
        }
    }

label_abort:
    file.close();
    return errors;
}

int ConfigParser::populateConfig() {
    int status = 0;

    if (!findFiles(std::string(ROOT_DIR) + std::string(CONFIGS), CFGEXT, &cfgFiles)) {
        cout << "Using default configuration." << endl;
    } else {
        int choice = chooseConfigFile();
        if (choice >= 0) {
            if ((status = loadConfigFileGUI(cfgFiles[choice], &this->config))) {
                cout << "ERROR: Errors in config file. Please correct." << endl;
            }
        }
    }
    return status;
}

void ConfigParser::printConfig() {
    cout << "Orbits: " << config.waves << "\n";
    cout << "Amplitude: " << config.amplitude << "\n";
    cout << "Period: " << config.period << "\n";
    cout << "Wavelength: " << config.wavelength << "\n";
    cout << "Resolution: " << config.resolution << "\n";
    cout << "Parallel: " << config.parallel << "\n";
    cout << "Superposition: " << config.superposition << "\n";
    cout << "CPU: " << config.cpu << "\n";
    cout << "Sphere: " << config.sphere << "\n";
    cout << "Vert Shader: " << config.vert << "\n";
    cout << "Frag Shader: " << config.frag << endl;
}
