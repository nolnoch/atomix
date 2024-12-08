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

#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <filesystem>
#include <iostream>
#include <vector>
#include <map>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "global.hpp"


struct AtomixFiles {
    bool setRoot(const std::string &_root) {
        bool hasShaders = std::filesystem::exists(std::string(_root + "/shaders"));
        bool hasConfigs = std::filesystem::exists(std::string(_root + "/configs"));
        
        if (!hasShaders || !hasConfigs) {
            return false;
        }

        rootDir = _root + "/";
        shadersDir = rootDir + "shaders/";
        configsDir = rootDir + "configs/";
        resourcesDir = rootDir + "resources/";
        fontsDir = resourcesDir + "fonts/";
        iconsDir = resourcesDir + "icons/";

        return true;
    }

    constexpr std::string& root() { return rootDir; }
    constexpr std::string& shaders() { return shadersDir; }
    constexpr std::string& configs() { return configsDir; }
    constexpr std::string& resources() { return resourcesDir; }
    constexpr std::string& fonts() { return fontsDir; }
    constexpr std::string& icons() { return iconsDir; }

    const std::string WAVEXT = ".wave";
    const std::string CLDEXT = ".cloud";
    const std::string VSHEXT = ".vert";
    const std::string FSHEXT = ".frag";

private:
    std::string rootDir;
    std::string shadersDir;
    std::string configsDir;
    std::string resourcesDir;
    std::string fontsDir;
    std::string iconsDir;
};
Q_DECLARE_METATYPE(AtomixFiles);


class FileHandler {
    public:
        FileHandler();
        virtual ~FileHandler();
        void init();

        std::variant<AtomixWaveConfig, AtomixCloudConfig> loadConfigFile(QString filepath);
        void saveConfigFile(QString filepath, std::variant<AtomixWaveConfig, AtomixCloudConfig> &cfg);
        std::variant<AtomixWaveConfig, AtomixCloudConfig> configFromJson(QJsonObject &json);
        QJsonObject configToJson(std::variant<AtomixWaveConfig, AtomixCloudConfig> &cfg);

        std::vector<std::string> getWaveFilesList() { return wavFiles; }
        std::vector<std::string> getCloudFilesList() { return cldFiles; }
        std::vector<std::string> getVertexShadersList() { return vshFiles; }
        std::vector<std::string> getFragmentShadersList() { return fshFiles; }
        int getWaveFilesCount() { return wavFiles.size(); }
        int getCloudFilesCount() { return cldFiles.size(); }
        int getVertexShadersCount() { return vshFiles.size(); }
        int getFragmentShadersCount() { return fshFiles.size(); }

        void printConfig(std::variant<AtomixWaveConfig, AtomixCloudConfig> &config);

        AtomixFiles atomixFiles;
        enum ConfigType { WAVE = 0, CLOUD = 1 };

    private:
        void findFiles();

        std::vector<std::string> wavFiles;
        std::vector<std::string> cldFiles;
        std::vector<std::string> vshFiles;
        std::vector<std::string> fshFiles;
        AtomixWaveConfig waveConfig;
        AtomixCloudConfig cloudConfig;
};

#endif