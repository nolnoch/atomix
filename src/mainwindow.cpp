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

int VK_MINOR_VERSION;
int VK_SPIRV_VERSION;


MainWindow::MainWindow() {
    // onAddNew();
}

void MainWindow::init(QRect &screenSize) {
    cfgParser = new ConfigParser;
    setWindowTitle(tr("atomix"));
    
    double windowRatio = 0.33;
    mw_width = SWIDTH + int((screenSize.width() - SWIDTH) * windowRatio);
    mw_height = SHEIGHT + int((screenSize.height() - SHEIGHT) * windowRatio);
    this->resize(mw_width, mw_height);
    this->move(screenSize.center() - this->frameGeometry().center());

    valIntSmall = new QIntValidator();  
    valIntSmall->setRange(1, 8);
    valIntLarge = new QIntValidator();
    valIntLarge->setRange(1, 999);
    valDoubleSmall = new QDoubleValidator();
    valDoubleSmall->setRange(0.0001, 0.9999, 4);
    valDoubleLarge = new QDoubleValidator();
    valDoubleLarge->setRange(0.001, 999.999, 3);

    lastSliderSentX = 0.0f;
    lastSliderSentY = 0.0f;

    setupStyleSheet();

    // Graphics setup
#ifdef USING_QVULKAN
    // Vulkan
    QByteArrayList layers = { "VK_LAYER_KHRONOS_validation" };
    QByteArrayList extensions = { "VK_KHR_get_physical_device_properties2", "VK_EXT_graphics_pipeline_library" };
    
    QVersionNumber version = vkInst.supportedApiVersion();
    VK_MINOR_VERSION = version.minorVersion();
    if (VK_MINOR_VERSION >= 3) {
        VK_SPIRV_VERSION = 6;
    } else if (VK_MINOR_VERSION == 2) {
        VK_SPIRV_VERSION = 5;
    } else if (VK_MINOR_VERSION == 1) {
        VK_SPIRV_VERSION = 3;
    } else {
        VK_SPIRV_VERSION = 0;
    }
    std::cout << "Vulkan API version: " << version.toString().toStdString() << std::endl;
    std::cout << "Vulkan SPIRV version: 1." << VK_SPIRV_VERSION << std::endl;
    
    vkInst.setApiVersion(version);
    vkInst.setLayers(layers);
    vkInst.setExtensions(extensions);
    if (!vkInst.create()) {
        qFatal("Failed to create Vulkan Instance: %d", vkInst.errorCode());
    }
    
    vkGraph = new VKWindow(this, cfgParser);
    vkGraph->setVulkanInstance(&vkInst);
    graph = QWidget::createWindowContainer(vkGraph);
    setCentralWidget(graph);
    this->setMaximumWidth(mw_width);
    graphWin = vkGraph;
#elifdef USING_QOPENGL
    // OpenGL
    glGraph = new GWidget(this, cfgParser);
    setCentralWidget(glGraph);
    graph = glGraph;
    graphWin = glGraph;
#endif

    // Setup Dock GUI
    setupTabs();
    this->addDockWidget(Qt::RightDockWidgetArea, dockTabs);
    refreshConfigs();
    refreshShaders();
    loadConfig();
    refreshOrbits();

    // Signal-Slot connections
    connect(vkGraph, SIGNAL(detailsChanged(AtomixInfo*)), this, SLOT(updateDetails(AtomixInfo*)));
    connect(vkGraph, SIGNAL(toggleLoading(bool)), this, SLOT(setLoading(bool)));
    connect(comboConfigFile, &QComboBox::activated, this, &MainWindow::handleComboCfg);
    connect(buttMorbWaves, &QPushButton::clicked, this, &MainWindow::handleButtMorbWaves);
#ifdef USING_QVULKAN
    connect(buttGroupOrbits, &QButtonGroup::idToggled, vkGraph, &VKWindow::selectRenderedWaves, Qt::DirectConnection);
#elifdef USING_QOPENGL
    connect(buttGroupOrbits, &QButtonGroup::idToggled, glGraph, &GWidget::selectRenderedWaves, Qt::DirectConnection);
#endif
    connect(buttGroupConfig, &QButtonGroup::idToggled, this, &MainWindow::handleButtConfig);
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
    connect(slideCullingX, &QSlider::valueChanged, this, &MainWindow::handleSlideCullingX);
    connect(slideCullingY, &QSlider::valueChanged, this, &MainWindow::handleSlideCullingY);
    connect(slideCullingX, &QSlider::sliderReleased, this, &MainWindow::handleSlideReleased);
    connect(slideCullingY, &QSlider::sliderReleased, this, &MainWindow::handleSlideReleased);
    connect(slideBackground, &QSlider::sliderMoved, this, &MainWindow::handleSlideBackground);

    // setupStyleSheet();
}

