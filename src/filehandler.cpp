/**
 * filehandler.hpp
 *
 *    Created on: Oct 15, 2023
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


#include "filehandler.hpp"


/**
 * @brief Default constructor for FileHandler
 *
 * FileHandler is a class designed to find and manage files used by the
 * application. It is used to find config files, shaders, and other resources
 * used by the application.
 *
 * @details
 * The FileHandler class is used to manage files used by the application. It
 * is used to find config files, shaders, and other resources used by the
 * application. The class is designed to be used as a singleton, as it is
 * not intended to be used by multiple threads.
 */
FileHandler::FileHandler() {
}

/**
 * @brief Destructor for FileHandler
 *
 * @details
 * The destructor for FileHandler clears any loaded files from memory. This
 * is done to prevent memory leaks from occurring. The files are cleared by
 * calling clear() on each of the vectors containing the file paths.
 */
FileHandler::~FileHandler() {
    wavFiles.clear();
    cldFiles.clear();
    vshFiles.clear();
    fshFiles.clear();
}

/**
 * @brief Find all files in the config and shader directories
 *
 * @details
 * This function finds all files in the config and shader directories and
 * stores the file paths in the appropriate vectors. The files are found by
 * calling the findFiles() function on each of the vectors containing the
 * file paths.
 */
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

/**
 * @brief Loads a config file and returns a SuperConfig object
 *
 * @details
 * This function loads a config file and returns a SuperConfig object.
 * The config file is loaded from the given filepath and the recipes
 * are loaded from the given harmap object if it is not null.
 *
 * @param[in] filepath The filepath of the config file to load
 * @param[in] recipes The harmap object containing the recipes to load
 *
 * @returns A SuperConfig object containing the loaded config
 */
SuperConfig FileHandler::loadConfigFile(QString filepath, harmap *recipes) {
    QFile f(filepath);
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    QJsonObject jo = doc.object();

    return configFromJson(jo, recipes);
}

/**
 * @brief Saves a config file
 *
 * @details
 * This function saves a config file from a SuperConfig object. The config
 * file is saved to the given filepath and the recipes are saved from the
 * given harmap object if it is not null.
 *
 * @param[in] filepath The filepath of the config file to save
 * @param[in] cfg The SuperConfig object containing the config to save
 * @param[in] recipes The harmap object containing the recipes to save
 */
void FileHandler::saveConfigFile(QString filepath, SuperConfig &cfg, harmap *recipes) {
    QFile f(filepath);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QJsonDocument doc(configToJson(cfg, recipes));
    f.write(doc.toJson());
    f.close();
}

/**
 * @brief Converts a QJsonObject to a SuperConfig object
 *
 * @details
 * This function is used to convert a QJsonObject to a SuperConfig object.
 * The QJsonObject is expected to contain the config data in JSON format.
 * The recipes are loaded from the given harmap object if it is not null.
 *
 * @param[in] json The QJsonObject to convert
 * @param[in] recipes The harmap object containing the recipes to load
 *
 * @returns A SuperConfig object containing the loaded config
 */
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

/**
 * @brief Convert a SuperConfig object into a QJsonObject
 *
 * If the SuperConfig object is an AtomixWaveConfig, the following keys are
 * set in the returned QJsonObject:
 * - type: "wave"
 * - waves: the number of waves
 * - amplitude: the wave amplitude
 * - period: the wave period
 * - wavelength: the wave wavelength
 * - resolution: the wave resolution
 * - parallel: whether the wave is rendered in parallel
 * - superposition: whether the wave is rendered with superposition
 * - cpu: whether the wave is rendered on the CPU
 * - sphere: whether the wave is rendered as a sphere
 * - visibleOrbits: the number of visible orbits
 *
 * If the SuperConfig object is an AtomixCloudConfig, the following keys are
 * set in the returned QJsonObject:
 * - type: "cloud"
 * - cloudLayDivisor: the number of layers per radius
 * - cloudResolution: the number of points per circle
 * - cloudTolerance: the minimum probability for rendering
 * - cloudCull_x: the culling slider -- theta
 * - cloudCull_y: the culling slider -- phi
 * - cloudCull_rIn: the culling slider -- radius-inward
 * - cloudCull_rOut: the culling slider -- radius-outward
 * - cpu: whether the cloud is rendered on the CPU
 * - recipes: a QJsonArray containing the recipes as a list of QJsonObjects
 *
 * @param cfg the SuperConfig object to convert
 * @param recipes the recipes to include in the QJsonObject
 * @return a QJsonObject containing the converted SuperConfig object
 */
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

/**
 * @brief Convert a harmap into a QJsonArray
 *
 * Each entry in the QJsonArray is a QJsonObject with the following keys:
 * - Principal: the principal quantum number
 * - Azimuthal: the azimuthal quantum number
 * - Magnetic: the magnetic quantum number
 * - Weight: the weight of the recipe
 *
 * The QJsonArray is sorted by Principal quantum number.
 *
 * @param har the harmap to convert
 * @return a QJsonArray containing the converted harmap
 */
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

/**
 * @brief Inflate a harmap from a QJsonArray
 *
 * Each entry in the QJsonArray should be a QJsonObject with the following keys:
 * - Principal: the principal quantum number
 * - Azimuthal: the azimuthal quantum number
 * - Magnetic: the magnetic quantum number
 * - Weight: the weight of the recipe
 *
 * The QJsonArray may be empty, and the returned harmap will be empty in this case.
 *
 * @param ja the QJsonArray to inflate
 * @return a harmap containing the inflated data
 */
harmap FileHandler::inflateHarmap(const QJsonArray &ja) {
    harmap har;

    for (int i = 0; i < ja.size(); i++) {
        QJsonObject jo = ja.at(i).toObject();
        glm::ivec3 v(jo["Azimuthal"].toInt(), jo["Magnetic"].toInt(), jo["Weight"].toInt());
        har[jo["Principal"].toInt()].push_back(v);
    }

    return har;
}

/**
 * @brief Deletes a file
 *
 * @details
 * This function deletes a file located at the given filepath.
 *
 * @param filepath the filepath of the file to delete
 *
 * @returns true if the file was successfully deleted, false otherwise
 */
bool FileHandler::deleteFile(QString filepath) {
    return QFile::remove(filepath);
}

/**
 * @brief Prints a SuperConfig object to the console
 *
 * @details
 * This function prints a SuperConfig object to the console.
 * The config is printed in a human-readable format.
 *
 * @param config the SuperConfig object to print
 */
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
