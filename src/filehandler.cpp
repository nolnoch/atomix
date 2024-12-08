/**
 * filehandler.hpp
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


#include "filehandler.hpp"


FileHandler::FileHandler() {
}

FileHandler::~FileHandler() {
    wavFiles.clear();
    cldFiles.clear();
    vshFiles.clear();
    fshFiles.clear();
}

void FileHandler::init() {
    findFiles();
}

void FileHandler::findFiles() {
    std::tuple<std::string, std::string, std::vector<std::string>*> pack[4] = {
        {atomixFiles.configs(), atomixFiles.WAVEXT, &wavFiles},
        {atomixFiles.configs(), atomixFiles.CLDEXT, &cldFiles},
        {atomixFiles.shaders(), atomixFiles.VSHEXT, &vshFiles},
        {atomixFiles.shaders(), atomixFiles.FSHEXT, &fshFiles}
    };

    for (auto &p: pack) {
        std::string loc = std::get<0>(p);
        std::string ext = std::get<1>(p);
        std::vector<std::string>* fileList = std::get<2>(p);

        for (auto &p: std::filesystem::recursive_directory_iterator(loc)) {
            if (p.path().extension() == ext) {
                fileList->push_back(p.path().string());
            }
        }
        // cout << "Found " << fileList->size() << " candidate file(s) with extension " << type << "." << endl;
    }
}

std::variant<AtomixWaveConfig, AtomixCloudConfig> FileHandler::loadConfigFile(QString filepath) {
    QFile f(filepath);
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    QJsonObject jo = doc.object();

    return configFromJson(jo);
}

void FileHandler::saveConfigFile(QString filepath, std::variant<AtomixWaveConfig, AtomixCloudConfig> &cfg) {
    QFile f(filepath);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QJsonDocument doc(configToJson(cfg));
    f.write(doc.toJson());
    f.close();
}

std::variant<AtomixWaveConfig, AtomixCloudConfig> FileHandler::configFromJson(QJsonObject &json) {
    std::variant<AtomixWaveConfig, AtomixCloudConfig> config;

    if (const QJsonValue v = json["type"]; v.isString()) {
        if (v.toString() == "wave") {
            AtomixWaveConfig cfg;

            if (const QJsonValue v = json["waves"]; v.isDouble()) {
                cfg.waves = v.toInt();
            }
            if (const QJsonValue v = json["amplitude"]; v.isDouble()) {
                cfg.amplitude = v.toDouble();
            }
            if (const QJsonValue v = json["period"]; v.isDouble()) {
                cfg.period = v.toDouble();
            }
            if (const QJsonValue v = json["wavelength"]; v.isDouble()) {
                cfg.wavelength = v.toDouble();
            }
            if (const QJsonValue v = json["resolution"]; v.isDouble()) {
                cfg.resolution = v.toInt();
            }
            if (const QJsonValue v = json["parallel"]; v.isBool()) {
                cfg.parallel = v.toBool();
            }
            if (const QJsonValue v = json["superposition"]; v.isBool()) {
                cfg.superposition = v.toBool();
            }
            if (const QJsonValue v = json["cpu"]; v.isBool()) {
                cfg.cpu = v.toBool();
            }
            if (const QJsonValue v = json["sphere"]; v.isBool()) {
                cfg.sphere = v.toBool();
            }
            if (const QJsonValue v = json["visibleOrbits"]; v.isDouble()) {
                cfg.visibleOrbits = v.toInt();
            }
            config = cfg;
        } else if (v.toString() == "cloud") {
            AtomixCloudConfig cfg;

            if (const QJsonValue v = json["cloudLayDivisor"]; v.isDouble()) {
                cfg.cloudLayDivisor = v.toInt();
            }
            if (const QJsonValue v = json["cloudResolution"]; v.isDouble()) {
                cfg.cloudResolution = v.toInt();
            }
            if (const QJsonValue v = json["cloudTolerance"]; v.isDouble()) {
                cfg.cloudTolerance = v.toInt();
            }
            if (const QJsonValue v = json["cloudCull_x"]; v.isDouble()) {
                cfg.cloudCull_x = v.toInt();
            }
            if (const QJsonValue v = json["cloudCull_y"]; v.isDouble()) {
                cfg.cloudCull_y = v.toInt();
            }
            if (const QJsonValue v = json["cloudCull_rIn"]; v.isDouble()) {
                cfg.cloudCull_rIn = v.toInt();
            }
            if (const QJsonValue v = json["cloudCull_rOut"]; v.isDouble()) {
                cfg.cloudCull_rOut = v.toInt();
            }
            if (const QJsonValue v = json["cpu"]; v.isBool()) {
                cfg.cpu = v.toBool();
            }
            config = cfg;
        }
    }

    return config;
}

QJsonObject FileHandler::configToJson(std::variant<AtomixWaveConfig, AtomixCloudConfig> &cfg) {
    QJsonObject jo;

    if (std::holds_alternative<AtomixWaveConfig>(cfg)) {
        jo["type"] = "wave";
        jo["waves"] = std::get<AtomixWaveConfig>(cfg).waves;
        jo["amplitude"] = std::get<AtomixWaveConfig>(cfg).amplitude;
        jo["period"] = std::get<AtomixWaveConfig>(cfg).period;
        jo["wavelength"] = std::get<AtomixWaveConfig>(cfg).wavelength;
        jo["resolution"] = std::get<AtomixWaveConfig>(cfg).resolution;
        jo["parallel"] = std::get<AtomixWaveConfig>(cfg).parallel;
        jo["superposition"] = std::get<AtomixWaveConfig>(cfg).superposition;
        jo["cpu"] = std::get<AtomixWaveConfig>(cfg).cpu;
        jo["sphere"] = std::get<AtomixWaveConfig>(cfg).sphere;
        jo["visibleOrbits"] = int(std::get<AtomixWaveConfig>(cfg).visibleOrbits);
    } else if (std::holds_alternative<AtomixCloudConfig>(cfg)) {
        jo["type"] = "cloud";
        jo["cloudLayDivisor"] = std::get<AtomixCloudConfig>(cfg).cloudLayDivisor;
        jo["cloudResolution"] = std::get<AtomixCloudConfig>(cfg).cloudResolution;
        jo["cloudTolerance"] = std::get<AtomixCloudConfig>(cfg).cloudTolerance;
        jo["cloudCull_x"] = std::get<AtomixCloudConfig>(cfg).cloudCull_x;
        jo["cloudCull_y"] = std::get<AtomixCloudConfig>(cfg).cloudCull_y;
        jo["cloudCull_rIn"] = std::get<AtomixCloudConfig>(cfg).cloudCull_rIn;
        jo["cloudCull_rOut"] = std::get<AtomixCloudConfig>(cfg).cloudCull_rOut;
        jo["cpu"] = std::get<AtomixCloudConfig>(cfg).cpu;
    }

    return jo;
}

void FileHandler::printConfig(std::variant<AtomixWaveConfig, AtomixCloudConfig> &config) {
    if (std::holds_alternative<AtomixWaveConfig>(config)) {
        std::cout << "Type: " << std::get<AtomixWaveConfig>(config).type << "\n";
        std::cout << "Orbits: " << std::get<AtomixWaveConfig>(config).waves << "\n";
        std::cout << "Amplitude: " << std::get<AtomixWaveConfig>(config).amplitude << "\n";
        std::cout << "Period: " << std::get<AtomixWaveConfig>(config).period << "\n";
        std::cout << "Wavelength: " << std::get<AtomixWaveConfig>(config).wavelength << "\n";
        std::cout << "Resolution: " << std::get<AtomixWaveConfig>(config).resolution << "\n";
        std::cout << "Parallel: " << std::boolalpha << std::get<AtomixWaveConfig>(config).parallel << "\n";
        std::cout << "Superposition: " << std::boolalpha << std::get<AtomixWaveConfig>(config).superposition << "\n";
        std::cout << "CPU: " << std::boolalpha << std::get<AtomixWaveConfig>(config).cpu << "\n";
        std::cout << "Sphere: " << std::boolalpha << std::get<AtomixWaveConfig>(config).sphere << "\n";
        std::cout << "Visible Orbits: " << std::get<AtomixWaveConfig>(config).visibleOrbits << "\n";
    } else if (std::holds_alternative<AtomixCloudConfig>(config)) {
        std::cout << "Type: " << std::get<AtomixCloudConfig>(config).type << "\n";
        std::cout << "Cloud Layer Divisor: " << std::get<AtomixCloudConfig>(config).cloudLayDivisor << "\n";
        std::cout << "Cloud Resolution: " << std::get<AtomixCloudConfig>(config).cloudResolution << "\n";
        std::cout << "Cloud Tolerance: " << std::get<AtomixCloudConfig>(config).cloudTolerance << "\n";
        std::cout << "Cloud Cull x: " << std::get<AtomixCloudConfig>(config).cloudCull_x << "\n";
        std::cout << "Cloud Cull y: " << std::get<AtomixCloudConfig>(config).cloudCull_y << "\n";
        std::cout << "Cloud Cull rIn: " << std::get<AtomixCloudConfig>(config).cloudCull_rIn << "\n";
        std::cout << "Cloud Cull rOut: " << std::get<AtomixCloudConfig>(config).cloudCull_rOut << "\n";
        std::cout << "CPU: " << std::boolalpha << std::get<AtomixCloudConfig>(config).cpu << "\n";
    }
    std::cout << std::endl;
}