void MainWindow::postInit(int titlebarHeight) {
    wTabs->adjustSize();
    mw_titleHeight = titlebarHeight;

    QRect mwLoc = this->geometry();
    // mw_width = mwLoc.width();
    // mw_height = mwLoc.height() - titlebarHeight;
    mw_x = mwLoc.x();
    mw_y = mwLoc.y();

    QRect tabLoc = wTabs->geometry();
    int colWidth = (tabLoc.width() - 40) >> 2;
    tableOrbitalReport->setColumnWidth(0, colWidth);
    tableOrbitalReport->setColumnWidth(1, colWidth);

    QRect entryLoc = entryOrbit->geometry();
    int entryWidth = entryLoc.width();
    int entryHeight = entryLoc.height();

    slswPara->setMinimumHeight(entryHeight);
    slswSuper->setMinimumHeight(entryHeight);
    slswCPU->setMinimumHeight(entryHeight);
    slswSphere->setMinimumHeight(entryHeight);

    slswPara->setMaximumWidth(entryWidth);
    slswSuper->setMaximumWidth(entryWidth);
    slswCPU->setMaximumWidth(entryWidth);
    slswSphere->setMaximumWidth(entryWidth);

    slswPara->redraw();
    slswSuper->redraw();
    slswCPU->redraw();
    slswSphere->redraw();

    // TODO : Test on macOS and make dynamic
    // int dent = treeOrbitalSelect->indentation();
    // treeOrbitalSelect->setIndentation(15);

    setupDetails();
    setupLoading();
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
                                 .arg(bufs[0], 9, 'f', 2, ' ').arg(bufs[1], 9, 'f', 2, ' ').arg(bufs[2], 9, 'f', 2, ' ')\
                                 .arg(units[u[0]]).arg(units[u[1]]).arg(units[u[2]])\
                                 .arg(bufs[3], 9, 'f', 2, ' ').arg(units[u[3]]);
    labelDetails->setText(strDetails);
    labelDetails->adjustSize();
}

