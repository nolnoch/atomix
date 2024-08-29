/**
 * MainWindow.cpp
 *
 *    Created on: Oct 3, 2023
 *   Last Update: Oct 14, 2023
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

#include "MainWindow.hpp"

MainWindow::MainWindow() {
    onAddNew();
}

void MainWindow::lockConfig(WaveConfig *cfg) {
    //cout << "Updating config." << endl;
    emit sendConfig(cfg);
}

void MainWindow::onAddNew() {
    cfgParser = new ConfigParser;
    graph = new GWidget(this, cfgParser);
    customConfig = new WaveConfig;

    /* Setup Dock GUI */
    setupDock();
    addDockWidget(Qt::RightDockWidgetArea, controlBox);
    setCentralWidget(graph);

    refreshConfigs();
    refreshShaders();
    loadConfig();
    
    connect(this, &MainWindow::sendConfig, graph, &GWidget::configReceived, Qt::DirectConnection);
    connect(qCombo, &QComboBox::activated, this, &MainWindow::handleComboCfg);
    connect(qMorb, &QPushButton::clicked, this, &MainWindow::handleMorb);
    connect(buttGroupOrbits, &QButtonGroup::idToggled, graph, &GWidget::selectRenderedOrbits, Qt::DirectConnection);

    setWindowTitle(tr("atomix"));
}

void MainWindow::refreshConfigs() {
    int files = cfgParser->cfgFiles.size();
    int rootLength = ROOT_DIR.length() + CONFIGS.length();

    if (!files)
        files = cfgParser->findFiles(std::string(ROOT_DIR) + std::string(CONFIGS), CFGEXT, &cfgParser->cfgFiles);
    assert(files);

    qCombo->clear();
    for (int i = 0; i < files; i++) {
        qCombo->addItem(QString::fromStdString(cfgParser->cfgFiles[i]).sliced(rootLength), i + 1);
    }
    qCombo->addItem(tr("Custom"), files + 1);
    qCombo->setCurrentText(DEFAULT);
}

void MainWindow::refreshShaders() {
    int rootLength = ROOT_DIR.length() + SHADERS.length();
    int files = 0;

    /* Vertex Shaders */
    files = cfgParser->vshFiles.size();
    if (!files)
        files = cfgParser->findFiles(std::string(ROOT_DIR) + std::string(SHADERS), VSHEXT, &cfgParser->vshFiles);
    assert(files);

    entryVertex->clear();
    for (int i = 0; i < files; i++) {
        QString item = QString::fromStdString(cfgParser->vshFiles[i]).sliced(rootLength);
        if (!item.contains("crystal"))
            entryVertex->addItem(item);
    }
    entryVertex->setCurrentText(QString::fromStdString(cfgParser->config->vert));

    /* Fragment Shaders */
    files = cfgParser->fshFiles.size();
    if (!files)
        files = cfgParser->findFiles(std::string(ROOT_DIR) + std::string(SHADERS), FSHEXT, &cfgParser->fshFiles);
    assert(files);

    entryFrag->clear();
    for (int i = 0; i < files; i++) {
        QString item = QString::fromStdString(cfgParser->fshFiles[i]).sliced(rootLength);
        if (!item.contains("crystal"))
            entryFrag->addItem(item);
    }
    entryFrag->setCurrentText(QString::fromStdString(cfgParser->config->frag));
}

void MainWindow::loadConfig() {
    int files = cfgParser->cfgFiles.size();
    int comboID = qCombo->currentData().toInt();
    WaveConfig *cfg = nullptr;

    if (comboID <= files) {
        assert(!cfgParser->loadConfigFileGUI(cfgParser->cfgFiles[comboID - 1]));
        cfg = cfgParser->config;
    } else if (comboID == files + 1) {
        cfg = customConfig;
    } else {
        return;
    }

    entryOrbit->setText(QString::number(cfg->orbits));
    entryAmp->setText(QString::number(cfg->amplitude));
    entryPeriod->setText(QString::number(cfg->period));
    entryWavelength->setText(QString::number(cfg->wavelength));
    entryResolution->setText(QString::number(cfg->resolution));

    /* To my horror and disgust, neither toggle() nor setChecked() work to flip from False to True.
       It is therefore necessary to manually set every radio button individually.
    */    
    entryPara->setChecked(cfg->parallel);
    entryOrtho->setChecked(!cfg->parallel);
    entrySuperOn->setChecked(cfg->superposition);
    entrySuperOff->setChecked(!cfg->superposition);
    entryCPU->setChecked(cfg->cpu);
    entryGPU->setChecked(!cfg->cpu);
    entrySphere->setChecked(cfg->sphere);
    entryCircle->setChecked(!cfg->sphere);
    
    entryVertex->setCurrentText(QString::fromStdString(cfg->vert));
    entryFrag->setCurrentText(QString::fromStdString(cfg->frag));
}

