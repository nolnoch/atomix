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

#include "mainwindow.hpp"

MainWindow::MainWindow() {
    onAddNew();
}

void MainWindow::onAddNew() {
    cfgParser = new ConfigParser;
    graph = new GWidget(this, cfgParser);
    // waveConfig = new AtomixConfig;

    /* Setup Dock GUI */
    setupTabs();
    addDockWidget(Qt::RightDockWidgetArea, dockTabs);
    setCentralWidget(graph);

    refreshConfigs();
    refreshShaders();
    loadConfig();
    refreshOrbits();
    setupDetails();

    connect(graph, SIGNAL(detailsChanged(AtomixInfo*)), this, SLOT(updateDetails(AtomixInfo*)));
    connect(comboConfigFile, &QComboBox::activated, this, &MainWindow::handleComboCfg);
    connect(buttMorbWaves, &QPushButton::clicked, this, &MainWindow::handleButtMorbWaves);
    connect(buttGroupOrbits, &QButtonGroup::idToggled, graph, &GWidget::selectRenderedWaves, Qt::DirectConnection);
    connect(buttGroupColors, &QButtonGroup::idClicked, this, &MainWindow::handleButtColors);
    connect(treeOrbitalSelect, &QTreeWidget::itemChanged, this, &MainWindow::handleRecipeCheck);
    connect(treeOrbitalSelect, &QTreeWidget::itemDoubleClicked, this, &MainWindow::handleDoubleClick);
    connect(buttLockRecipes, &QPushButton::clicked, this, &MainWindow::handleButtLockRecipes);
    connect(buttMorbHarmonics, &QPushButton::clicked, this, &MainWindow::handleButtMorbHarmonics);
    connect(buttResetRecipes, &QPushButton::clicked, this, &MainWindow::handleButtResetRecipes);
    connect(buttClearRecipes, &QPushButton::clicked, this, &MainWindow::handleButtClearRecipes);
    connect(entryCloudLayers, &QLineEdit::textEdited, this, &MainWindow::handleConfigChanged);
    connect(entryCloudRes, &QLineEdit::textEdited, this, &MainWindow::handleConfigChanged);
    connect(entryCloudMinRDP, &QLineEdit::textEdited, this, &MainWindow::handleConfigChanged);
    connect(tableOrbitalReport, &QTableWidget::cellDoubleClicked, this, &MainWindow::handleWeightChange);
    connect(slideCullingX, &QSlider::sliderMoved, this, &MainWindow::handleSlideCullingX);
    connect(slideCullingY, &QSlider::sliderMoved, this, &MainWindow::handleSlideCullingY);
    connect(slideCullingX, &QSlider::sliderReleased, this, &MainWindow::handleSlideReleased);
    connect(slideCullingY, &QSlider::sliderReleased, this, &MainWindow::handleSlideReleased);
    connect(slideBackground, &QSlider::sliderMoved, this, &MainWindow::handleSlideBackground);

    setWindowTitle(tr("atomix"));

    // std::cout << "PC Default Thread Count is: " << QThread::idealThreadCount() << std::endl;
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
    entryVertex->setCurrentText(QString::fromStdString(waveConfig.vert));

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
    entryFrag->setCurrentText(QString::fromStdString(waveConfig.frag));
}

