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

void FileHandler::findFiles() {
    wavFiles.clear();
    cldFiles.clear();
    vshFiles.clear();
    fshFiles.clear();

    std::tuple<std::string, std::string, QStringList *> pack[4] = {
        {atomixFiles.configs(), atomixFiles.WAVEXT, &wavFiles},
        {atomixFiles.configs(), atomixFiles.CLDEXT, &cldFiles},
        {atomixFiles.shaders(), atomixFiles.VSHEXT, &vshFiles},
        {atomixFiles.shaders(), atomixFiles.FSHEXT, &fshFiles}
    };

    for (auto &p: pack) {
        std::string loc = std::get<0>(p);
        std::string ext = std::get<1>(p);
        QStringList* fileList = std::get<2>(p);

        for (auto &e: std::filesystem::recursive_directory_iterator(loc)) {
            if (e.path().extension() == ext) {
                (*fileList) << QString::fromStdString(e.path().string());
            }
        }
    }
}

SuperConfig FileHandler::loadConfigFile(QString filepath, harmap *recipes) {
    QFile f(filepath);
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    QJsonObject jo = doc.object();

    return configFromJson(jo, recipes);
}

void FileHandler::saveConfigFile(QString filepath, SuperConfig &cfg, harmap *recipes) {
    QFile f(filepath);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QJsonDocument doc(configToJson(cfg, recipes));
    f.write(doc.toJson());
    f.close();
}

SuperConfig FileHandler::configFromJson(QJsonObject &json, harmap *recipes) {
    SuperConfig config;

    if (const QJsonValue v = json["type"]; v.isString()) {
        if (v.toString() == "wave") {
            AtomixWaveConfig cfg;

            if (const QJsonValue h = json["waves"]; h.isDouble()) {
                cfg.waves = h.toInt();
            }
            if (const QJsonValue h = json["amplitude"]; h.isDouble()) {
                cfg.amplitude = h.toDouble();
            }
            if (const QJsonValue h = json["period"]; h.isDouble()) {
                cfg.period = h.toDouble();
            }
            if (const QJsonValue h = json["wavelength"]; h.isDouble()) {
                cfg.wavelength = h.toDouble();
            }
            if (const QJsonValue h = json["resolution"]; h.isDouble()) {
                cfg.resolution = h.toInt();
            }
            if (const QJsonValue h = json["parallel"]; h.isBool()) {
                cfg.parallel = h.toBool();
            }
            if (const QJsonValue h = json["superposition"]; h.isBool()) {
                cfg.superposition = h.toBool();
            }
            if (const QJsonValue h = json["cpu"]; h.isBool()) {
                cfg.cpu = h.toBool();
            }
            if (const QJsonValue h = json["sphere"]; h.isBool()) {
                cfg.sphere = h.toBool();
            }
            if (const QJsonValue h = json["visibleOrbits"]; h.isDouble()) {
                cfg.visibleOrbits = h.toInt();
            }
            config = cfg;
        } else if (v.toString() == "cloud") {
            AtomixCloudConfig cfg;

            if (const QJsonValue h = json["cloudLayDivisor"]; h.isDouble()) {
                cfg.cloudLayDivisor = h.toInt();
            }
            if (const QJsonValue h = json["cloudResolution"]; h.isDouble()) {
                cfg.cloudResolution = h.toInt();
            }
            if (const QJsonValue h = json["cloudTolerance"]; h.isDouble()) {
                cfg.cloudTolerance = h.toDouble();
            }
            if (const QJsonValue h = json["cloudCull_x"]; h.isDouble()) {
                cfg.cloudCull_x = h.toDouble();
            }
            if (const QJsonValue h = json["cloudCull_y"]; h.isDouble()) {
                cfg.cloudCull_y = h.toDouble();
            }
            if (const QJsonValue h = json["cloudCull_rIn"]; h.isDouble()) {
                cfg.cloudCull_rIn = h.toDouble();
            }
            if (const QJsonValue h = json["cloudCull_rOut"]; h.isDouble()) {
                cfg.cloudCull_rOut = h.toDouble();
            }
            if (const QJsonValue h = json["cpu"]; h.isBool()) {
                cfg.cpu = h.toBool();
            }
            if (const QJsonValue h = json["recipes"]; h.isArray()) {
                const QJsonArray ja = h.toArray();
                *recipes = inflateHarmap(ja);
            }
            config = cfg;
        }
    }

    return config;
}

QJsonObject FileHandler::configToJson(SuperConfig &cfg, harmap *recipes) {
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
        jo["recipes"] = collapseHarmap(recipes);
    }

    return jo;
}

QJsonArray FileHandler::collapseHarmap(harmap *har) {
    QJsonArray ja;

    for (auto [key, vec] : *har) {
        for (auto v : vec) {
            QJsonObject jo;
            jo["Principal"] = key;
            jo["Azimuthal"] = v.x;
            jo["Magnetic"] = v.y;
            jo["Weight"] = v.z;
            
            ja.append(jo);
        }
    }

    return ja;
}

harmap FileHandler::inflateHarmap(const QJsonArray &ja) {
    harmap har;

    for (int i = 0; i < ja.size(); i++) {
        QJsonObject jo = ja.at(i).toObject();
        glm::ivec3 v(jo["Azimuthal"].toInt(), jo["Magnetic"].toInt(), jo["Weight"].toInt());
        har[jo["Principal"].toInt()].push_back(v);
    }

    return har;
}

bool FileHandler::deleteFile(QString filepath) {
    return QFile::remove(filepath);
}

void FileHandler::printConfig(SuperConfig &config) {
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