void MainWindow::setLoading(bool loading) {
    if (loading) {
        pbLoading->show();
    } else {
        pbLoading->hide();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
    // TODO : Refactor to switch-case
    if (e->key() == Qt::Key_Escape) {
        close();
    } else if (e->key() == Qt::Key_D) {
        if (labelDetails->isVisible()) {
            labelDetails->hide();
        } else {
            labelDetails->show();
        }
    } else if (e->key() == Qt::Key_P) {
        if (!vkGraph->supportsGrab()) {
            std::cout << "Grabbing not supported." << std::endl;
            return;
        }
        QImage image = vkGraph->grab();
        QFileDialog fd(this, "Save Image");
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.setDefaultSuffix("png");
        fd.selectFile("filename.png");
        if (fd.exec() == QDialog::Accepted) {
            image.save(fd.selectedFiles().first());
        }
    } else if (e->key() == Qt::Key_Home) {
        vkGraph->handleHome();
    } else if (e->key() == Qt::Key_Space) {
        vkGraph->handlePause();
    } else {
        QWidget::keyPressEvent(e);
    }

}

void MainWindow::setupTabs() {
    dockTabs = new QDockWidget(this);
    dockTabs->setContentsMargins(0, 0, 0, 0);
    wTabs = new QTabWidget(this);

    wTabs->setMaximumWidth(aStyle.dockWidth);
    wTabs->setContentsMargins(0, 0, 0, 0);

    setupDockWaves();
    setupDockHarmonics();
    wTabs->addTab(wTabWaves, tr("Waves"));
    wTabs->addTab(wTabHarmonics, tr("Harmonics"));
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

    QLabel *labelWaves = new QLabel("<p>Explore stable circular or spherical wave patterns</p>");
    labelWaves->setObjectName("tabDesc");
    labelWaves->setMaximumHeight(aStyle.tabLabelHeight);
    labelWaves->setMinimumHeight(aStyle.tabLabelHeight);
    labelWaves->setWordWrap(true);
    labelWaves->setFrameStyle(QFrame::Panel | QFrame::Raised);
    labelWaves->setLineWidth(aStyle.borderWidth);
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
    labelDebug = labelOrthoPara;
    labelOrthoPara->setObjectName("configLabel");
    QLabel *labelSuper = new QLabel("Superposition:");
    labelSuper->setObjectName("configLabel");
    QLabel *labelCPU = new QLabel("GPU vs CPU:");
    labelCPU->setObjectName("configLabel");
    QLabel *labelSphere = new QLabel("Circular vs Spherical:");
    labelSphere->setObjectName("configLabel");
    /* QLabel *labelVertex = new QLabel("Vertex Shader:");
    labelVertex->setObjectName("configLabel");
    QLabel *labelFrag = new QLabel("Fragment Shader:");
    labelFrag->setObjectName("configLabel"); */
    
    entryOrbit = new QLineEdit("4");
    entryOrbit->setValidator(valIntSmall);
    entryAmp = new QLineEdit("0.4");
    entryAmp->setValidator(valDoubleLarge);
    entryPeriod = new QLineEdit("1.0");
    entryPeriod->setValidator(valDoubleLarge);
    entryWavelength = new QLineEdit("2.0");
    entryWavelength->setValidator(valDoubleLarge);
    entryResolution = new QLineEdit("180");
    entryResolution->setValidator(valIntLarge);
    /* entryVertex = new QComboBox(this);
    entryFrag = new QComboBox(this); */

    entryOrbit->setAlignment(Qt::AlignRight);
    entryAmp->setAlignment(Qt::AlignRight);
    entryPeriod->setAlignment(Qt::AlignRight);
    entryWavelength->setAlignment(Qt::AlignRight);
    entryResolution->setAlignment(Qt::AlignRight);

    // TODO : This is being resized automatically now. Shouldn't be necessary.
    int entryHintWidth = 135;
    int entryHintHeight = 27;

    slswPara = new SlideSwitch("Para", "Ortho", entryHintWidth, entryHintHeight, this);
    slswSuper = new SlideSwitch("On", "Off", entryHintWidth, entryHintHeight, this);
    slswCPU = new SlideSwitch("CPU", "GPU", entryHintWidth, entryHintHeight, this);
    slswSphere = new SlideSwitch("Sphere", "Circle", entryHintWidth, entryHintHeight, this);

    slswPara->setChecked(false);
    slswSuper->setChecked(false);
    slswCPU->setChecked(false);
    slswSphere->setChecked(false);

    buttGroupConfig = new QButtonGroup();
    buttGroupConfig->setExclusive(false);
    buttGroupConfig->addButton(slswPara, 0);
    buttGroupConfig->addButton(slswSuper, 1);
    buttGroupConfig->addButton(slswCPU, 2);
    buttGroupConfig->addButton(slswSphere, 3);

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
    layoutDebug = layWaveConfig;
    layWaveConfig->addWidget(labelOrbit, 0, 0, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(labelAmp, 1, 0, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(labelPeriod, 2, 0, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(labelWavelength, 3, 0, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(labelResolution, 4, 0, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(labelOrthoPara, 5, 0, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(labelSuper, 6, 0, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(labelCPU, 7, 0, 1, 1, Qt::AlignLeft);
    layWaveConfig->addWidget(labelSphere, 8, 0, 1, 1, Qt::AlignLeft);

    layWaveConfig->addWidget(entryOrbit, 0, 2, 1, 1, Qt::AlignRight);
    layWaveConfig->addWidget(entryAmp, 1, 2, 1, 1, Qt::AlignRight);
    layWaveConfig->addWidget(entryPeriod, 2, 2, 1, 1, Qt::AlignRight);
    layWaveConfig->addWidget(entryWavelength, 3, 2, 1, 1, Qt::AlignRight);
    layWaveConfig->addWidget(entryResolution, 4, 2, 1, 1, Qt::AlignRight);

    layWaveConfig->addWidget(slswPara, 5, 2, 1, 1, Qt::AlignRight);
    layWaveConfig->addWidget(slswSuper, 6, 2, 1, 1, Qt::AlignRight);
    layWaveConfig->addWidget(slswCPU, 7, 2, 1, 1, Qt::AlignRight);
    layWaveConfig->addWidget(slswSphere, 8, 2, 1, 1, Qt::AlignRight);

    layWaveConfig->setColumnStretch(0,6);
    layWaveConfig->setColumnStretch(1,1);
    layWaveConfig->setColumnStretch(2,6);
    // layWaveConfig->setRowStretch(layWaveConfig->rowCount(),1);

    layOptionBox->addLayout(layWaveConfig);

    QPushButton *buttColorPeak = new QPushButton(" Peak");
    QPushButton *buttColorBase = new QPushButton(" Base");
    QPushButton *buttColorTrough = new QPushButton(" Trough");
    
    pmColour = new QPixmap(aStyle.baseFont, aStyle.baseFont);
    pmColour->fill(QColor::fromString("#FF00FF"));
    buttColorPeak->setIcon(QIcon(*pmColour));
    pmColour->fill(QColor::fromString("#0000FF"));
    buttColorBase->setIcon(QIcon(*pmColour));
    pmColour->fill(QColor::fromString("#00FFFF"));    
    buttColorTrough->setIcon(QIcon(*pmColour));

    buttGroupColors->addButton(buttColorPeak, 1);
    buttGroupColors->addButton(buttColorBase, 2);
    buttGroupColors->addButton(buttColorTrough, 3);

    layColorPicker->addWidget(buttColorPeak);
    layColorPicker->addWidget(buttColorBase);
    layColorPicker->addWidget(buttColorTrough);
    // layColorPicker->setContentsMargins(0, 0, 0, 0);
    // layColorPicker->setSpacing(5);

    groupConfig->setLayout(layConfigFile);
    groupOptions->setLayout(layOptionBox);
    groupColors->setLayout(layColorPicker);
    groupOrbits->setLayout(layOrbitSelect);

    groupConfig->setAlignment(Qt::AlignRight);
    groupOptions->setAlignment(Qt::AlignRight);

    layDock->addWidget(labelWaves);
    layDock->addStretch(1);
    layDock->addWidget(groupConfig);
    layDock->addWidget(groupOptions);
    layDock->addWidget(buttMorbWaves);
    layDock->addStretch(1);
    layDock->addWidget(groupColors);
    layDock->addWidget(groupOrbits);

    // layDock->setStretchFactor(labelWaves, 2);
    /* layDock->setStretchFactor(groupConfig, 1);
    layDock->setStretchFactor(groupOptions, 6);
    layDock->setStretchFactor(buttMorbWaves, 1);
    layDock->setStretchFactor(groupColors, 1);
    layDock->setStretchFactor(groupOrbits, 1); */

    buttMorbWaves->setSizePolicy(qPolicyExpand);

    layDock->setContentsMargins(0, 0, 0, 0);
    wTabWaves->setLayout(layDock);
    // wTabWaves->setMinimumWidth(intTabMinWidth);
    // wTabWaves->setMaximumWidth(intTabMaxWidth);
}

void MainWindow::setupDockHarmonics() {
    QSizePolicy qPolicyExpandA = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy qPolicyExpandH = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    wTabHarmonics = new QWidget(this);
    buttMorbHarmonics = new QPushButton("Render Cloud", this);
    buttMorbHarmonics->setEnabled(recipeLoaded);
    buttMorbHarmonics->setSizePolicy(qPolicyExpandA);

    groupGenVertices = new QGroupBox();
    groupGenVertices->setAlignment(Qt::AlignRight);
    groupRecipeBuilder = new QGroupBox("Orbital Selector");
    groupRecipeReporter = new QGroupBox("Selected Orbitals");
    groupRecipeLocked = new QGroupBox("Locked Orbitals");

    QLabel *labelHarmonics = new QLabel("Generate atomic orbital probability clouds for (<i>n</i>, <i>l</i>, <i>m<sub>l</sub></i>)");
    labelHarmonics->setObjectName("tabDesc");
    labelHarmonics->setMaximumHeight(aStyle.tabLabelHeight);
    labelHarmonics->setMinimumHeight(aStyle.tabLabelHeight);
    labelHarmonics->setWordWrap(true);
    labelHarmonics->setFrameStyle(QFrame::Panel | QFrame::Raised);
    labelHarmonics->setLineWidth(aStyle.borderWidth);
    labelHarmonics->setMargin(12);
    labelHarmonics->setAlignment(Qt::AlignCenter);

    treeOrbitalSelect = new QTreeWidget();
    treeOrbitalSelect->setSortingEnabled(true);
    QStringList strlistTreeHeaders = { "Orbital" };
    treeOrbitalSelect->setHeaderLabels(strlistTreeHeaders);
    treeOrbitalSelect->header()->setDefaultAlignment(Qt::AlignCenter);
    treeOrbitalSelect->setContentsMargins(0, 0, 0, 0);

    QTreeWidgetItem *lastN = nullptr;
    QTreeWidgetItem *lastL = nullptr;
    QTreeWidgetItem *thisItem = nullptr;
    for (int n = 1; n <= MAX_ORBITS; n++) {
        QStringList treeitemParentN = { QString("%1 _ _").arg(n), QString::number(n), QString("-"), QString("-") };
        thisItem = new QTreeWidgetItem(treeOrbitalSelect, treeitemParentN);
        thisItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        // thisItem->setBackground(0, QBrush(Qt::white));
        // thisItem->setTextAlignment(1, Qt::AlignCenter);
        // thisItem->setTextAlignment(2, Qt::AlignCenter);
        // thisItem->setTextAlignment(3, Qt::AlignCenter);
        thisItem->setCheckState(0, Qt::Unchecked);
        lastN = thisItem;
        for (int l = 0; l < n; l++) {
            QStringList treeitemParentL = { QString("%1 %2 _").arg(n).arg(l), QString::number(n), QString::number(l), QString("-") };
            thisItem = new QTreeWidgetItem(lastN, treeitemParentL);
            thisItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
            // thisItem->setTextAlignment(1, Qt::AlignCenter);
            // thisItem->setTextAlignment(2, Qt::AlignCenter);
            // thisItem->setTextAlignment(3, Qt::AlignCenter);
            thisItem->setCheckState(0, Qt::Unchecked);
            lastL = thisItem;
            for (int m_l = l; m_l >= 0; m_l--) {
                QStringList treeitemFinal = { QString("%1 %2 %3").arg(n).arg(l).arg(m_l), QString::number(n), QString::number(l), QString::number(m_l) };
                thisItem = new QTreeWidgetItem(lastL, treeitemFinal);
                thisItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
                // thisItem->setTextAlignment(1, Qt::AlignCenter);
                // thisItem->setTextAlignment(2, Qt::AlignCenter);
                // thisItem->setTextAlignment(3, Qt::AlignCenter);
                thisItem->setCheckState(0, Qt::Unchecked);
            }
        }
    }

    QLabel *labelCloudResolution = new QLabel("Points rendered per circle");
    QLabel *labelCloudLayers = new QLabel("Layers per step in radius");
    QLabel *labelMinRDP = new QLabel("Min probability per rendered point");
    entryCloudRes = new QLineEdit(QString::number(cloudConfig.cloudResolution));
    entryCloudRes->setValidator(valIntLarge);
    entryCloudRes->setAlignment(Qt::AlignRight);
    entryCloudLayers = new QLineEdit(QString::number(cloudConfig.cloudLayDivisor));
    entryCloudLayers->setValidator(valIntLarge);
    entryCloudLayers->setAlignment(Qt::AlignRight);
    entryCloudMinRDP = new QLineEdit(QString::number(cloudConfig.cloudTolerance));
    entryCloudMinRDP->setValidator(valDoubleSmall);
    entryCloudMinRDP->setAlignment(Qt::AlignRight);

    QGridLayout *layGenVertices = new QGridLayout;
    layGenVertices->addWidget(labelCloudLayers, 0, 0, 1, 1);
    layGenVertices->addWidget(labelCloudResolution, 1, 0, 1, 1);
    layGenVertices->addWidget(labelMinRDP, 2, 0, 1, 1);

    layGenVertices->addWidget(entryCloudLayers, 0, 2, 1, 1);
    layGenVertices->addWidget(entryCloudRes, 1, 2, 1, 1);
    layGenVertices->addWidget(entryCloudMinRDP, 2, 2, 1, 1);
    layGenVertices->setColumnStretch(0, 9);
    layGenVertices->setColumnStretch(1, 1);
    layGenVertices->setColumnStretch(2, 3);
    layGenVertices->setContentsMargins(5, 5, 5, 5);
    layGenVertices->setSpacing(0);

    groupGenVertices->setLayout(layGenVertices);
    groupGenVertices->setStyleSheet("QGroupBox { color: #FF7777; }");
    // groupGenVertices->setContentsMargins(0, 0, 0, 0);

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
    layRecipeBuilder->setContentsMargins(0, 0, 0, 0);
    groupRecipeBuilder->setLayout(layRecipeBuilder);
    // groupRecipeBuilder->setSizePolicy(qPolicyExpandA);
    QVBoxLayout *layRecipeReporter = new QVBoxLayout;
    layRecipeReporter->addWidget(tableOrbitalReport);
    layRecipeReporter->setContentsMargins(0, 0, 0, 0);
    groupRecipeReporter->setLayout(layRecipeReporter);
    QVBoxLayout *layRecipeLocked = new QVBoxLayout;
    layRecipeLocked->addWidget(listOrbitalLocked);
    layRecipeLocked->setContentsMargins(0, 0, 0, 0);
    groupRecipeLocked->setLayout(layRecipeLocked);

    QGridLayout *layOrbitalGrid = new QGridLayout;
    layOrbitalGrid->addWidget(groupRecipeBuilder, 0, 0, 7, 1);
    layOrbitalGrid->addWidget(groupRecipeReporter, 0, 1, 4, 1);
    layOrbitalGrid->addWidget(groupRecipeLocked, 4, 1, 3, 1);
    layOrbitalGrid->setContentsMargins(0, 0, 0, 0);
    layOrbitalGrid->setSpacing(0);

    buttLockRecipes = new QPushButton("Lock Selection");
    buttLockRecipes->setSizePolicy(qPolicyExpandH);
    buttLockRecipes->setEnabled(false);
    buttLockRecipes->setContentsMargins(0, 0, 0, 0);
    buttClearRecipes = new QPushButton("Clear Selection");
    buttClearRecipes->setSizePolicy(qPolicyExpandH);
    buttClearRecipes->setEnabled(false);
    buttResetRecipes = new QPushButton("Clear Locked");
    buttResetRecipes->setSizePolicy(qPolicyExpandH);
    buttResetRecipes->setEnabled(false);

    QHBoxLayout *layHRecipeButts = new QHBoxLayout;
    layHRecipeButts->addWidget(buttLockRecipes);
    layHRecipeButts->addWidget(buttClearRecipes);
    layHRecipeButts->addWidget(buttResetRecipes);
    layHRecipeButts->setContentsMargins(0, 0, 0, 0);
    layHRecipeButts->setSpacing(0);

    groupRecipeReporter->setAlignment(Qt::AlignRight);
    groupRecipeReporter->setMaximumWidth(aStyle.groupMaxWidth);
    groupRecipeReporter->setStyleSheet("QGroupBox { color: #FF7777 }");
    groupRecipeReporter->layout()->setContentsMargins(0, 0, 0, 0);
    groupRecipeBuilder->setAlignment(Qt::AlignLeft);
    groupRecipeBuilder->setMaximumWidth(aStyle.groupMaxWidth);
    groupRecipeBuilder->layout()->setContentsMargins(0, 0, 0, 0);
    groupRecipeLocked->setAlignment(Qt::AlignRight);
    groupRecipeLocked->setMaximumWidth(aStyle.groupMaxWidth);
    groupRecipeLocked->setStyleSheet("QGroupBox { color: #FF7777; }");
    groupRecipeLocked->layout()->setContentsMargins(0, 0, 0, 0);

    slideCullingX = new QSlider(Qt::Horizontal);
    slideCullingX->setMinimum(0);
    slideCullingX->setMaximum(aStyle.sliderTicks);
    slideCullingX->setTickInterval(aStyle.sliderInterval);
    slideCullingX->setTickPosition(QSlider::TicksAbove);
    slideCullingY = new QSlider(Qt::Horizontal);
    slideCullingY->setMinimum(0);
    slideCullingY->setMaximum(aStyle.sliderTicks);
    slideCullingY->setTickInterval(aStyle.sliderInterval);
    slideCullingY->setTickPosition(QSlider::TicksAbove);
    slideBackground = new QSlider(Qt::Horizontal);
    slideBackground->setMinimum(0);
    slideBackground->setMaximum(aStyle.sliderTicks);
    slideBackground->setTickInterval(aStyle.sliderInterval);
    slideBackground->setTickPosition(QSlider::TicksBelow);
    
    QHBoxLayout *layHCulling = new QHBoxLayout;
    layHCulling->addWidget(slideCullingX);
    layHCulling->setContentsMargins(0, 0, 0, 0);
    layHCulling->setSpacing(0);
    QHBoxLayout *layVCulling = new QHBoxLayout;
    layVCulling->addWidget(slideCullingY);
    layVCulling->setContentsMargins(0, 0, 0, 0);
    layVCulling->setSpacing(0);
    QHBoxLayout *laySlideBackground = new QHBoxLayout;
    laySlideBackground->addWidget(slideBackground);
    laySlideBackground->setContentsMargins(0, 0, 0, 0);
    laySlideBackground->setSpacing(0);

    groupHSlideCulling = new QGroupBox("Theta Culling");
    groupHSlideCulling->setLayout(layHCulling);
    groupHSlideCulling->setContentsMargins(0, 0, 0, 0);
    groupVSlideCulling = new QGroupBox("Phi Culling");
    groupVSlideCulling->setLayout(layVCulling);
    groupVSlideCulling->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *laySlideCulling = new QHBoxLayout;
    laySlideCulling->addWidget(groupHSlideCulling);
    laySlideCulling->addWidget(groupVSlideCulling);
    laySlideCulling->setContentsMargins(0, 0, 0, 0);
    laySlideCulling->setSpacing(0);
    groupSlideBackground = new QGroupBox("Background Brightness");
    groupSlideBackground->setLayout(laySlideBackground);

    QVBoxLayout *layDockHarmonics = new QVBoxLayout;
    layDockHarmonics->addWidget(labelHarmonics);
    layDockHarmonics->addStretch(1);
    layDockHarmonics->addLayout(layOrbitalGrid);
    layDockHarmonics->addLayout(layHRecipeButts);
    layDockHarmonics->addWidget(groupGenVertices);
    layDockHarmonics->addWidget(buttMorbHarmonics);
    layDockHarmonics->addStretch(1);
    layDockHarmonics->addLayout(laySlideCulling);
    layDockHarmonics->addWidget(groupSlideBackground);

    layDockHarmonics->setStretchFactor(labelHarmonics, 2);
    layDockHarmonics->setStretchFactor(layOrbitalGrid, 9);
    layDockHarmonics->setStretchFactor(layHRecipeButts, 1);
    layDockHarmonics->setStretchFactor(groupGenVertices, 1);
    layDockHarmonics->setStretchFactor(buttMorbHarmonics, 1);
    layDockHarmonics->setStretchFactor(laySlideCulling, 1);
    layDockHarmonics->setStretchFactor(groupSlideBackground, 1);
    layDockHarmonics->setSpacing(0);

    buttMorbHarmonics->setSizePolicy(qPolicyExpandA);

    layDockHarmonics->setContentsMargins(0, 0, 0, 0);
    wTabHarmonics->setLayout(layDockHarmonics);
    // wTabHarmonics->setMinimumWidth(intTabMinWidth);
    // wTabHarmonics->setMaximumWidth(intTabMaxWidth);
}

void MainWindow::setupStyleSheet() {
    aStyle.setWindowSize(mw_width, mw_height);
    aStyle.setDockWidth(mw_width * 0.2);
    aStyle.scaleFonts();
    aStyle.scaleWidgets();
    aStyle.scaleTabWidth(2);
    aStyle.updateStyleSheet();
    this->setStyleSheet(aStyle.getStyleSheet());
    
    // std::cout << "\nStyleSheet:\n" << aStyle.getStyleSheet().toStdString() << "\n" << std::endl;
}

void MainWindow::refreshConfigs() {
    int files = cfgParser->cfgFiles.size();
    std::string configPath = atomixFiles.configs();
    int rootLength = configPath.length();

    if (!files)
        files = cfgParser->findFiles(configPath, CFGEXT, &cfgParser->cfgFiles);
    assert(files);

    comboConfigFile->clear();
    for (int i = 0; i < files; i++) {
        comboConfigFile->addItem(QString::fromStdString(cfgParser->cfgFiles[i]).sliced(rootLength), i + 1);
    }
    comboConfigFile->addItem(tr("Custom"), files + 1);
    comboConfigFile->setCurrentText(DEFAULT);
}

void MainWindow::refreshShaders() {
    std::string shaderPath = atomixFiles.shaders();
    int rootLength = shaderPath.length();
    int files = 0;

    // Vertex Shaders
    files = cfgParser->vshFiles.size();
    if (!files)
        files = cfgParser->findFiles(shaderPath, VSHEXT, &cfgParser->vshFiles);
    assert(files);

    for (int i = 0; i < files; i++) {
        QString item = QString::fromStdString(cfgParser->vshFiles[i]).sliced(rootLength);
    }

    // Fragment Shaders
    files = cfgParser->fshFiles.size();
    if (!files)
        files = cfgParser->findFiles(shaderPath, FSHEXT, &cfgParser->fshFiles);
    assert(files);

    for (int i = 0; i < files; i++) {
        QString item = QString::fromStdString(cfgParser->fshFiles[i]).sliced(rootLength);
    }
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
    /* entryVertex->setCurrentText(QString::fromStdString(cfg->vert));
    entryFrag->setCurrentText(QString::fromStdString(cfg->frag)); */

    slswPara->setValue(cfg->parallel);
    slswSuper->setValue(cfg->superposition);
    slswCPU->setValue(cfg->cpu);
    slswSphere->setValue(cfg->sphere);

    entryCloudLayers->setText(QString::number(cfg->cloudLayDivisor));
    entryCloudRes->setText(QString::number(cfg->cloudResolution));
    entryCloudMinRDP->setText(QString::number(cfg->cloudTolerance));
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
    fontDebug.setStyleHint(QFont::Monospace);
    fontDebug.setFamily((isMacOS) ? "Monaco" : "Monospace");
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

    labelDetails->move(mw_x + 10, mw_y + 50);
    
    labelDetails->setAttribute(Qt::WA_NoSystemBackground);
    labelDetails->setAttribute(Qt::WA_TranslucentBackground);
    labelDetails->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::CoverWindow);
}

void MainWindow::setupLoading() {
    pbLoading = new QProgressBar(graph);
    pbLoading->setMinimum(0);
    pbLoading->setMaximum(0);
    pbLoading->setTextVisible(true);

    int lh = pbLoading->sizeHint().height();
    int gh = mw_y + mw_titleHeight + mw_height + 12;
    int gw = this->centralWidget()->width();
    pbLoading->resize(gw, lh);
    pbLoading->move(mw_x, gh - lh);

    pbLoading->setAttribute(Qt::WA_NoSystemBackground);
    pbLoading->setAttribute(Qt::WA_TranslucentBackground);
    pbLoading->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::CoverWindow);

    pbLoading->raise();
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

    // More of a surgeon here...
    for (int i = 0; i < topLevelItems; i++) {
        QTreeWidgetItem *thisItem = treeOrbitalSelect->topLevelItem(i);
        Qt::CheckState itemChecked = thisItem->checkState(0);
        if (itemChecked & (Qt::Checked | Qt::PartiallyChecked)) {
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
    buttLockRecipes->setEnabled(true);
}

void MainWindow::handleButtMorbWaves() {
    waveConfig.waves = std::clamp(entryOrbit->text().toInt(), 1, 8);
    waveConfig.amplitude = std::clamp(entryAmp->text().toDouble(), 0.001, 999.999);
    waveConfig.period = std::clamp(entryPeriod->text().toDouble(), 0.001, 999.999) * M_PI;
    waveConfig.wavelength = std::clamp(entryWavelength->text().toDouble(), 0.001, 999.999) * M_PI;
    waveConfig.resolution = std::clamp(entryResolution->text().toInt(), 1, 999);
    waveConfig.parallel = slswPara->value();
    waveConfig.superposition = slswSuper->value();
    waveConfig.cpu = slswCPU->value();
    waveConfig.sphere = slswSphere->value();
    /* waveConfig.vert = entryVertex->currentText().toStdString();
    waveConfig.frag = entryFrag->currentText().toStdString(); */

    refreshOrbits();

#ifdef USING_QVULKAN
    vkGraph->newWaveConfig(&waveConfig);
#elifdef USING_QOPENGL
    glGraph->newWaveConfig(&waveConfig);
#endif

    groupColors->setEnabled(true);
    groupOrbits->setEnabled(true);
    if (listOrbitalLocked->count()) {
        buttMorbHarmonics->setEnabled(true);
    }
}

void MainWindow::handleButtMorbHarmonics() {
    cloudConfig.cloudLayDivisor = entryCloudLayers->text().toInt();
    cloudConfig.cloudResolution = entryCloudRes->text().toInt();
    cloudConfig.cloudTolerance = entryCloudMinRDP->text().toFloat();

    uint vertex, data, index;
    uint64_t total;
#ifdef USING_QVULKAN
    vkGraph->estimateSize(&cloudConfig, &mapCloudRecipesLocked, &vertex, &data, &index);
#elifdef USING_QOPENGL
    glGraph->estimateSize(&cloudConfig, &mapCloudRecipesLocked, &vertex, &data, &index);
#endif
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
                                          ).arg(bufs[0], 9, 'f', 2, ' ').arg(bufs[1], 9, 'f', 2, ' ').arg(bufs[2], 9, 'f', 2, ' ')\
                                           .arg(units[u[0]]).arg(units[u[1]]).arg(units[u[2]])\
                                           .arg(bufs[3], 9, 'f', 2, ' ').arg(units[u[3]]);
        dialogConfim.setText(strDialogConfirm);
        dialogConfim.setFont(fontDebug);
        dialogConfim.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        dialogConfim.setDefaultButton(QMessageBox::Ok);
        if (dialogConfim.exec() == QMessageBox::Cancel) { return; }
    }

#ifdef USING_QVULKAN
    vkGraph->newCloudConfig(&this->cloudConfig, &this->mapCloudRecipesLocked, this->numRecipes, true);
#elifdef USING_QOPENGL
    glGraph->newCloudConfig(&this->cloudConfig, &this->mapCloudRecipesLocked, this->numRecipes, true);
#endif

    groupRecipeLocked->setStyleSheet("QGroupBox { color: #FFFF77; }");
    groupGenVertices->setStyleSheet("QGroupBox { color: #FFFF77; }");
    buttMorbHarmonics->setEnabled(false);
}

void MainWindow::handleWeightChange([[maybe_unused]] int row, [[maybe_unused]] int col) {
    // Getting older sucks...
    buttLockRecipes->setEnabled(true);
}

void MainWindow::handleButtConfig(int id, bool checked) {
    enum CfgButt { PARA = 0, SUPER  = 1, CPU = 2, SPHERE = 3 };

    if (checked) {
        switch (id) {
            case PARA:
                // Parallel waves
                break;
            case SUPER:
                // Superposition
                buttGroupConfig->button(PARA)->setChecked(true);
                buttGroupConfig->button(CPU)->setChecked(true);
                break;
            case CPU:
                // CPU rendering
                break;
            case SPHERE:
                // Spherical wave pattern
                buttGroupConfig->button(PARA)->setChecked(true);
                break;
            default:
                break;
        }
    } else {
        switch (id) {
            case PARA:
                // Orthogonal waves
                buttGroupConfig->button(SUPER)->setChecked(false);
                buttGroupConfig->button(SPHERE)->setChecked(false);
                break;
            case SUPER:
                // No superposition
                break;
            case CPU:
                // GPU rendering
                buttGroupConfig->button(SUPER)->setChecked(false);
                break;
            case SPHERE:
                // Circular wave pattern
                break;
            default:
                break;
        }
    }
}

void MainWindow::handleButtColors(int id) {
    QColorDialog::ColorDialogOptions colOpts = QFlag(QColorDialog::ShowAlphaChannel);
    QColor colorChoice = QColorDialog::getColor(Qt::white, this, tr("Choose a Color"), colOpts);
    uint colour = 0;

    int dRed = colorChoice.red();
    colour |= dRed;
    colour <<= 8;
    int dGreen = colorChoice.green();
    colour |= dGreen;
    colour <<= 8;
    int dBlue = colorChoice.blue();
    colour |= dBlue;
    colour <<= 8;
    int dAlpha = colorChoice.alpha();
    colour |= dAlpha;

    QString colourHex = "#" + QString::fromStdString(std::format("{:08X}", colour));
    pmColour->fill(QColor::fromString(colourHex));
    buttGroupColors->button(id)->setIcon(QIcon(*pmColour));

#ifdef USING_QVULKAN
    vkGraph->setColorsWaves(id, colour);
#elifdef USING_QOPENGL
    glGraph->setColorsWaves(id, colour);
#endif
}

void MainWindow::handleSlideCullingX(int val) {
    float pct = (static_cast<float>(val) / static_cast<float>(aStyle.sliderTicks));
    this->cloudConfig.CloudCull_x = pct;
    // slideCullingX->setFocus();
}

void MainWindow::handleSlideCullingY(int val) {
    float pct = (static_cast<float>(val) / static_cast<float>(aStyle.sliderTicks));
    this->cloudConfig.CloudCull_y = pct;
    // slideCullingY->setFocus();
}

void MainWindow::handleSlideReleased() {
    if ((this->cloudConfig.CloudCull_x != lastSliderSentX) || (this->cloudConfig.CloudCull_y != lastSliderSentY)) {
#ifdef USING_QVULKAN
        vkGraph->newCloudConfig(&this->cloudConfig, &this->mapCloudRecipesLocked, this->numRecipes, false);
#elifdef USING_QOPENGL
        glGraph->newCloudConfig(&this->cloudConfig, &this->mapCloudRecipesLocked, this->numRecipes, false);
#endif
        lastSliderSentX = this->cloudConfig.CloudCull_x;
        lastSliderSentY = this->cloudConfig.CloudCull_y;
    }
    // slideCullingX->clearFocus();
    // slideCullingY->clearFocus();
}

void MainWindow::handleSlideBackground(int val) {
#ifdef USING_QVULKAN
    vkGraph->setBGColour((static_cast<float>(val) / static_cast<float>(aStyle.sliderTicks)));
#elifdef USING_QOPENGL
    glGraph->setBGColour((static_cast<float>(val) / static_cast<float>(aStyle.sliderTicks)));
#endif
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
