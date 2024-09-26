/**
 * MainWindow.cpp
 *
 *    Created on: Oct 3, 2023
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

#include "MainWindow.hpp"

MainWindow::MainWindow() {
    onAddNew();
}

void MainWindow::lockConfig(AtomixConfig *cfg) {
    //cout << "Updating config." << endl;
    emit sendConfig(cfg);
}

void MainWindow::onAddNew() {
    cfgParser = new ConfigParser;
    graph = new GWidget(this, cfgParser);
    customConfig = new AtomixConfig;

    /* Setup Dock GUI */
    setupTabs();
    addDockWidget(Qt::RightDockWidgetArea, dockTabs);
    setCentralWidget(graph);

    refreshConfigs();
    refreshShaders();
    loadConfig();
    refreshOrbits(cfgParser->config);
    
    connect(this, &MainWindow::sendConfig, graph, &GWidget::newWaveConfig, Qt::DirectConnection);
    connect(comboConfigFile, &QComboBox::activated, this, &MainWindow::handleComboCfg);
    connect(buttMorbWaves, &QPushButton::clicked, this, &MainWindow::handleButtMorb);
    connect(buttGroupOrbits, &QButtonGroup::idToggled, graph, &GWidget::selectRenderedWaves, Qt::DirectConnection);
    connect(buttGroupColors, &QButtonGroup::idClicked, this, &MainWindow::handleButtColors);
    connect(buttMorbHarmonics, &QPushButton::clicked, this, &MainWindow::handleButtMorbHarmonics);
    connect(treeOrbitalSelect, &QTreeWidget::itemChanged, this, &MainWindow::handleRecipeCheck);

    setWindowTitle(tr("atomix"));
}