void MainWindow::setupDock() {
    qCombo = new QComboBox(this);
    layGrid = new QVBoxLayout;
    cfgGrid = new QVBoxLayout;
    wDock = new QWidget;
    controlBox = new QDockWidget(this);
    qMorb = new QPushButton("Morb", this);
    QHBoxLayout *orbitSelectLayout = new QHBoxLayout();

    QSizePolicy qPolicy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QGroupBox *groupConfig = new QGroupBox("Configuration");
    QGroupBox *groupOrbits = new QGroupBox("Visible Orbits");
    
    QHBoxLayout *row1 = new QHBoxLayout;
    QHBoxLayout *row2 = new QHBoxLayout;
    QHBoxLayout *row3 = new QHBoxLayout;
    QHBoxLayout *row4 = new QHBoxLayout;
    QHBoxLayout *row5 = new QHBoxLayout;
    QHBoxLayout *row6 = new QHBoxLayout;
    QHBoxLayout *row7 = new QHBoxLayout;
    QHBoxLayout *row8 = new QHBoxLayout;
    QHBoxLayout *row9 = new QHBoxLayout;
    QHBoxLayout *row10 = new QHBoxLayout;
    QHBoxLayout *row11 = new QHBoxLayout;

    QLabel *labelConfig = new QLabel("Select Config File:");
    QLabel *labelOrbit = new QLabel("Number of orbit waves (0,8]");
    QLabel *labelAmp = new QLabel("Amplitude of waves");
    QLabel *labelPeriod = new QLabel("Period of waves (n * PI)");
    QLabel *labelWavelength = new QLabel("Wavelength of waves (n * PI)");
    QLabel *labelResolution = new QLabel("Resolution (points) per wave");
    QLabel *labelOrthoPara = new QLabel("Orthogonal vs Parallel waves");
    QLabel *labelSuper = new QLabel("Superposition on/off");
    QLabel *labelCPU = new QLabel("CPU vs GPU rendering");
    QLabel *labelSphere = new QLabel("Spherical vs Circular waves");
    QLabel *labelVertex = new QLabel("Select Vertex Shader file");
    QLabel *labelFrag = new QLabel("Select Fragment Shader file");

    entryOrbit = new QLineEdit("4");
    entryAmp = new QLineEdit("0.4");
    entryPeriod = new QLineEdit("1.0");
    entryWavelength = new QLineEdit("2.0");
    entryResolution = new QLineEdit("180");
    entryOrtho = new QRadioButton("Ortho");
    entryPara = new QRadioButton("Para");
    entrySuperOn = new QRadioButton("On");
    entrySuperOff = new QRadioButton("Off");
    entryCPU = new QRadioButton("CPU");
    entryGPU = new QRadioButton("GPU");
    entryCircle = new QRadioButton("Circle");
    entrySphere = new QRadioButton("Sphere");
    entryVertex = new QComboBox(this);
    entryFrag = new QComboBox(this);

    buttGroupOrtho = new QButtonGroup();
    buttGroupOrtho->addButton(entryOrtho, 1);
    buttGroupOrtho->addButton(entryPara, 2);
    entryOrtho->toggle();
    buttGroupSuper = new QButtonGroup();
    buttGroupSuper->addButton(entrySuperOn, 4);
    buttGroupSuper->addButton(entrySuperOff, 8);
    entrySuperOff->toggle();
    buttGroupCPU = new QButtonGroup();
    buttGroupCPU->addButton(entryCPU, 16);
    buttGroupCPU->addButton(entryGPU, 32);
    entryGPU->toggle();
    buttGroupSphere = new QButtonGroup();
    buttGroupSphere->addButton(entryCircle, 64);
    buttGroupSphere->addButton(entrySphere, 128);
    entryCircle->toggle();

    QCheckBox *orbit1 = new QCheckBox("1");
    QCheckBox *orbit2 = new QCheckBox("2");
    QCheckBox *orbit3 = new QCheckBox("3");
    QCheckBox *orbit4 = new QCheckBox("4");
    QCheckBox *orbit5 = new QCheckBox("5");
    QCheckBox *orbit6 = new QCheckBox("6");
    QCheckBox *orbit7 = new QCheckBox("7");
    QCheckBox *orbit8 = new QCheckBox("8");
    orbitSelectLayout->addWidget(orbit1);
    orbitSelectLayout->addWidget(orbit2);
    orbitSelectLayout->addWidget(orbit3);
    orbitSelectLayout->addWidget(orbit4);
    orbitSelectLayout->addWidget(orbit5);
    orbitSelectLayout->addWidget(orbit6);
    orbitSelectLayout->addWidget(orbit7);
    orbitSelectLayout->addWidget(orbit8);
    
    orbit1->setCheckState(Qt::CheckState::Checked);
    orbit2->setCheckState(Qt::CheckState::Checked);
    orbit3->setCheckState(Qt::CheckState::Checked);
    orbit4->setCheckState(Qt::CheckState::Checked);
    orbit5->setCheckState(Qt::CheckState::Unchecked);
    orbit6->setCheckState(Qt::CheckState::Unchecked);
    orbit7->setCheckState(Qt::CheckState::Unchecked);
    orbit8->setCheckState(Qt::CheckState::Unchecked);

    buttGroupOrbits = new QButtonGroup();
    buttGroupOrbits->setExclusive(false);
    buttGroupOrbits->addButton(orbit1, 1);
    buttGroupOrbits->addButton(orbit2, 2);
    buttGroupOrbits->addButton(orbit3, 4);
    buttGroupOrbits->addButton(orbit4, 8);
    buttGroupOrbits->addButton(orbit5, 16);
    buttGroupOrbits->addButton(orbit6, 32);
    buttGroupOrbits->addButton(orbit7, 64);
    buttGroupOrbits->addButton(orbit8, 128);

    row1->addWidget(labelOrbit, 2, Qt::AlignLeft);
    row1->addWidget(entryOrbit, 2, Qt::AlignRight);
    row2->addWidget(labelAmp, 2, Qt::AlignLeft);
    row2->addWidget(entryAmp, 2, Qt::AlignRight);
    row3->addWidget(labelPeriod, 2, Qt::AlignLeft);
    row3->addWidget(entryPeriod, 2, Qt::AlignRight);
    row4->addWidget(labelWavelength, 2, Qt::AlignLeft);
    row4->addWidget(entryWavelength, 2, Qt::AlignRight);
    row5->addWidget(labelResolution, 2, Qt::AlignLeft);
    row5->addWidget(entryResolution, 2, Qt::AlignRight);
    row6->addWidget(labelOrthoPara, 2, Qt::AlignLeft);
    row6->addWidget(entryPara, 1, Qt::AlignRight);
    row6->addWidget(entryOrtho, 1, Qt::AlignRight);
    row7->addWidget(labelSuper, 2, Qt::AlignLeft);
    row7->addWidget(entrySuperOn, 1, Qt::AlignRight);
    row7->addWidget(entrySuperOff, 1, Qt::AlignRight);
    row8->addWidget(labelCPU, 2, Qt::AlignLeft);
    row8->addWidget(entryCPU, 1, Qt::AlignRight);
    row8->addWidget(entryGPU, 1, Qt::AlignRight);
    row9->addWidget(labelSphere, 2, Qt::AlignLeft);
    row9->addWidget(entrySphere, 1, Qt::AlignRight);
    row9->addWidget(entryCircle, 1, Qt::AlignRight);
    row10->addWidget(labelVertex, 2, Qt::AlignLeft);
    row10->addWidget(entryVertex, 2, Qt::AlignRight);
    row11->addWidget(labelFrag, 2, Qt::AlignLeft);
    row11->addWidget(entryFrag, 2, Qt::AlignRight);
    
    cfgGrid->addLayout(row1);
    cfgGrid->addLayout(row2);
    cfgGrid->addLayout(row3);
    cfgGrid->addLayout(row4);
    cfgGrid->addLayout(row5);
    cfgGrid->addLayout(row6);
    cfgGrid->addLayout(row7);
    cfgGrid->addLayout(row8);
    cfgGrid->addLayout(row9);
    cfgGrid->addLayout(row10);
    cfgGrid->addLayout(row11);

    groupConfig->setLayout(cfgGrid);
    groupOrbits->setLayout(orbitSelectLayout);
    
    layGrid->addWidget(labelConfig);
    layGrid->addWidget(qCombo);
    layGrid->addStretch(1);
    layGrid->addWidget(groupConfig);
    layGrid->addStretch(1);
    layGrid->addWidget(groupOrbits);
    layGrid->addStretch(1);
    layGrid->addWidget(qMorb);

    layGrid->setStretchFactor(labelConfig, 1);
    layGrid->setStretchFactor(qCombo, 1);
    layGrid->setStretchFactor(groupConfig, 8);
    layGrid->setStretchFactor(groupOrbits, 2);
    layGrid->setStretchFactor(qMorb, 2);
    
    wDock->setLayout(layGrid);
    wDock->setMinimumSize(500,0);
    controlBox->setWidget(wDock);
}

void MainWindow::handleComboCfg() {
    this->loadConfig();
}

void MainWindow::handleMorb() {
    cfgParser->config->orbits = entryOrbit->text().toInt();
    cfgParser->config->amplitude = entryAmp->text().toDouble();
    cfgParser->config->period = entryPeriod->text().toDouble() * M_PI;
    cfgParser->config->wavelength = entryWavelength->text().toDouble() * M_PI;
    cfgParser->config->resolution = entryResolution->text().toInt();
    cfgParser->config->parallel = entryPara->isChecked();
    cfgParser->config->superposition = entrySuperOn->isChecked();
    cfgParser->config->cpu = entryCPU->isChecked();
    cfgParser->config->sphere = entrySphere->isChecked();
    cfgParser->config->vert = entryVertex->currentText().toStdString();
    cfgParser->config->frag = entryFrag->currentText().toStdString();

    lockConfig(cfgParser->config);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}