void MainWindow::refreshOrbits() {
    const QSignalBlocker blocker(buttGroupOrbits);
    ushort renderedOrbits = 0;

    for (int i = 0; i < waveConfig.waves; i++) {
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

void MainWindow::setupDetails() {
    fontDebug.setFamily("Monospace");
    fontDebug.setStyleHint(QFont::Courier);
    QString strDetails = QString("Position:      %1\n"\
                                 "View|Near:     %2\n"\
                                 "View|Far:      %3\n\n"\
                                 "Buffer|Vertex: %4\n"\
                                 "Buffer|Data:   %5\n"\
                                 "Buffer|Index:  %6\n"\
                                 "Buffer|Total:  %7\n"\
                                 ).arg("--").arg("--").arg("--").arg("--").arg("--").arg("--").arg("--");
    labelDetails = new QLabel(graph);
    labelDetails->setFont(fontDebug);
    labelDetails->setText(strDetails);
    labelDetails->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    labelDetails->raise();
    labelDetails->adjustSize();
    labelDetails->hide();
}

void MainWindow::updateDetails(AtomixInfo *info) {
    this->dInfo.pos = info->pos;
    this->dInfo.near = info->near;
    this->dInfo.far = info->far;
    this->dInfo.vertex = info->vertex;
    this->dInfo.data = info->data;
    this->dInfo.index = info->index;
    
    uint64_t total = dInfo.vertex + dInfo.data + dInfo.index;
    std::array<float, 4> bufs = { static_cast<float>(dInfo.vertex), static_cast<float>(dInfo.data), static_cast<float>(dInfo.index), static_cast<float>(total) };
    QList<QString> units = { "B", "KB", "MB", "GB" };
    std::array<int, 4> u = { 0, 0, 0, 0 };
    int div = 1024;
    
    uint64_t oneGiB = 1024 * 1024 * 1024;

    for (int idx = 0; auto& f : bufs) {
        while (f > div) {
            f /= div;
            u[idx]++;
        }
        idx++;
    }
    
    QString strDetails = QString("Position:      %1\n"\
                                 "View|Near:     %2\n"\
                                 "View|Far:      %3\n\n"\
                                 "Buffer|Vertex: %4 %7\n"\
                                 "Buffer|Data:   %5 %8\n"\
                                 "Buffer|Index:  %6 %9\n"\
                                 "Buffer|Total:  %10 %11\n"\
                                 ).arg(dInfo.pos).arg(dInfo.near).arg(dInfo.far)\
                                 .arg(bufs[0], 5, 'g', 4, ' ').arg(bufs[1], 5, 'g', 4, ' ').arg(bufs[2], 5, 'g', 4, ' ')\
                                 .arg(units[u[0]]).arg(units[u[1]]).arg(units[u[2]])\
                                 .arg(bufs[3], 5, 'g', 4, ' ').arg(units[u[3]]);
    labelDetails->setText(strDetails);
    labelDetails->adjustSize();
}

void MainWindow::loadConfig() {
    int files = cfgParser->cfgFiles.size();
    int comboID = comboConfigFile->currentData().toInt();
    AtomixConfig *cfg = nullptr;

    if (comboID <= files) {
        assert(!cfgParser->loadConfigFileGUI(cfgParser->cfgFiles[comboID - 1], &waveConfig));
        cfg = &waveConfig;
    } else if (comboID == files + 1) {
        // TODO handle this
        std::cout << "Invalid at this time." << std::endl;
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

    entryCloudLayers->setText(QString::number(cfg->cloudLayDivisor));
    entryCloudRes->setText(QString::number(cfg->cloudResolution));
    entryCloudMinRDP->setText(QString::number(cfg->cloudTolerance));
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
                                  /* "QLabel { font-size: 17px; }"\ */
                                  "QLabel#tabTitle { font-size: 23px; }"\
                                  "QTabBar::tab { height: 40px; width: %1 px; font-size: 15px; }"\
                                  "QTabBar::tab::selected { color: #9999FF; font-size: 19px; }"\
                                  "QTabBar::tab::!selected { color: #999999; background: #222222; }").arg(intTabWidth);
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
    QHBoxLayout *layColorPicker = new QHBoxLayout;

    QGroupBox *groupConfig = new QGroupBox("Config File");
    QGroupBox *groupOptions = new QGroupBox("Config Options");
    groupColors = new QGroupBox("Wave Colors");
    groupColors->setEnabled(false);
    groupOrbits = new QGroupBox("Visible Orbits");
    groupOrbits->setEnabled(false);

    QLabel *labelWaves = new QLabel("<p>Explore stable wave patterns in circular or spherical forms in this configuration</p>");
    labelWaves->setObjectName("tabTitle");
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

    QGridLayout *layWaveConfig = new QGridLayout;
    layWaveConfig->addWidget(labelOrbit, 0, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelAmp, 1, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelPeriod, 2, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelWavelength, 3, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelResolution, 4, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelOrthoPara, 5, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelSuper, 6, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelCPU, 7, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelSphere, 8, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelVertex, 9, 0, 1, 2, Qt::AlignLeft);
    layWaveConfig->addWidget(labelFrag, 10, 0, 1, 2, Qt::AlignLeft);

    layWaveConfig->addWidget(entryOrbit, 0, 3, 1, 2, Qt::AlignRight);
    layWaveConfig->addWidget(entryAmp, 1, 3, 1, 2, Qt::AlignRight);
    layWaveConfig->addWidget(entryPeriod, 2, 3, 1, 2, Qt::AlignRight);
    layWaveConfig->addWidget(entryWavelength, 3, 3, 1, 2, Qt::AlignRight);
    layWaveConfig->addWidget(entryResolution, 4, 3, 1, 2, Qt::AlignRight);

    layWaveConfig->addWidget(entryPara, 5, 3, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(entryOrtho, 5, 4, 1, 1, Qt::AlignLeft);

    layWaveConfig->addWidget(entrySuperOn, 6, 3, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(entrySuperOff, 6, 4, 1, 1, Qt::AlignLeft);

    layWaveConfig->addWidget(entryCPU, 7, 3, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(entryGPU, 7, 4, 1, 1, Qt::AlignLeft);

    layWaveConfig->addWidget(entrySphere, 8, 3, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(entryCircle, 8, 4, 1, 1, Qt::AlignLeft);

    layWaveConfig->addWidget(entryVertex, 9, 3, 1, 2, Qt::AlignRight);
    layWaveConfig->addWidget(entryFrag, 10, 3, 1, 2, Qt::AlignRight);

    layOptionBox->addLayout(layWaveConfig);

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

    groupConfig->setAlignment(Qt::AlignRight);
    groupOptions->setAlignment(Qt::AlignRight);

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
    QSizePolicy qPolicyExpand = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    wTabHarmonics = new QWidget(this);
    buttMorbHarmonics = new QPushButton("Render Cloud", this);
    buttMorbHarmonics->setEnabled(recipeLoaded);
    buttMorbHarmonics->setSizePolicy(qPolicyExpand);

    groupGenVertices = new QGroupBox();
    groupGenVertices->setAlignment(Qt::AlignRight);
    groupRecipeBuilder = new QGroupBox("Orbital Selector");
    groupRecipeReporter = new QGroupBox("Selected Orbitals");
    groupRecipeLocked = new QGroupBox("Locked Orbitals");

    QLabel *labelHarmonics = new QLabel("Generate accurate atomic orbital probability clouds for (<i>n</i>, <i>l</i>, <i>m<sub>l</sub></i>)");
    labelHarmonics->setObjectName("tabTitle");
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
    for (int n = 1; n <= MAX_ORBITS; n++) {
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
            for (int m_l = l; m_l >= 0; m_l--) {
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

    QLabel *labelCloudResolution = new QLabel("Points rendered per circle");
    QLabel *labelCloudLayers = new QLabel("Layers per step in radius");
    QLabel *labelMinRDP = new QLabel("Min probability per rendered point");
    entryCloudRes = new QLineEdit(QString::number(cloudConfig.cloudResolution));
    entryCloudLayers = new QLineEdit(QString::number(cloudConfig.cloudLayDivisor));
    entryCloudMinRDP = new QLineEdit(QString::number(cloudConfig.cloudTolerance));

    QGridLayout *layGenVertices = new QGridLayout;
    layGenVertices->addWidget(labelCloudLayers, 0, 0, 1, 1);
    layGenVertices->addWidget(labelCloudResolution, 1, 0, 1, 1);
    layGenVertices->addWidget(labelMinRDP, 2, 0, 1, 1);
    layGenVertices->addWidget(entryCloudLayers, 0, 1, 1, 1);
    layGenVertices->addWidget(entryCloudRes, 1, 1, 1, 1);
    layGenVertices->addWidget(entryCloudMinRDP, 2, 1, 1, 1);
    
    layGenVertices->setColumnStretch(0, 3);
    layGenVertices->setColumnStretch(1, 1);
    layGenVertices->setRowStretch(0, 1);
    layGenVertices->setRowStretch(1, 1);
    layGenVertices->setRowStretch(2, 1);

    groupGenVertices->setLayout(layGenVertices);
    groupGenVertices->setStyleSheet("QGroupBox { color: #FF7777; }");

    tableOrbitalReport = new QTableWidget();
    tableOrbitalReport->setColumnCount(2);
    QStringList headersReport = { "Weight", "Orbital" };
    tableOrbitalReport->setHorizontalHeaderLabels(headersReport);
    tableOrbitalReport->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tableOrbitalReport->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableOrbitalReport->verticalHeader()->setVisible(false);
    tableOrbitalReport->setShowGrid(false);
    tableOrbitalReport->setSortingEnabled(true);
    listOrbitalLocked = new QListWidget();

    QVBoxLayout *layRecipeBuilder = new QVBoxLayout;
    layRecipeBuilder->addWidget(treeOrbitalSelect);
    groupRecipeBuilder->setLayout(layRecipeBuilder);
    QVBoxLayout *layRecipeReporter = new QVBoxLayout;
    layRecipeReporter->addWidget(tableOrbitalReport);
    groupRecipeReporter->setLayout(layRecipeReporter);
    QVBoxLayout *layRecipeLocked = new QVBoxLayout;
    layRecipeLocked->addWidget(listOrbitalLocked);
    groupRecipeLocked->setLayout(layRecipeLocked);

    buttLockRecipes = new QPushButton("Lock Selection");
    buttLockRecipes->setEnabled(false);
    buttClearRecipes = new QPushButton("Clear Selection");
    buttClearRecipes->setEnabled(false);
    buttResetRecipes = new QPushButton("Clear Locked");
    buttResetRecipes->setEnabled(false);

    QVBoxLayout *layRecipeIO = new QVBoxLayout;
    QHBoxLayout *layHRecipeViews = new QHBoxLayout;
    QVBoxLayout *layVRecipeBuild = new QVBoxLayout;
    QVBoxLayout *layVRecipeChose = new QVBoxLayout;
    QHBoxLayout *layHRecipeButts = new QHBoxLayout;
    layVRecipeBuild->addWidget(groupRecipeBuilder);

    layVRecipeChose->addWidget(groupRecipeReporter);
    layVRecipeChose->addWidget(groupRecipeLocked);
    layVRecipeChose->setStretchFactor(groupRecipeReporter, 4);
    layVRecipeChose->setStretchFactor(groupRecipeLocked, 3);

    layHRecipeViews->addLayout(layVRecipeBuild);
    layHRecipeViews->addLayout(layVRecipeChose);

    layHRecipeButts->addWidget(buttLockRecipes);
    layHRecipeButts->addWidget(buttClearRecipes);
    layHRecipeButts->addWidget(buttResetRecipes);

    layRecipeIO->addLayout(layHRecipeViews);
    layRecipeIO->addLayout(layHRecipeButts);

    groupRecipeReporter->setAlignment(Qt::AlignRight);
    groupRecipeReporter->setStyleSheet("QGroupBox { color: #FF7777; }");
    groupRecipeReporter->setMaximumWidth(235);
    groupRecipeBuilder->setMaximumWidth(235);
    groupRecipeLocked->setMaximumWidth(235);
    groupRecipeLocked->setStyleSheet("QGroupBox { color: #FF7777; }");
    groupRecipeLocked->setAlignment(Qt::AlignRight);

    slideCullingX = new QSlider(Qt::Horizontal);
    slideCullingX->setMinimum(0);
    slideCullingX->setMaximum(intSliderLen);
    slideCullingX->setTickInterval(25);
    slideCullingX->setTickPosition(QSlider::TicksBelow);
    slideCullingY = new QSlider(Qt::Horizontal);
    slideCullingY->setMinimum(0);
    slideCullingY->setMaximum(intSliderLen);
    slideCullingY->setTickInterval(25);
    slideCullingY->setTickPosition(QSlider::TicksBelow);
    slideBackground = new QSlider(Qt::Horizontal);
    slideBackground->setMinimum(0);
    slideBackground->setMaximum(intSliderLen);
    slideBackground->setTickInterval(25);
    slideBackground->setTickPosition(QSlider::TicksBelow);

    QHBoxLayout *laySlideCulling = new QHBoxLayout;
    laySlideCulling->addWidget(slideCullingX);
    laySlideCulling->addWidget(slideCullingY);
    QHBoxLayout *laySlideBackground = new QHBoxLayout;
    laySlideBackground->addWidget(slideBackground);

    groupSlideCulling = new QGroupBox("Model Culling");
    groupSlideCulling->setLayout(laySlideCulling);
    groupSlideBackground = new QGroupBox("Background Brightness");
    groupSlideBackground->setLayout(laySlideBackground);

    QVBoxLayout *layDockHarmonics = new QVBoxLayout;
    layDockHarmonics->addWidget(labelHarmonics);
    layDockHarmonics->addStretch(1);
    layDockHarmonics->addLayout(layRecipeIO);
    layDockHarmonics->addWidget(groupGenVertices);
    layDockHarmonics->addWidget(buttMorbHarmonics);
    layDockHarmonics->addStretch(1);
    layDockHarmonics->addWidget(groupSlideCulling);
    layDockHarmonics->addWidget(groupSlideBackground);

    layDockHarmonics->setStretchFactor(labelHarmonics, 2);
    layDockHarmonics->setStretchFactor(layRecipeIO, 7);
    layDockHarmonics->setStretchFactor(groupGenVertices, 1);
    layDockHarmonics->setStretchFactor(buttMorbHarmonics, 1);
    layDockHarmonics->setStretchFactor(groupSlideCulling, 1);
    layDockHarmonics->setStretchFactor(groupSlideBackground, 1);

    buttMorbHarmonics->setSizePolicy(qPolicyExpand);

    wTabHarmonics->setLayout(layDockHarmonics);
    wTabHarmonics->setMinimumSize(intTabMinWidth,0);
}

void MainWindow::handleComboCfg() {
    this->loadConfig();
}

void MainWindow::handleConfigChanged() {
    if (listOrbitalLocked->count()) {
        buttMorbHarmonics->setEnabled(true);
    }
}

void MainWindow::handleDoubleClick(QTreeWidgetItem *item, int col) {
    Qt::CheckState checked = item->checkState(col);
    int itemChildren = item->childCount();
    
    /* Leaf Nodes */
    if (!itemChildren) {
        if (checked) {
            item->setCheckState(col, Qt::Unchecked);
        } else {
            item->setCheckState(col, Qt::Checked);
        }
    }
}

void MainWindow::handleRecipeCheck(QTreeWidgetItem *item, int col) {
    QTreeWidgetItem *ptrParent = item->parent();
    Qt::CheckState checked = item->checkState(col);
    int itemChildren = item->childCount();

    // Enable button(s) since a change has been made
    buttLockRecipes->setEnabled(true);
    buttClearRecipes->setEnabled(true);

    /* Parent nodes recurse to children while checking/unchecking */
    if (itemChildren) {
        for (int i = 0; i < item->childCount(); i++) {
            item->child(i)->setCheckState(0,checked);
        }

    /* Leaf nodes */
    } else {
        if (checked) {
            /* Leaf node checked */
            QString strItem = item->text(col);
            QTableWidgetItem *thisOrbital = new QTableWidgetItem(strItem);
            QTableWidgetItem *thisWeight = new QTableWidgetItem("1");
            int intTableRows = tableOrbitalReport->rowCount();
            tableOrbitalReport->setRowCount(intTableRows + 1);
            tableOrbitalReport->setItem(intTableRows, 1, thisOrbital);
            tableOrbitalReport->setItem(intTableRows, 0, thisWeight);
            thisOrbital->setTextAlignment(Qt::AlignCenter);
            thisOrbital->setForeground(Qt::red);
            thisWeight->setTextAlignment(Qt::AlignCenter);
            thisWeight->setForeground(Qt::yellow);
            thisOrbital->setFlags(Qt::ItemNeverHasChildren);
            thisWeight->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
        } else {
            /* Leaf node unchecked */
            int intItemRow = tableOrbitalReport->findItems(item->text(col), Qt::MatchExactly).first()->row();
            QTableWidgetItem *thisOrbital = tableOrbitalReport->item(intItemRow, 0);
            QTableWidgetItem *thisWeight = tableOrbitalReport->item(intItemRow, 1);
            tableOrbitalReport->removeRow(intItemRow);

            if (!tableOrbitalReport->rowCount()) {
                // Disable buttons if list is now empty
                buttLockRecipes->setEnabled(false);
                buttClearRecipes->setEnabled(true);
            }
        }
    }

    /* ALL Nodes make it here */
    // Since a change has been made here, set title to yellow to show unlocked recipes
    groupRecipeReporter->setStyleSheet("QGroupBox { color: #FFFF77; }");

    // If has parent and all siblings are now checked/unchecked, check/uncheck parent
    while (ptrParent) {
        int intSiblings = ptrParent->childCount();
        int intSiblingsSame = 0;
        bool siblingChecked;

        for (int i = 0; i < intSiblings; i++) {
            if (ptrParent->child(i)->checkState(col) == checked) {
                intSiblingsSame++;
            }
        }
        const QSignalBlocker blocker(treeOrbitalSelect);
        ptrParent->setCheckState(col, (intSiblingsSame == intSiblings) ? checked : Qt::PartiallyChecked);
        ptrParent = ptrParent->parent();
    }
}

void MainWindow::handleButtLockRecipes() {
    for (int i = 0; i < tableOrbitalReport->rowCount(); i++) {
        QTableWidgetItem *thisOrbital = tableOrbitalReport->item(i, 1);
        QTableWidgetItem *thisWeight = tableOrbitalReport->item(i, 0);
        thisOrbital->setForeground(Qt::white);
        thisWeight->setForeground(Qt::white);

        QString strOrbital = thisOrbital->text();
        QString strWeight = thisWeight->text();
        QString strLocked = strWeight + "  *  (" + strOrbital + ")";
        // QList<QListWidgetItem *> resultsExact = listOrbitalLocked->findItems(strLocked, Qt::MatchExactly);
        QList<QListWidgetItem *> resultsPartial = listOrbitalLocked->findItems(strOrbital, Qt::MatchContains);

        if (!resultsPartial.count()) {
            /* Not even a partial match found -- add new item to harmap */
            QListWidgetItem *newItem = new QListWidgetItem(strLocked, listOrbitalLocked);
            listOrbitalLocked->addItem(newItem);
            newItem->setTextAlignment(Qt::AlignRight);

            // Add item to harmap
            QStringList strlistItem = strOrbital.split(u' ');
            int n = strlistItem.at(0).toInt();
            int w = strWeight.toInt();
            std::vector<ivec3> *vecElem = &mapCloudRecipesLocked[n];
            ivec3 lm = ivec3(strlistItem.at(1).toInt(), strlistItem.at(2).toInt(), w);
            vecElem->push_back(lm);
            this->numRecipes++;
        } else {
            /* Partial match found -- update item in harmap */
            // Update match to new weight
            resultsPartial.first()->setText(strLocked);

            // Update harmap item
            QStringList strlistItem = strOrbital.split(u' ');
            int n = strlistItem.at(0).toInt();
            int l = strlistItem.at(1).toInt();
            int m = strlistItem.at(2).toInt();
            int w = strWeight.toInt();
            for (auto& vecElem : mapCloudRecipesLocked[n]) {
                if (vecElem.x == l && vecElem.y == m) {
                    vecElem.z = w;
                    break;
                }
            }
        }
    }
    buttLockRecipes->setEnabled(false);
    buttResetRecipes->setEnabled(true);
    buttMorbHarmonics->setEnabled(true);

    groupRecipeReporter->setStyleSheet("QGroupBox { color: #77FF77; }");
    groupRecipeLocked->setStyleSheet("QGroupBox { color: #77FF77; }");
}

void MainWindow::handleButtClearRecipes() {
    int topLevelItems = treeOrbitalSelect->topLevelItemCount();

    // Check, then uncheck all top-level items. A bit brutal, but very effective :)
    /* for (int i = 0; i < topLevelItems; i++) {
        treeOrbitalSelect->topLevelItem(i)->setCheckState(0, Qt::Checked);
        treeOrbitalSelect->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
    } */

    // More of a surgeon here...
    for (int i = 0; i < topLevelItems; i++) {
        QTreeWidgetItem *thisItem = treeOrbitalSelect->topLevelItem(i);
        Qt::CheckState itemChecked = thisItem->checkState(0);
        if (itemChecked == Qt::Checked || itemChecked == Qt::PartiallyChecked) {
            thisItem->setCheckState(0, Qt::Unchecked);
        }
    }
}

void MainWindow::handleButtResetRecipes() {
    listOrbitalLocked->clear();
    mapCloudRecipesLocked.clear();
    this->numRecipes = 0;

    groupRecipeLocked->setStyleSheet("QGroupBox { color: #FF7777; }");
    buttMorbHarmonics->setEnabled(false);
}

void MainWindow::handleButtMorbWaves() {
    waveConfig.waves = entryOrbit->text().toInt();
    waveConfig.amplitude = entryAmp->text().toDouble();
    waveConfig.period = entryPeriod->text().toDouble() * M_PI;
    waveConfig.wavelength = entryWavelength->text().toDouble() * M_PI;
    waveConfig.resolution = entryResolution->text().toInt();
    waveConfig.parallel = entryPara->isChecked();
    waveConfig.superposition = entrySuperOn->isChecked();
    waveConfig.cpu = entryCPU->isChecked();
    waveConfig.sphere = entrySphere->isChecked();
    waveConfig.vert = entryVertex->currentText().toStdString();
    waveConfig.frag = entryFrag->currentText().toStdString();

    refreshOrbits();
    graph->newWaveConfig(&waveConfig);

    groupColors->setEnabled(true);
    groupOrbits->setEnabled(true);
}

void MainWindow::handleButtMorbHarmonics() {
    cloudConfig.cloudLayDivisor = entryCloudLayers->text().toInt();
    cloudConfig.cloudResolution = entryCloudRes->text().toInt();
    cloudConfig.cloudTolerance = entryCloudMinRDP->text().toDouble();

    uint vertex, data, index;
    uint64_t total;
    graph->estimateSize(&cloudConfig, &mapCloudRecipesLocked, &vertex, &data, &index);
    total = vertex + data + index;
    uint64_t oneGiB = 1024 * 1024 * 1024;

    if (total > oneGiB) {
        std::array<float, 4> bufs = { static_cast<float>(vertex), static_cast<float>(data), static_cast<float>(index), static_cast<float>(total) };
        QList<QString> units = { " B", "KB", "MB", "GB" };
        std::array<int, 4> u = { 0, 0, 0, 0 };
        int div = 1024;

        for (int idx = 0; auto& f : bufs) {
            while (f > div) {
                f /= div;
                u[idx]++;
            }
            idx++;
        }

        QMessageBox dialogConfim(this);
        QString strDialogConfirm = QString("Estimated buffer sizes: \n"\
                                           "Vertex:        %1 %4\n"\
                                           "Data:          %2 %5\n"\
                                           "Index:         %3 %6\n\n"\
                                           "Total:         %7 %8"\
                                          ).arg(bufs[0], 5, 'g', 4, ' ').arg(bufs[1], 5, 'g', 4, ' ').arg(bufs[2], 5, 'g', 4, ' ')\
                                           .arg(units[u[0]]).arg(units[u[1]]).arg(units[u[2]])\
                                           .arg(bufs[3], 5, 'g', 4, ' ').arg(units[u[3]]);
        dialogConfim.setText(strDialogConfirm);
        dialogConfim.setFont(fontDebug);
        dialogConfim.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        dialogConfim.setDefaultButton(QMessageBox::Ok);
        if (dialogConfim.exec() == QMessageBox::Cancel) { return; }
    }

    graph->newCloudConfig(&cloudConfig, &this->mapCloudRecipesLocked, this->numRecipes);

    groupRecipeLocked->setStyleSheet("QGroupBox { color: #FFFF77; }");
    groupGenVertices->setStyleSheet("QGroupBox { color: #FFFF77; }");
    buttMorbHarmonics->setEnabled(false);
}

void MainWindow::handleWeightChange(int row, int col) {
    // Getting older sucks...
    buttLockRecipes->setEnabled(true);
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

void MainWindow::handleSlideCullingX(int val) {
    float pct = (static_cast<float>(val) / static_cast<float>(intSliderLen));
    this->cloudConfig.CloudCull_x = pct;

    if (graph->cullModel(pct, true, false)) {
        lastSliderSentX = pct;
    }
}

void MainWindow::handleSlideCullingY(int val) {
    float pct = (static_cast<float>(val) / static_cast<float>(intSliderLen));
    this->cloudConfig.CloudCull_y = pct;
    
    if (graph->cullModel(pct, false, false)) {
        lastSliderSentY = pct;
    }
}

void MainWindow::handleSlideReleased() {
    float pct = 0.0f;
    bool upToDateX = (slideCullingX->value() == lastSliderSentX);
    bool upToDateY = (slideCullingY->value() == lastSliderSentY);

    if (!upToDateX) {
        pct = this->cloudConfig.CloudCull_x;
        graph->cullModel(pct, true, true);
        lastSliderSentX = pct;
    }
    if (!upToDateY) {
        pct = this->cloudConfig.CloudCull_y;
        graph->cullModel(pct, false, true);
        lastSliderSentY = pct;
    }
}

void MainWindow::handleSlideBackground(int val) {
    graph->setBGColour((static_cast<float>(val) / intSliderLen));
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        close();
    } else if (e->key() == Qt::Key_D) {
        if (labelDetails->isVisible()) {
            labelDetails->hide();
        } else {
            labelDetails->show();
        }
    } else {
        QWidget::keyPressEvent(e);
    }
}

void MainWindow::printHarmap() {
    for (auto k : mapCloudRecipesLocked) {
        std::cout << k.first << ": ";
        for (auto v : k.second) {
            std::cout << glm::to_string(v) << ", ";
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

void MainWindow::printList() {
    int listSize = tableOrbitalReport->rowCount();
    for (int i = 0; i < listSize; i++) {
        QTableWidgetItem *item = tableOrbitalReport->item(i, 1);
        std::cout << item << ": " << item->text().toStdString() << "\n";
    }
    std::cout << std::endl;
}