void MainWindow::refreshConfigs() {
    int files = cfgParser->cfgFiles.size();
    int rootLength = ROOT_DIR.length() + CONFIGS.length();

    if (!files)
        files = cfgParser->findFiles(std::string(ROOT_DIR) + std::string(CONFIGS), CFGEXT, &cfgParser->cfgFiles);
    assert(files);

    comboConfigFile->clear();
    for (int i = 0; i < files; i++) {
        comboConfigFile->addItem(QString::fromStdString(cfgParser->cfgFiles[i]).sliced(rootLength), i + 1);
    }
    comboConfigFile->addItem(tr("Custom"), files + 1);
    comboConfigFile->setCurrentText(DEFAULT);
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

void MainWindow::refreshOrbits(AtomixConfig *cfg) {
    const QSignalBlocker blocker(buttGroupOrbits);
    ushort renderedOrbits = 0;
    
    for (int i = 0; i < cfg->waves; i++) {
        renderedOrbits |= (1 << i);
    }
    for (int i = 0; i < MAX_ORBITS; i++) {
        uint checkID = 1 << i;
        QAbstractButton *checkBox = buttGroupOrbits->button(checkID);
        bool checkState = (renderedOrbits & checkID);

        checkBox->setEnabled(checkState);
        checkBox->setChecked(checkState);
    }
}

void MainWindow::loadConfig() {
    int files = cfgParser->cfgFiles.size();
    int comboID = comboConfigFile->currentData().toInt();
    AtomixConfig *cfg = nullptr;

    if (comboID <= files) {
        assert(!cfgParser->loadConfigFileGUI(cfgParser->cfgFiles[comboID - 1]));
        cfg = cfgParser->config;
    } else if (comboID == files + 1) {
        cfg = customConfig;
    } else {
        return;
    }

    entryOrbit->setText(QString::number(cfg->waves));
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

    //refreshOrbits(cfg);
}

void MainWindow::setupTabs() {
    dockTabs = new QDockWidget(this);
    wTabs = new QTabWidget(this);

    setupDockWaves();
    setupDockHarmonics();
    wTabs->addTab(wTabWaves, tr("Waves"));
    wTabs->addTab(wTabHarmonics, tr("Harmonics"));
    // wTabs->setTabShape(QTabWidget::TabShape::Triangular);
    int intTabWidth = wTabWaves->width() / wTabs->count();
    QString strTabStyle = QString("QWidget { font-size: 17px; }"\
                                  "QLabel { font-size: 23px; }"\
                                  "QLabel#configLabel { font-size: 17px; }"
                                  "QTabBar::tab { height: 40px; width: %1 px; }"\
                                  "QTabBar::tab:selected { color: #9999FF; font-size: 18px; }"\
                                  "QTabBar::tab:!selected { color: #999999; background: #222222; }").arg(intTabWidth);
    wTabs->setStyleSheet(strTabStyle);

    dockTabs->setWidget(wTabs);
}

void MainWindow::setupDockWaves() {
    comboConfigFile = new QComboBox(this);
    wTabWaves = new QWidget(this);
    
    buttMorbWaves = new QPushButton("Morb", this);
    buttGroupColors = new QButtonGroup(this);

    QSizePolicy qPolicyExpand = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layDock = new QVBoxLayout;
    QVBoxLayout *layConfigFile = new QVBoxLayout;
    QVBoxLayout *layOptionBox = new QVBoxLayout;
    QHBoxLayout *layOrbitSelect = new QHBoxLayout;
    QHBoxLayout *layOptionRow1 = new QHBoxLayout;
    QHBoxLayout *layOptionRow2 = new QHBoxLayout;
    QHBoxLayout *layOptionRow3 = new QHBoxLayout;
    QHBoxLayout *layOptionRow4 = new QHBoxLayout;
    QHBoxLayout *layOptionRow5 = new QHBoxLayout;
    QHBoxLayout *layOptionRow6 = new QHBoxLayout;
    QHBoxLayout *layOptionRow7 = new QHBoxLayout;
    QHBoxLayout *layOptionRow8 = new QHBoxLayout;
    QHBoxLayout *layOptionRow9 = new QHBoxLayout;
    QHBoxLayout *layOptionRow10 = new QHBoxLayout;
    QHBoxLayout *layOptionRow11 = new QHBoxLayout;
    QHBoxLayout *layColorPicker = new QHBoxLayout;

    QGroupBox *groupConfig = new QGroupBox("Config File");
    QGroupBox *groupOptions = new QGroupBox("Config Options");
    QGroupBox *groupColors = new QGroupBox("Wave Colors");
    QGroupBox *groupOrbits = new QGroupBox("Visible Orbits");

    QLabel *labelWaves = new QLabel("<p>Explore stable wave patterns in circular or spherical forms in this configuration</p>");
    labelWaves->setMaximumHeight(intTabLabelHeight);
    labelWaves->setMinimumHeight(intTabLabelHeight);
    labelWaves->setWordWrap(true);
    labelWaves->setFrameStyle(QFrame::Panel | QFrame::Raised);
    labelWaves->setLineWidth(3);
    labelWaves->setMargin(12);
    labelWaves->setAlignment(Qt::AlignCenter);
    
    QLabel *labelOrbit = new QLabel("Number of orbits:");
    labelOrbit->setObjectName("configLabel");
    QLabel *labelAmp = new QLabel("Amplitude:");
    labelAmp->setObjectName("configLabel");
    QLabel *labelPeriod = new QLabel("Period (N * pi):");
    labelPeriod->setObjectName("configLabel");
    QLabel *labelWavelength = new QLabel("Wavelength (N * pi):");
    labelWavelength->setObjectName("configLabel");
    QLabel *labelResolution = new QLabel("Resolution:");
    labelResolution->setObjectName("configLabel");
    QLabel *labelOrthoPara = new QLabel("Orthogonal vs Parallel:");
    labelOrthoPara->setObjectName("configLabel");
    QLabel *labelSuper = new QLabel("Superposition:");
    labelSuper->setObjectName("configLabel");
    QLabel *labelCPU = new QLabel("CPU vs GPU:");
    labelCPU->setObjectName("configLabel");
    QLabel *labelSphere = new QLabel("Spherical vs Circular:");
    labelSphere->setObjectName("configLabel");
    QLabel *labelVertex = new QLabel("Vertex Shader:");
    labelVertex->setObjectName("configLabel");
    QLabel *labelFrag = new QLabel("Fragment Shader:");
    labelFrag->setObjectName("configLabel");

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

    entryOrbit->setAlignment(Qt::AlignRight);
    entryAmp->setAlignment(Qt::AlignRight);
    entryPeriod->setAlignment(Qt::AlignRight);
    entryWavelength->setAlignment(Qt::AlignRight);
    entryResolution->setAlignment(Qt::AlignRight);

    QButtonGroup *buttGroupOrtho = new QButtonGroup();
    buttGroupOrtho->addButton(entryOrtho, 1);
    buttGroupOrtho->addButton(entryPara, 2);
    entryOrtho->toggle();
    QButtonGroup *buttGroupSuper = new QButtonGroup();
    buttGroupSuper->addButton(entrySuperOn, 4);
    buttGroupSuper->addButton(entrySuperOff, 8);
    entrySuperOff->toggle();
    QButtonGroup *buttGroupCPU = new QButtonGroup();
    buttGroupCPU->addButton(entryCPU, 16);
    buttGroupCPU->addButton(entryGPU, 32);
    entryGPU->toggle();
    QButtonGroup *buttGroupSphere = new QButtonGroup();
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
    layOrbitSelect->addWidget(orbit1);
    layOrbitSelect->addWidget(orbit2);
    layOrbitSelect->addWidget(orbit3);
    layOrbitSelect->addWidget(orbit4);
    layOrbitSelect->addWidget(orbit5);
    layOrbitSelect->addWidget(orbit6);
    layOrbitSelect->addWidget(orbit7);
    layOrbitSelect->addWidget(orbit8);

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

    layConfigFile->addWidget(comboConfigFile);

    layOptionRow1->addWidget(labelOrbit, 2, Qt::AlignLeft);
    layOptionRow1->addWidget(entryOrbit, 2, Qt::AlignRight);
    layOptionRow2->addWidget(labelAmp, 2, Qt::AlignLeft);
    layOptionRow2->addWidget(entryAmp, 2, Qt::AlignRight);
    layOptionRow3->addWidget(labelPeriod, 2, Qt::AlignLeft);
    layOptionRow3->addWidget(entryPeriod, 2, Qt::AlignRight);
    layOptionRow4->addWidget(labelWavelength, 2, Qt::AlignLeft);
    layOptionRow4->addWidget(entryWavelength, 2, Qt::AlignRight);
    layOptionRow5->addWidget(labelResolution, 2, Qt::AlignLeft);
    layOptionRow5->addWidget(entryResolution, 2, Qt::AlignRight);
    layOptionRow6->addWidget(labelOrthoPara, 2, Qt::AlignLeft);
    layOptionRow6->addWidget(entryPara, 1, Qt::AlignRight);
    layOptionRow6->addWidget(entryOrtho, 1, Qt::AlignRight);
    layOptionRow7->addWidget(labelSuper, 2, Qt::AlignLeft);
    layOptionRow7->addWidget(entrySuperOn, 1, Qt::AlignRight);
    layOptionRow7->addWidget(entrySuperOff, 1, Qt::AlignRight);
    layOptionRow8->addWidget(labelCPU, 2, Qt::AlignLeft);
    layOptionRow8->addWidget(entryCPU, 1, Qt::AlignRight);
    layOptionRow8->addWidget(entryGPU, 1, Qt::AlignRight);
    layOptionRow9->addWidget(labelSphere, 2, Qt::AlignLeft);
    layOptionRow9->addWidget(entrySphere, 1, Qt::AlignRight);
    layOptionRow9->addWidget(entryCircle, 1, Qt::AlignRight);
    layOptionRow10->addWidget(labelVertex, 2, Qt::AlignLeft);
    layOptionRow10->addWidget(entryVertex, 2, Qt::AlignRight);
    layOptionRow11->addWidget(labelFrag, 2, Qt::AlignLeft);
    layOptionRow11->addWidget(entryFrag, 2, Qt::AlignRight);
    
    layOptionBox->addLayout(layOptionRow1);
    layOptionBox->addLayout(layOptionRow2);
    layOptionBox->addLayout(layOptionRow3);
    layOptionBox->addLayout(layOptionRow4);
    layOptionBox->addLayout(layOptionRow5);
    layOptionBox->addLayout(layOptionRow6);
    layOptionBox->addLayout(layOptionRow7);
    layOptionBox->addLayout(layOptionRow8);
    layOptionBox->addLayout(layOptionRow9);
    layOptionBox->addLayout(layOptionRow10);
    layOptionBox->addLayout(layOptionRow11);

    QPushButton *buttColorPeak = new QPushButton("Peak");
    QPushButton *buttColorBase = new QPushButton("Base");
    QPushButton *buttColorTrough = new QPushButton("Trough");
    buttGroupColors->addButton(buttColorPeak, 1);
    buttGroupColors->addButton(buttColorBase, 2);
    buttGroupColors->addButton(buttColorTrough, 3);
    buttColorPeak->setStyleSheet("QPushButton {background-color: #FF00FF; color: #000000;}");
    buttColorBase->setStyleSheet("QPushButton {background-color: #0000FF; color: #000000;}");
    buttColorTrough->setStyleSheet("QPushButton {background-color: #00FFFF; color: #000000;}");

    layColorPicker->addWidget(buttColorPeak);
    layColorPicker->addWidget(buttColorBase);
    layColorPicker->addWidget(buttColorTrough);

    groupConfig->setLayout(layConfigFile);
    groupOptions->setLayout(layOptionBox);
    groupColors->setLayout(layColorPicker);
    groupOrbits->setLayout(layOrbitSelect);
    
    layDock->addWidget(labelWaves);
    layDock->addStretch(2);
    layDock->addWidget(groupConfig);
    layDock->addWidget(groupOptions);
    layDock->addWidget(buttMorbWaves);
    layDock->addStretch(2);
    layDock->addWidget(groupColors);
    layDock->addWidget(groupOrbits);    

    layDock->setStretchFactor(labelWaves, 2);
    layDock->setStretchFactor(groupConfig, 1);
    layDock->setStretchFactor(groupOptions, 6);
    layDock->setStretchFactor(buttMorbWaves, 1);
    layDock->setStretchFactor(groupColors, 1);
    layDock->setStretchFactor(groupOrbits, 1);

    buttMorbWaves->setSizePolicy(qPolicyExpand);
    
    wTabWaves->setLayout(layDock);
    wTabWaves->setMinimumSize(intTabMinWidth,0);
}

void MainWindow::setupDockHarmonics() {
    wTabHarmonics = new QWidget(this);
    buttMorbHarmonics = new QPushButton("Generate Cloud", this);
    buttMorbHarmonics->setDisabled(true);
    buttGenVertices = new QPushButton("Generate Vertices", this);

    QSizePolicy qPolicyExpand = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QGroupBox *groupGenVertices = new QGroupBox("Vertex Generation");
    QGroupBox *groupRecipeBuilder = new QGroupBox("Orbital Selector");
    groupRecipeReporter = new QGroupBox("Selected Orbitals");

    QLabel *labelHarmonics = new QLabel("Generate accurate atomic orbital probability clouds for (<i>n</i>, <i>l</i>, <i>m<sub>l</sub></i>)");
    labelHarmonics->setMaximumHeight(intTabLabelHeight);
    labelHarmonics->setMinimumHeight(intTabLabelHeight);
    labelHarmonics->setWordWrap(true);
    labelHarmonics->setFrameStyle(QFrame::Panel | QFrame::Raised);
    labelHarmonics->setLineWidth(3);
    labelHarmonics->setMargin(12);
    labelHarmonics->setAlignment(Qt::AlignCenter);

    treeOrbitalSelect = new QTreeWidget();
    treeOrbitalSelect->setSortingEnabled(true);
    QStringList strlistTreeHeaders = { "Orbital" };
    treeOrbitalSelect->setHeaderLabels(strlistTreeHeaders);
    treeOrbitalSelect->header()->setDefaultAlignment(Qt::AlignCenter);
    QFont fontTree = treeOrbitalSelect->font();
    fontTree.setPointSize(14);
    treeOrbitalSelect->setFont(fontTree);

    QTreeWidgetItem *lastN = nullptr;
    QTreeWidgetItem *lastL = nullptr;
    QTreeWidgetItem *thisItem = nullptr;
    for (int n = 0; n <= MAX_ORBITS; n++) {
        QStringList treeitemParentN = { QString("%1 _ _").arg(n), QString::number(n), QString("-"), QString("-") };
        thisItem = new QTreeWidgetItem(treeOrbitalSelect, treeitemParentN);
        thisItem->setTextAlignment(0, Qt::AlignLeft);
        thisItem->setTextAlignment(1, Qt::AlignCenter);
        thisItem->setTextAlignment(2, Qt::AlignCenter);
        thisItem->setTextAlignment(3, Qt::AlignCenter);
        thisItem->setCheckState(0, Qt::Unchecked);
        lastN = thisItem;
        for (int l = 0; l < n; l++) {
            QStringList treeitemParentL = { QString("%1 %2 _").arg(n).arg(l), QString::number(n), QString::number(l), QString("-") };
            thisItem = new QTreeWidgetItem(lastN, treeitemParentL);
            thisItem->setTextAlignment(0, Qt::AlignLeft);
            thisItem->setTextAlignment(1, Qt::AlignCenter);
            thisItem->setTextAlignment(2, Qt::AlignCenter);
            thisItem->setTextAlignment(3, Qt::AlignCenter);
            thisItem->setCheckState(0, Qt::Unchecked);
            lastL = thisItem;
            for (int m_l = l; m_l >= -l; m_l--) {
                QStringList treeitemFinal = { QString("%1 %2 %3").arg(n).arg(l).arg(m_l), QString::number(n), QString::number(l), QString::number(m_l) };
                thisItem = new QTreeWidgetItem(lastL, treeitemFinal);
                thisItem->setTextAlignment(0, Qt::AlignLeft);
                thisItem->setTextAlignment(1, Qt::AlignCenter);
                thisItem->setTextAlignment(2, Qt::AlignCenter);
                thisItem->setTextAlignment(3, Qt::AlignCenter);
                thisItem->setCheckState(0, Qt::Unchecked);
            }
        }
    }

    listOrbitalReport = new QListWidget();
    listOrbitalReport->setFont(fontTree);

    QVBoxLayout *layGenVertices = new QVBoxLayout;
    layGenVertices->addWidget(buttGenVertices);
    groupGenVertices->setLayout(layGenVertices);
    QVBoxLayout *layRecipeBuilder = new QVBoxLayout;
    layRecipeBuilder->addWidget(treeOrbitalSelect);
    groupRecipeBuilder->setLayout(layRecipeBuilder);
    QVBoxLayout *layRecipeReporter = new QVBoxLayout;
    layRecipeReporter->addWidget(listOrbitalReport);
    groupRecipeReporter->setLayout(layRecipeReporter);

    QHBoxLayout *layRecipeIO = new QHBoxLayout;
    layRecipeIO->addWidget(groupRecipeBuilder);
    layRecipeIO->addWidget(groupRecipeReporter);
    groupRecipeReporter->setAlignment(Qt::AlignRight);
    groupRecipeReporter->setStyleSheet("QGroupBox { color: #FF7777; }");
    groupRecipeBuilder->setMaximumWidth(235);
    groupRecipeReporter->setMaximumWidth(235);

    QVBoxLayout *layDockHarmonics = new QVBoxLayout;
    layDockHarmonics->addWidget(labelHarmonics);
    layDockHarmonics->addStretch(1);
    layDockHarmonics->addWidget(groupGenVertices);
    layDockHarmonics->addStretch(1);
    layDockHarmonics->addLayout(layRecipeIO);
    layDockHarmonics->addStretch(1);
    layDockHarmonics->addWidget(buttMorbHarmonics);    

    layDockHarmonics->setStretchFactor(labelHarmonics, 2);
    layDockHarmonics->setStretchFactor(groupGenVertices, 2);
    layDockHarmonics->setStretchFactor(layRecipeIO, 7);
    layDockHarmonics->setStretchFactor(buttMorbHarmonics, 1);
    
    buttMorbHarmonics->setSizePolicy(qPolicyExpand);
    
    wTabHarmonics->setLayout(layDockHarmonics);
    wTabHarmonics->setMinimumSize(intTabMinWidth,0);
}

void MainWindow::handleComboCfg() {
    this->loadConfig();
}

void MainWindow::handleRecipeCheck(QTreeWidgetItem *item, int col) {
    bool checked = item->checkState(col);

    if (checked) {
        QString strItem = item->text(col);
        QStringList strlistItem = strItem.split(u' ');
        QListWidgetItem *thisItem = new QListWidgetItem(strItem, listOrbitalReport);
        listOrbitalReport->addItem(thisItem);
        thisItem->setTextAlignment(Qt::AlignRight);

        if (!buttMorbHarmonics->isEnabled()) {
            buttMorbHarmonics->setEnabled(true);
        }

        int nlm[3] = { 0, 0, 0}, i = 0;
        bool hasInt = false;

        for (auto &str : strlistItem) {
            nlm[i++] = str.toInt(&hasInt);

            if (!hasInt)
                nlm[i-1] = -1;
        }

        // TODO register recipes in cloudManager
    } else {
        QListWidgetItem *thisItem = listOrbitalReport->findItems(item->text(col), Qt::MatchExactly).first();
        listOrbitalReport->takeItem(listOrbitalReport->row(thisItem));
        delete (thisItem);

        if (!listOrbitalReport->count() && buttMorbHarmonics->isEnabled()) {
            buttMorbHarmonics->setEnabled(false);
        }

        // TODO unregister recipes in cloudManager
    }

    QString strRecipes = (listOrbitalReport->count()) ? "QGroupBox { color: #77FF77; }" : "QGroupBox { color: #FF7777; }";
    groupRecipeReporter->setStyleSheet(strRecipes);
}

void MainWindow::handleButtMorb() {
    cfgParser->config->waves = entryOrbit->text().toInt();
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

    refreshOrbits(cfgParser->config);

    lockConfig(cfgParser->config);
}

void MainWindow::handleButtMorbHarmonics() {
    graph->newCloudConfig();
}

void MainWindow::handleButtColors(int id) {
    QColorDialog::ColorDialogOptions colOpts = QFlag(QColorDialog::ShowAlphaChannel);
    QColor colorChoice = QColorDialog::getColor(Qt::white, this, tr("Choose a Color"), colOpts);
    uint color = 0;

    int dRed = colorChoice.red();
    color |= dRed;
    color <<= 8;
    int dGreen = colorChoice.green();
    color |= dGreen;
    color <<= 8;
    int dBlue = colorChoice.blue();
    color |= dBlue;
    uint nbc = color;
    color <<= 8;
    int dAlpha = colorChoice.alpha();
    color |= dAlpha;

    //cout << "Incoming color " << id << ": (" << hex << dRed << ", " << dGreen << ", " << dBlue << ", " << dAlpha << ") as #" << color << endl;
    std::string ntcHex = "000000";
    if (dRed + dGreen + dBlue < 128)
        ntcHex = "FFFFFF";
    std::string nbcHex = std::format("{:06X}", nbc);
    std::string ss = "QPushButton {background-color: #" + nbcHex + "; color: #" + ntcHex + ";}";
    QString qss = QString::fromStdString(ss);
    buttGroupColors->button(id)->setStyleSheet(qss);
    graph->setColorsWaves(id, color);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(e);
}
