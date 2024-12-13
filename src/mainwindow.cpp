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

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QHeaderView>
#include <QFontDatabase>
#include <QSignalBlocker>
#include <QtQml/QJSEngine>

#include "mainwindow.hpp"

int VK_MINOR_VERSION;
int VK_SPIRV_VERSION;


MainWindow::MainWindow() {
    fileHandler = new FileHandler;
}

void MainWindow::init(QRect &screenSize) {
    aStyle.qtStyle = this->style()->name();
    setWindowTitle(tr("atomix"));
    fileHandler->findFiles();
    
    // Window Size and Position on Screen
    double windowRatio = 0.3333333333333333;
    mw_width = SWIDTH + int((screenSize.width() - SWIDTH) * windowRatio);
    mw_height = SHEIGHT + int((screenSize.height() - SHEIGHT) * windowRatio);
    this->resize(mw_width, mw_height);
    this->move(screenSize.center() - this->frameGeometry().center());

    _initStyle();
    _initGraphics();
    _initWidgets();
    _connectSignals();
}

void MainWindow::postInit() {
    mw_graphHeight = vkGraph->height();
    mw_graphWidth = vkGraph->width();

    if (loadGeometry) loadSavedSettings();
    _dockResize();

    wTabs->installEventFilter(this);
    showReady();
}

void MainWindow::updateDetails(AtomixInfo *info) {
    this->dInfo.pos = info->pos;
    this->dInfo.near = info->near;
    this->dInfo.far = info->far;
    this->dInfo.vertex = info->vertex;
    this->dInfo.data = info->data;
    this->dInfo.index = info->index;
    uint64_t total = dInfo.vertex + dInfo.data + dInfo.index;
    
    // Simple subroutine to convert bytes to human readable units. This routine is not human-readable.
    std::array<float, 4> bufs = { static_cast<float>(dInfo.vertex), static_cast<float>(dInfo.data), static_cast<float>(dInfo.index), static_cast<float>(total) };
    QStringList units = { " B", "KB", "MB", "GB" };
    std::array<int, 4> u = { 0, 0, 0, 0 };
    int div = 1024;
    for (int idx = 0; auto& f : bufs) {
        while (f > div) {
            f /= div;
            u[idx]++;
        }
        idx++;
    }
    
    QString strDetails = QString("Distance:  %1 | Near:      %2 | Far:       %3 |\n"
                                 "Vertex: %4 %7 | Data:   %5 %8 | Index:  %6 %9 | Total:  %10 %11"
                                 ).arg(dInfo.pos, 9, 'f', 2, ' ').arg(dInfo.near, 9, 'f', 2, ' ').arg(dInfo.far, 9, 'f', 2, ' ')\
                                 .arg(bufs[0], 9, 'f', 2, ' ').arg(bufs[1], 9, 'f', 2, ' ').arg(bufs[2], 9, 'f', 2, ' ')\
                                 .arg(units[u[0]]).arg(units[u[1]]).arg(units[u[2]])\
                                 .arg(bufs[3], 9, 'f', 2, ' ').arg(units[u[3]]);
    labelDetails->setText(strDetails);
    labelDetails->adjustSize();
}

void MainWindow::setupStatusBar() {
    statBar = this->statusBar();
    statBar->setObjectName("statusBar");
    statBar->setFont(aStyle.fontMonoStatus);
    statBar->setMinimumHeight(aStyle.loadingHeight);
}

void MainWindow::setupDetails() {
    labelDetails = new QLabel(this);
    labelDetails->setObjectName("labelDetails");
    labelDetails->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    labelDetails->hide();
}

void MainWindow::setupLoading() {
    pbLoading = new QProgressBar(this);
    pbLoading->setMinimum(0);
    pbLoading->setMaximum(0);
    pbLoading->setTextVisible(false);
    pbLoading->hide();
    aStyle.loadingHeight = pbLoading->sizeHint().height();
}

void MainWindow::showLoading(bool loading) {
    if (this->isLoading == loading) return;
    this->isLoading = loading;
    
    if (loading) {
        statBar->addWidget(pbLoading, 1);
        pbLoading->show();
    } else {
        statBar->removeWidget(pbLoading);
    }
}

void MainWindow::showDetails() {
    showDebug = !showDebug;
    if (showDebug) {
        statBar->addPermanentWidget(labelDetails, 0);
        labelDetails->show();
        statBar->setMinimumHeight(aStyle.detailsHeight);
    } else {
        statBar->removeWidget(labelDetails);
        statBar->setMinimumHeight(aStyle.loadingHeight);
    }
}

void MainWindow::showReady() {
    statBar->showMessage(tr("Ready"));
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
    switch (e->key()) {
        case Qt::Key_Escape:
            close();
            break;
        case Qt::Key_D:
            showDetails();
            break;
        case Qt::Key_P: {
            if (!vkGraph->supportsGrab()) {
                std::cout << "Grabbing not supported." << std::endl;
                break;
            }
            QImage image = vkGraph->grab();
            QFileDialog fd(this, "Save Image");
            fd.setAcceptMode(QFileDialog::AcceptSave);
            fd.setDefaultSuffix("png");
            fd.selectFile("filename.png");
            if (fd.exec() == QDialog::Accepted) {
                image.save(fd.selectedFiles().first());
            }
            break;
        }
        case Qt::Key_Home:
            vkGraph->handleHome();
            break;
        case Qt::Key_Space:
            vkGraph->handlePause();
            break;
        default:
            QWidget::keyPressEvent(e);
            break;
    }
}

void MainWindow::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    
    // Update Window Geometry (in case of resize)
    QRect mwLoc = this->geometry();
    mw_width = mwLoc.width();
    mw_height = mwLoc.height();
    mw_x = mwLoc.x();
    mw_y = mwLoc.y();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if ((obj == wTabs) && (event->type() == QEvent::Resize)) {
        _dockResize();
        return true;
    }

    return false;
}

void MainWindow::closeEvent(QCloseEvent *e) {
    QSettings settings("Nolnoch", "atomix");
    settings.setValue("geometry", this->saveGeometry());
    settings.setValue("state", this->saveState());
    QWidget::closeEvent(e);
}

void MainWindow::setupTabs() {
    dockTabs = new QDockWidget(this);
    dockTabs->setObjectName("dockTabs");
    dockTabs->setAllowedAreas(Qt::RightDockWidgetArea);
    
    wTabs = new QTabWidget(this);
    wTabs->setObjectName("tabsAtomix");

    setupDockWaves();
    setupDockHarmonics();

    wTabs->addTab(wTabWaves, tr("Waves"));
    wTabs->addTab(wTabHarmonics, tr("Harmonics"));
    dockTabs->setWidget(wTabs);
    this->addDockWidget(Qt::RightDockWidgetArea, dockTabs);
    mw_tabCount = wTabs->count();

    buttGroupConfig = new QButtonGroup(this);
    buttGroupConfig->setExclusive(false);
    buttGroupConfig->addButton(buttDeleteCloudConfig, 0);
    buttGroupConfig->addButton(buttDeleteWaveConfig, 1);
    buttGroupConfig->addButton(buttSaveCloudConfig, 2);
    buttGroupConfig->addButton(buttSaveWaveConfig, 3);
}

void MainWindow::setupDockWaves() {
    QSizePolicy qPolicyExpandV = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    wTabWaves = new QWidget(this);

    // Buttons
    buttMorbWaves = new QPushButton("Render Waves", this);
    buttMorbWaves->setObjectName("morb");
    buttMorbWaves->setSizePolicy(qPolicyExpandV);
    buttGroupColors = new QButtonGroup(this);

    // Groups
    QGroupBox *groupWaveConfig = new QGroupBox("Config File", this);
    groupWaveConfig->setObjectName("groupWaveConfig");
    groupOptions = new QGroupBox("Config Options", this);
    groupOptions->setObjectName("groupOptions");
    groupColors = new QGroupBox("Wave Colors", this);
    groupColors->setObjectName("groupColors");
    groupColors->setEnabled(false);
    groupOrbits = new QGroupBox("Visible Waves", this);
    groupOrbits->setObjectName("groupOrbits");
    groupOrbits->setEnabled(false);

    // Tab Description Label
    labelWaves = new QLabel("<p>Explore stable circular or spherical wave patterns</p>", this);
    labelWaves->setObjectName("tabDesc");
    labelWaves->setFixedHeight(aStyle.labelDescHeight);
    labelWaves->setWordWrap(true);
    labelWaves->setFrameStyle(QFrame::Panel | QFrame::Raised);
    labelWaves->setLineWidth(aStyle.borderWidth);
    labelWaves->setMargin(aStyle.spaceM);
    labelWaves->setAlignment(Qt::AlignCenter);

    // Config Selection Box
    comboWaveConfigFile = new QComboBox(this);
    comboWaveConfigFile->setObjectName("comboWaveConfigFile");
    buttDeleteWaveConfig = new QPushButton("-", this);
    buttDeleteWaveConfig->setObjectName("buttDeleteWaveConfig");
    buttDeleteWaveConfig->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    buttDeleteWaveConfig->setMaximumWidth(aStyle.fontAtomixWidth << 1);
    buttDeleteWaveConfig->setContentsMargins(0, 0, 0, 0);
    buttSaveWaveConfig = new QPushButton("+", this);
    buttSaveWaveConfig->setObjectName("buttSaveWaveConfig");
    buttSaveWaveConfig->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    buttSaveWaveConfig->setMaximumWidth(aStyle.fontAtomixWidth << 1);
    buttSaveWaveConfig->setContentsMargins(0, 0, 0, 0);
    buttSaveWaveConfig->setEnabled(false);
    layWaveConfigFile = new QHBoxLayout;
    layWaveConfigFile->addWidget(comboWaveConfigFile, 8);
    layWaveConfigFile->addWidget(buttDeleteWaveConfig, 1);
    layWaveConfigFile->addWidget(buttSaveWaveConfig, 1);
    layWaveConfigFile->setContentsMargins(aStyle.spaceS, aStyle.spaceS, aStyle.spaceS, aStyle.spaceS);
    layWaveConfigFile->setSpacing(aStyle.spaceS);
    groupWaveConfig->setLayout(layWaveConfigFile);
    groupWaveConfig->setAlignment(Qt::AlignRight);
    
    // LineEdits (entries)
    entryOrbit = new QLineEdit("4");
    entryOrbit->setObjectName("entryOrbit");
    entryOrbit->setValidator(valIntSmall);
    entryAmp = new QLineEdit("0.4");
    entryAmp->setObjectName("entryAmp");
    entryAmp->setValidator(valDoubleLarge);
    entryPeriod = new QLineEdit("1.0");
    entryPeriod->setObjectName("entryPeriod");
    entryWavelength = new QLineEdit("2.0");
    entryWavelength->setObjectName("entryWavelength");
    entryResolution = new QLineEdit("180");
    entryResolution->setObjectName("entryResolution");
    entryResolution->setValidator(valIntLarge);
    entryOrbit->setAlignment(Qt::AlignRight);
    entryAmp->setAlignment(Qt::AlignRight);
    entryPeriod->setAlignment(Qt::AlignRight);
    entryWavelength->setAlignment(Qt::AlignRight);
    entryResolution->setAlignment(Qt::AlignRight);

    // SlideSwitches (toggles)
    int switchWidth = 100;
    int switchHeight = 20;
    slswPara = new SlideSwitch("Para", "Ortho", switchWidth, switchHeight, this);
    slswSuper = new SlideSwitch("On", "Off", switchWidth, switchHeight, this);
    slswCPU = new SlideSwitch("CPU", "GPU", switchWidth, switchHeight, this);
    slswSphere = new SlideSwitch("Sphere", "Circle", switchWidth, switchHeight, this);
    slswPara->setObjectName("slswPara");
    slswSuper->setObjectName("slswSuper");
    slswCPU->setObjectName("slswCPU");
    slswSphere->setObjectName("slswSphere");
    slswPara->setChecked(false);
    slswSuper->setChecked(false);
    slswCPU->setChecked(false);
    slswSphere->setChecked(false);

    // Assign switches to button group
    buttGroupSwitch = new QButtonGroup(this);
    buttGroupSwitch->setExclusive(false);
    buttGroupSwitch->addButton(slswPara, 0);
    buttGroupSwitch->addButton(slswSuper, 1);
    buttGroupSwitch->addButton(slswCPU, 2);
    buttGroupSwitch->addButton(slswSphere, 3);

    // Wave Configuration Layout
    layWaveConfig = new QFormLayout;
    layWaveConfig->addRow("Number of waves:", entryOrbit);
    layWaveConfig->addRow("Amplitude:", entryAmp);
    layWaveConfig->addRow("Period:", entryPeriod);
    layWaveConfig->addRow("Wavelength:", entryWavelength);
    layWaveConfig->addRow("Resolution:", entryResolution);
    layWaveConfig->addRow("Orthogonal/Parallel:", slswPara);
    layWaveConfig->addRow("Superposition:", slswSuper);
    layWaveConfig->addRow("CPU/GPU:", slswCPU);
    layWaveConfig->addRow("Sphere/Circle:", slswSphere);
    layWaveConfig->setRowWrapPolicy(QFormLayout::DontWrapRows);
    layWaveConfig->setHorizontalSpacing(aStyle.spaceL);
    layWaveConfig->setVerticalSpacing(aStyle.spaceM);
    layWaveConfig->setLabelAlignment(Qt::AlignRight);
    layWaveConfig->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    layWaveConfig->setFormAlignment(Qt::AlignCenter);
    groupOptions->setLayout(layWaveConfig);
    groupOptions->setAlignment(Qt::AlignRight);

    // Color Picker Buttons
    QPushButton *buttColorPeak = new QPushButton(" Peak", this);
    buttColorPeak->setObjectName("buttColorPeak");
    QPushButton *buttColorBase = new QPushButton(" Base", this);
    buttColorBase->setObjectName("buttColorBase");
    QPushButton *buttColorTrough = new QPushButton(" Trough", this);
    buttColorTrough->setObjectName("buttColorTrough");
    
    // Generate Starting Colours (via Icons via Pixmap)
    pmColour = new QPixmap(aStyle.baseFontSize, aStyle.baseFontSize);
    pmColour->fill(QColor::fromString("#FF00FF"));
    buttColorPeak->setIcon(QIcon(*pmColour));
    pmColour->fill(QColor::fromString("#0000FF"));
    buttColorBase->setIcon(QIcon(*pmColour));
    pmColour->fill(QColor::fromString("#00FFFF"));
    buttColorTrough->setIcon(QIcon(*pmColour));

    // Assign buttons to button group
    buttGroupColors->addButton(buttColorPeak, 1);
    buttGroupColors->addButton(buttColorBase, 2);
    buttGroupColors->addButton(buttColorTrough, 3);

    // Color Picker Group (via Layout)
    layColorPicker = new QHBoxLayout;
    layColorPicker->addWidget(buttColorPeak);
    layColorPicker->addWidget(buttColorBase);
    layColorPicker->addWidget(buttColorTrough);
    layColorPicker->setContentsMargins(aStyle.spaceS, aStyle.spaceS, aStyle.spaceS, aStyle.spaceS);
    layColorPicker->setSpacing(aStyle.spaceS);
    groupColors->setLayout(layColorPicker);

    // Wave Visibility Selection
    QCheckBox *orbit1 = new QCheckBox(this);
    QCheckBox *orbit2 = new QCheckBox(this);
    QCheckBox *orbit3 = new QCheckBox(this);
    QCheckBox *orbit4 = new QCheckBox(this);
    QCheckBox *orbit5 = new QCheckBox(this);
    QCheckBox *orbit6 = new QCheckBox(this);
    QCheckBox *orbit7 = new QCheckBox(this);
    QCheckBox *orbit8 = new QCheckBox(this);
    orbit1->setObjectName("orbit1");
    orbit2->setObjectName("orbit2");
    orbit3->setObjectName("orbit3");
    orbit4->setObjectName("orbit4");
    orbit5->setObjectName("orbit5");
    orbit6->setObjectName("orbit6");
    orbit7->setObjectName("orbit7");
    orbit8->setObjectName("orbit8");
    layOrbitSelect = new QGridLayout;
    layOrbitSelect->addWidget(orbit1, 0, 0, Qt::AlignCenter);
    layOrbitSelect->addWidget(orbit2, 0, 1, Qt::AlignCenter);
    layOrbitSelect->addWidget(orbit3, 0, 2, Qt::AlignCenter);
    layOrbitSelect->addWidget(orbit4, 0, 3, Qt::AlignCenter);
    layOrbitSelect->addWidget(orbit5, 0, 4, Qt::AlignCenter);
    layOrbitSelect->addWidget(orbit6, 0, 5, Qt::AlignCenter);
    layOrbitSelect->addWidget(orbit7, 0, 6, Qt::AlignCenter);
    layOrbitSelect->addWidget(orbit8, 0, 7, Qt::AlignCenter);
    layOrbitSelect->addWidget(new QLabel("1", this), 1, 0, Qt::AlignCenter);
    layOrbitSelect->addWidget(new QLabel("2", this), 1, 1, Qt::AlignCenter);
    layOrbitSelect->addWidget(new QLabel("3", this), 1, 2, Qt::AlignCenter);
    layOrbitSelect->addWidget(new QLabel("4", this), 1, 3, Qt::AlignCenter);
    layOrbitSelect->addWidget(new QLabel("5", this), 1, 4, Qt::AlignCenter);
    layOrbitSelect->addWidget(new QLabel("6", this), 1, 5, Qt::AlignCenter);
    layOrbitSelect->addWidget(new QLabel("7", this), 1, 6, Qt::AlignCenter);
    layOrbitSelect->addWidget(new QLabel("8", this), 1, 7, Qt::AlignCenter);
    layOrbitSelect->setContentsMargins(0, 0, 0, 0);
    layOrbitSelect->setSpacing(aStyle.spaceS);
    groupOrbits->setLayout(layOrbitSelect);

    // Assign checkboxes to button group
    buttGroupOrbits = new QButtonGroup(this);
    buttGroupOrbits->setExclusive(false);
    buttGroupOrbits->addButton(orbit1, 1);
    buttGroupOrbits->addButton(orbit2, 2);
    buttGroupOrbits->addButton(orbit3, 4);
    buttGroupOrbits->addButton(orbit4, 8);
    buttGroupOrbits->addButton(orbit5, 16);
    buttGroupOrbits->addButton(orbit6, 32);
    buttGroupOrbits->addButton(orbit7, 64);
    buttGroupOrbits->addButton(orbit8, 128);

    // Add All Groups and Layouts to Main Tab Layout
    layDockWaves = new QVBoxLayout;
    layDockWaves->addWidget(labelWaves);
    layDockWaves->addStretch(1);
    layDockWaves->addWidget(groupWaveConfig);
    layDockWaves->addWidget(groupOptions);
    layDockWaves->addWidget(buttMorbWaves);
    layDockWaves->addStretch(1);
    layDockWaves->addWidget(groupColors);
    layDockWaves->addWidget(groupOrbits);

    // Set Main Tab Layout
    layDockWaves->setContentsMargins(aStyle.spaceM, aStyle.spaceM, aStyle.spaceM, aStyle.spaceM);
    wTabWaves->setLayout(layDockWaves);
}

void MainWindow::setupDockHarmonics() {
    QSizePolicy qPolicyExpandA = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    wTabHarmonics = new QWidget(this);
    
    // Buttons
    buttMorbHarmonics = new QPushButton("Render Cloud", this);
    buttMorbHarmonics->setObjectName("morb");
    buttMorbHarmonics->setEnabled(false);
    buttMorbHarmonics->setSizePolicy(qPolicyExpandA);
    buttMorbHarmonics->setAutoDefault(true);
    buttClearHarmonics = new QPushButton("Clear", this);
    buttClearHarmonics->setSizePolicy(qPolicyExpandA);
    buttClearHarmonics->setEnabled(false);
    buttClearHarmonics->setObjectName("buttClearHarmonics");

    // Groups
    QGroupBox *groupCloudConfig = new QGroupBox("Config File", this);
    groupCloudConfig->setObjectName("groupCloudConfig");
    groupRecipeBuilder = new QGroupBox("Orbital Selector", this);
    groupRecipeBuilder->setObjectName("groupRecipeBuilder");
    groupRecipeReporter = new QGroupBox("Selected Orbitals", this);
    groupRecipeReporter->setObjectName("groupRecipeReporter");
    groupGenVertices = new QGroupBox(this);
    groupGenVertices->setObjectName("groupGenVertices");

    // Tab Description Label
    labelHarmonics = new QLabel("Generate atomic orbital probability clouds for (<i>n</i>, <i>l</i>, <i>m<sub>l</sub></i>)", this);
    labelHarmonics->setObjectName("tabDesc");
    labelHarmonics->setFixedHeight(aStyle.labelDescHeight);
    labelHarmonics->setWordWrap(true);
    labelHarmonics->setFrameStyle(QFrame::Panel | QFrame::Raised);
    labelHarmonics->setLineWidth(aStyle.borderWidth);
    labelHarmonics->setMargin(aStyle.spaceM);
    labelHarmonics->setAlignment(Qt::AlignCenter);

    // Config Selection Box
    comboCloudConfigFile = new QComboBox(this);
    comboCloudConfigFile->setObjectName("comboCloudConfigFile");
    buttDeleteCloudConfig = new QPushButton("-", this);
    buttDeleteCloudConfig->setObjectName("buttDeleteCloudConfig");
    buttDeleteCloudConfig->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    buttDeleteCloudConfig->setMaximumWidth(aStyle.fontAtomixWidth << 1);
    buttDeleteCloudConfig->setContentsMargins(0, 0, 0, 0);
    buttDeleteCloudConfig->setEnabled(false);
    buttSaveCloudConfig = new QPushButton("+", this);
    buttSaveCloudConfig->setObjectName("buttSaveCloudConfig");
    buttSaveCloudConfig->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    buttSaveCloudConfig->setMaximumWidth(aStyle.fontAtomixWidth << 1);
    buttSaveCloudConfig->setContentsMargins(0, 0, 0, 0);
    buttSaveCloudConfig->setEnabled(false);
    layCloudConfigFile = new QHBoxLayout;
    layCloudConfigFile->addWidget(comboCloudConfigFile, 8);
    layCloudConfigFile->addWidget(buttDeleteCloudConfig, 1);
    layCloudConfigFile->addWidget(buttSaveCloudConfig, 1);
    layCloudConfigFile->setContentsMargins(aStyle.spaceS, aStyle.spaceS, aStyle.spaceS, aStyle.spaceS);
    layCloudConfigFile->setSpacing(aStyle.spaceS);
    groupCloudConfig->setLayout(layCloudConfigFile);
    groupCloudConfig->setAlignment(Qt::AlignRight);

    // Orbital Selection Tree
    treeOrbitalSelect = new QTreeWidget(this);
    treeOrbitalSelect->setObjectName("treeOrbitalSelect");
    treeOrbitalSelect->setColumnCount(1);
    
    SortableOrbitalTr *lastN = nullptr;
    SortableOrbitalTr *lastL = nullptr;
    SortableOrbitalTr *thisItem = nullptr;
    for (int n = 1; n <= MAX_ORBITS; n++) {
        QString strParentN = QString("%1 _ _").arg(n);
        thisItem = new SortableOrbitalTr(treeOrbitalSelect);
        thisItem->setText(0, strParentN);
        thisItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        thisItem->setCheckState(0, Qt::Unchecked);
        lastN = thisItem;
        for (int l = 0; l < n; l++) {
            QString strParentL = QString("%1 %2 _").arg(n).arg(l);
            thisItem = new SortableOrbitalTr(lastN);
            thisItem->setText(0, strParentL);
            thisItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
            thisItem->setCheckState(0, Qt::Unchecked);
            lastL = thisItem;
            for (int m_l = l; m_l >= -l; m_l--) {
                QString strFinal = QString("%1 %2 %3").arg(n).arg(l).arg(((m_l > 0) ? QString("+") : QString("")) + QString::number(m_l));
                thisItem = new SortableOrbitalTr(lastL);
                thisItem->setText(0, strFinal);
                thisItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
                thisItem->setCheckState(0, Qt::Unchecked);
            }
        }
    }
    treeOrbitalSelect->setSortingEnabled(true);
    treeOrbitalSelect->setHeaderLabels({ "Orbital" });
    treeOrbitalSelect->header()->setDefaultAlignment(Qt::AlignCenter);
    treeOrbitalSelect->header()->setSectionResizeMode(QHeaderView::Stretch);
    treeOrbitalSelect->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    treeOrbitalSelect->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    treeOrbitalSelect->setSizePolicy(qPolicyExpandA);

    // Selected Orbital Reporting Table
    tableOrbitalReport = new QTableWidget(this);
    tableOrbitalReport->setObjectName("tableOrbitalReport");
    tableOrbitalReport->setSizePolicy(qPolicyExpandA);
    tableOrbitalReport->setColumnCount(2);
    tableOrbitalReport->setHorizontalHeaderLabels({ "Weight", "Orbital" });
    tableOrbitalReport->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableOrbitalReport->verticalHeader()->setDefaultSectionSize(aStyle.tableFontSize);
    tableOrbitalReport->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableOrbitalReport->verticalHeader()->setVisible(false);
    tableOrbitalReport->setShowGrid(false);
    tableOrbitalReport->sortByColumn(1, Qt::SortOrder::DescendingOrder);
    tableOrbitalReport->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    tableOrbitalReport->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Add Orbital Selection Widgets to Groups (via Layouts)
    QVBoxLayout *layRecipeBuilder = new QVBoxLayout;
    layRecipeBuilder->addWidget(treeOrbitalSelect);
    layRecipeBuilder->setContentsMargins(0, 0, 0, 0);
    groupRecipeBuilder->setLayout(layRecipeBuilder);
    QVBoxLayout *layRecipeReporter = new QVBoxLayout;
    layRecipeReporter->addWidget(tableOrbitalReport);
    layRecipeReporter->setContentsMargins(0, 0, 0, 0);
    groupRecipeReporter->setLayout(layRecipeReporter);

    // Configure Orbital Selection Groups
    groupRecipeBuilder->setAlignment(Qt::AlignLeft);
    groupRecipeBuilder->setSizePolicy(qPolicyExpandA);
    groupRecipeReporter->setAlignment(Qt::AlignRight);
    groupRecipeReporter->setSizePolicy(qPolicyExpandA);
    groupRecipeReporter->setStyleSheet("QGroupBox { color: #FF7777 }");

    // Add Orbital Selection Groups to HBox Layout
    QHBoxLayout *layHOrbital = new QHBoxLayout;
    layHOrbital->addWidget(groupRecipeBuilder, 1);
    layHOrbital->addWidget(groupRecipeReporter, 1);
    layHOrbital->setSpacing(0);

    // Harmonics Configuration Input Widgets
    entryCloudRes = new QLineEdit(QString::number(mw_cloudConfig.cloudResolution));
    entryCloudRes->setObjectName("entryCloudRes");
    entryCloudRes->setValidator(valIntLarge);
    entryCloudRes->setAlignment(Qt::AlignRight);
    entryCloudLayers = new QLineEdit(QString::number(mw_cloudConfig.cloudLayDivisor));
    entryCloudLayers->setObjectName("entryCloudLayers");
    entryCloudLayers->setValidator(valIntLarge);
    entryCloudLayers->setAlignment(Qt::AlignRight);
    entryCloudMinRDP = new QLineEdit(QString::number(mw_cloudConfig.cloudTolerance));
    entryCloudMinRDP->setObjectName("entryCloudMinRDP");
    entryCloudMinRDP->setValidator(valDoubleSmall);
    entryCloudMinRDP->setAlignment(Qt::AlignRight);

    // Add Harmonics Configuration Widgets to Group (via Layouts)
    layGenVertices = new QFormLayout;
    layGenVertices->addRow("Point resolution:", entryCloudRes);
    layGenVertices->addRow("Layer resolution:", entryCloudLayers);
    layGenVertices->addRow("Minimum probability:", entryCloudMinRDP);
    layGenVertices->setLabelAlignment(Qt::AlignRight);
    layGenVertices->setRowWrapPolicy(QFormLayout::DontWrapRows);
    layGenVertices->setHorizontalSpacing(aStyle.spaceL);
    layGenVertices->setVerticalSpacing(aStyle.spaceM);
    layGenVertices->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    layGenVertices->setFormAlignment(Qt::AlignCenter);
    layGenVertices->setContentsMargins(aStyle.spaceS, aStyle.spaceS, aStyle.spaceS, aStyle.spaceS);
    groupGenVertices->setLayout(layGenVertices);
    groupGenVertices->setStyleSheet("QGroupBox { color: #FF7777; }");
    groupGenVertices->setAlignment(Qt::AlignCenter);

    // Add Render and Clear Buttons to Layout
    QHBoxLayout *layHarmButts = new QHBoxLayout;
    layHarmButts->addWidget(buttMorbHarmonics);
    layHarmButts->addWidget(buttClearHarmonics);
    layHarmButts->setSpacing(aStyle.spaceS);
    layHarmButts->setStretch(0, 3);
    layHarmButts->setStretch(1, 1);

    // Culling Sliders
    slideCullingX = new QSlider(Qt::Horizontal, this);
    slideCullingX->setObjectName("slideCullingX");
    slideCullingX->setMinimum(0);
    slideCullingX->setMaximum(aStyle.sliderTicks);
    slideCullingX->setTickInterval(aStyle.sliderInterval);
    slideCullingX->setTickPosition(QSlider::TicksAbove);
    slideCullingY = new QSlider(Qt::Horizontal, this);
    slideCullingY->setObjectName("slideCullingY");
    slideCullingY->setMinimum(0);
    slideCullingY->setMaximum(aStyle.sliderTicks);
    slideCullingY->setTickInterval(aStyle.sliderInterval);
    slideCullingY->setTickPosition(QSlider::TicksAbove);
    slideCullingR = new BiSlider(Qt::Horizontal, this);
    slideCullingR->setObjectName("slideCullingR");
    slideCullingR->setMinimum(-int(aStyle.sliderTicks));
    slideCullingR->setMaximum(+int(aStyle.sliderTicks));
    slideCullingR->setTickInterval(aStyle.sliderInterval);
    slideCullingR->setTickPosition(QSlider::TicksBelow);
    slideCullingR->setValue(0);
    slideBackground = new QSlider(Qt::Horizontal, this);
    slideBackground->setObjectName("slideBackground");
    slideBackground->setMinimum(0);
    slideBackground->setMaximum(aStyle.sliderTicks);
    slideBackground->setTickInterval(aStyle.sliderInterval);
    slideBackground->setTickPosition(QSlider::TicksBelow);
    
    // Add Culling Sliders to Groups (via Layouts)
    QHBoxLayout *layHCulling = new QHBoxLayout;
    layHCulling->addWidget(slideCullingX);
    layHCulling->setContentsMargins(0, 0, 0, 0);
    layHCulling->setSpacing(0);
    QHBoxLayout *layVCulling = new QHBoxLayout;
    layVCulling->addWidget(slideCullingY);
    layVCulling->setContentsMargins(0, 0, 0, 0);
    layVCulling->setSpacing(0);
    QHBoxLayout *layRCulling = new QHBoxLayout;
    layRCulling->addWidget(slideCullingR);
    layRCulling->setContentsMargins(0, 0, 0, 0);
    layRCulling->setSpacing(0);
    QHBoxLayout *laySlideBackground = new QHBoxLayout;
    laySlideBackground->addWidget(slideBackground);
    laySlideBackground->setContentsMargins(0, 0, 0, 0);
    laySlideBackground->setSpacing(0);

    groupHSlideCulling = new QGroupBox("Phi Culling", this);
    groupHSlideCulling->setObjectName("groupHSlideCulling");
    groupHSlideCulling->setLayout(layHCulling);
    groupHSlideCulling->setContentsMargins(0, 0, 0, 0);
    groupHSlideCulling->setEnabled(false);
    groupVSlideCulling = new QGroupBox("Theta Culling", this);
    groupVSlideCulling->setObjectName("groupVSlideCulling");
    groupVSlideCulling->setLayout(layVCulling);
    groupVSlideCulling->setContentsMargins(0, 0, 0, 0);
    groupVSlideCulling->setEnabled(false);
    QHBoxLayout *laySlideCulling = new QHBoxLayout;
    laySlideCulling->addWidget(groupHSlideCulling, 1);
    laySlideCulling->addWidget(groupVSlideCulling, 1);
    laySlideCulling->setContentsMargins(0, 0, 0, 0);
    laySlideCulling->setSpacing(0);
    
    groupRSlideCulling = new QGroupBox("Radial Culling", this);
    groupRSlideCulling->setObjectName("groupRSlideCulling");
    groupRSlideCulling->setLayout(layRCulling);
    groupRSlideCulling->setContentsMargins(0, 0, 0, 0);
    groupRSlideCulling->setEnabled(false);
    groupSlideBackground = new QGroupBox("Background", this);
    groupSlideBackground->setObjectName("groupSlideBackground");
    groupSlideBackground->setLayout(laySlideBackground);
    groupSlideBackground->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *laySlideRadialBG = new QHBoxLayout;
    laySlideRadialBG->addWidget(groupRSlideCulling, 1);
    laySlideRadialBG->addWidget(groupSlideBackground, 1);
    laySlideRadialBG->setContentsMargins(0, 0, 0, 0);
    laySlideRadialBG->setSpacing(0);

    // Add All Groups and Layouts to Main Tab Layout
    layDockHarmonics = new QVBoxLayout;
    layDockHarmonics->addWidget(labelHarmonics);
    layDockHarmonics->addStretch(1);
    layDockHarmonics->addWidget(groupCloudConfig);
    layDockHarmonics->addLayout(layHOrbital, 8);
    layDockHarmonics->addWidget(groupGenVertices);
    layDockHarmonics->addLayout(layHarmButts);
    layDockHarmonics->addStretch(1);
    layDockHarmonics->addLayout(laySlideCulling);
    layDockHarmonics->addLayout(laySlideRadialBG);

    // Set Main Tab Layout
    layDockHarmonics->setContentsMargins(aStyle.spaceM, aStyle.spaceM, aStyle.spaceM, aStyle.spaceM);
    wTabHarmonics->setLayout(layDockHarmonics);
}

void MainWindow::refreshShaders() {
    fileHandler->findFiles();

    // Vertex Shaders
    int files = fileHandler->getVertexShadersCount();
    auto pathLength = fileHandler->atomixFiles.shaders().length();
    assert(files);

    QStringList vshFiles = fileHandler->getVertexShadersList();

    for (int i = 0; i < files; i++) {
        QString item = vshFiles[i].sliced(pathLength);
    }

    // Fragment Shaders
    files = fileHandler->getFragmentShadersCount();
    pathLength = fileHandler->atomixFiles.shaders().length();
    assert(files);

    QStringList fshFiles = fileHandler->getFragmentShadersList();

    for (int i = 0; i < files; i++) {
        QString item = fshFiles[i].sliced(pathLength);
    }
}

void MainWindow::refreshConfigs(BitFlag target, QString selection) {
    fileHandler->findFiles();
    int waveFiles = fileHandler->getWaveFilesCount();
    int cloudFiles = fileHandler->getCloudFilesCount();
    QString path = QString::fromStdString(fileHandler->atomixFiles.configs());
    int pathLength = path.length();
    
    // Wave Config Combo Box
    if (waveFiles && target.hasAny(mw::WAVE)) {
        QStringList cfgFiles = fileHandler->getWaveFilesList();

        comboWaveConfigFile->clear();
        for (int i = 0; i < waveFiles; i++) {
            comboWaveConfigFile->addItem(cfgFiles[i].sliced(pathLength), i + 1);
        }
        comboWaveConfigFile->addItem(CUSTOM, waveFiles + 1);
        
        if (cfgFiles.contains(path + selection)) {
            comboWaveConfigFile->setCurrentText(selection);
        } else if (cfgFiles.contains(path + DEFAULT)) {
            comboWaveConfigFile->setCurrentText(DEFAULT);
        } else {
            comboWaveConfigFile->setCurrentIndex(waveFiles);
        }

        loadWaveConfig();
    } else {
        comboWaveConfigFile->clear();
    }

    // Cloud Config Combo Box
    if (cloudFiles && target.hasAny(mw::CLOUD)) {
        QStringList cfgFiles = fileHandler->getCloudFilesList();

        comboCloudConfigFile->clear();
        comboCloudConfigFile->addItem(SELECT, 1);
        for (int i = 0; i < cloudFiles; i++) {
            comboCloudConfigFile->addItem(cfgFiles[i].sliced(pathLength), i + 2);
        }

        if (cfgFiles.contains(path + selection)) {
            comboCloudConfigFile->setCurrentText(selection);
        } else {
            comboCloudConfigFile->setCurrentText(SELECT);
        }
    } else {
        comboCloudConfigFile->clear();
    }
}

void MainWindow::loadWaveConfig() {
    int files = fileHandler->getWaveFilesCount();
    int comboID = comboWaveConfigFile->currentIndex();
    AtomixWaveConfig cfg;

    if (comboID < files) {
        SuperConfig waveConfig;
        waveConfig = fileHandler->loadConfigFile(fileHandler->getWaveFilesList()[comboID]);
        if (std::holds_alternative<AtomixWaveConfig>(waveConfig)) {
            cfg = std::get<AtomixWaveConfig>(waveConfig);
        } else {
            assert("\"Should never get here.\" loadWaveConfig::(!std::holds_alternative<AtomixWaveConfig>(waveConfig))");
        }
    } else if (comboID > files) {
        assert("\"Should never get here.\" loadWaveConfig::(comboID > files)");
    } else {
        // Custom
        return;
    }

    notDefaultConfig = (comboWaveConfigFile->currentText() != DEFAULT);
    buttDeleteWaveConfig->setEnabled((notDefaultConfig && comboWaveConfigFile->currentText() != CUSTOM));
    buttSaveWaveConfig->setEnabled(false);
    refreshWaveConfigGUI(cfg);
}

void MainWindow::refreshWaveConfigGUI(AtomixWaveConfig &cfg) {
    entryOrbit->setText(QString::number(cfg.waves));
    entryAmp->setText(QString::number(cfg.amplitude));
    entryPeriod->setText(QString::number(cfg.period));
    entryWavelength->setText(QString::number(cfg.wavelength));
    entryResolution->setText(QString::number(cfg.resolution));

    slswPara->setValue(cfg.parallel);
    slswSuper->setValue(cfg.superposition);
    slswCPU->setValue(cfg.cpu);
    slswSphere->setValue(cfg.sphere);
}

void MainWindow::loadCloudConfig() {
    int files = fileHandler->getCloudFilesCount();
    int comboID = comboCloudConfigFile->currentIndex();

    if (!comboID) {
        return;
    } else {
        comboID--;
    }

    this->handleButtClearRecipes();

    AtomixCloudConfig cfg;
    if (comboID <= files) {
        SuperConfig cloudConfig;
        cloudConfig = fileHandler->loadConfigFile(fileHandler->getCloudFilesList()[comboID], &mapCloudRecipes);
        if (std::holds_alternative<AtomixCloudConfig>(cloudConfig)) {
            cfg = std::get<AtomixCloudConfig>(cloudConfig);
        } else {
            assert("\"Should never get here.\" loadCloudConfig::(!std::holds_alternative<AtomixCloudConfig>(cloudConfig))");
        }
    } else if (comboID > files) {
        assert("\"Should never get here.\" loadCloudConfig::(comboID > files)");
    } else {
        return;
    }

    refreshCloudConfigGUI(cfg);
    buttDeleteCloudConfig->setEnabled(true);
    buttSaveCloudConfig->setEnabled(false);
    comboCloudConfigFile->setCurrentIndex(++comboID);
}

void MainWindow::refreshCloudConfigGUI(AtomixCloudConfig &cfg) {
    entryCloudLayers->setText(QString::number(cfg.cloudLayDivisor));
    entryCloudRes->setText(QString::number(cfg.cloudResolution));
    entryCloudMinRDP->setText(QString::number(cfg.cloudTolerance));

    for (auto [key, vec] : mapCloudRecipes) {
        for (auto v : vec) {
            QString strOrbital = QString("%1 %2 %3%4").arg(key).arg(v.x).arg((v.y > 0) ? "+" : "").arg(v.y);
            treeOrbitalSelect->findItems(strOrbital, Qt::MatchFixedString | Qt::MatchRecursive, 0).at(0)->setCheckState(0, Qt::Checked);
        }
    }
}

uint MainWindow::refreshOrbits(std::pair<int, int> waveChange) {
    const QSignalBlocker blocker(buttGroupOrbits);
    ushort renderedOrbits = 0;

    if (activeModel) {
        for (auto &button : buttGroupOrbits->buttons()) {
            if (button->isChecked()) {
                renderedOrbits |= buttGroupOrbits->id(button);
            }
        }
    } else {
        for (int i = 0; i < mw_waveConfig.waves; i++) {
            renderedOrbits |= (1 << i);
        }
    }
    
    int incr = ((0 < waveChange.second) - (waveChange.second < 0));
    if (incr != 0) {
        bool neg = (incr < 0);
        int start = waveChange.first + (neg ? incr : 0);
        int end = start + waveChange.second;
        for (int i = start; i != end; i += incr) {
            if (neg) {
                renderedOrbits &= ~(1 << i);
            } else {
                renderedOrbits |= (1 << i);
            }
        }
    }

    for (int i = 0; i < MAX_ORBITS; i++) {
        uint checkID = 1 << i;
        QAbstractButton *checkBox = buttGroupOrbits->button(checkID);
        bool enabled = (i < mw_waveConfig.waves);
        bool checkState = (renderedOrbits & checkID);

        checkBox->setEnabled(enabled);
        checkBox->setVisible(enabled);
        checkBox->setChecked(checkState);
    }

    return renderedOrbits;
}

void MainWindow::loadSavedSettings() {
    QSettings settings("Nolnoch", "atomix");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
}

void MainWindow::handleWaveConfigChanged() {
    comboWaveConfigFile->setCurrentIndex(comboWaveConfigFile->count() - 1);
    notDefaultConfig = true;
}

void MainWindow::handleCloudConfigChanged() {
    if (numRecipes) {
        buttMorbHarmonics->setEnabled(true);
    }
    comboCloudConfigFile->setCurrentText(SELECT);
    buttDeleteCloudConfig->setEnabled(false);
}

void MainWindow::handleTreeDoubleClick(QTreeWidgetItem *item, int col) {
    Qt::CheckState checked = item->checkState(col);
    int itemChildren = item->childCount();
    
    /* Leaf Nodes */
    if (!itemChildren) {
        item->setCheckState(col, (checked) ? Qt::Unchecked : Qt::Checked);
    }
}

void MainWindow::handleTableDoubleClick(int row, int col) {
    if (col != 1) {
        return;
    }

    QString strOrbital = tableOrbitalReport->item(row, 1)->text();
    treeOrbitalSelect->findItems(strOrbital, Qt::MatchFixedString | Qt::MatchRecursive, 0).at(0)->setCheckState(0, Qt::Unchecked);
}

void MainWindow::handleRecipeCheck(QTreeWidgetItem *item, int col) {
    tableOrbitalReport->setSortingEnabled(false);
    const QSignalBlocker blocker(tableOrbitalReport);

    SortableOrbitalTr *ptrParent = (SortableOrbitalTr *)item->parent();
    Qt::CheckState checked = item->checkState(col);
    int itemChildren = item->childCount();

    if (itemChildren) {
        // Parent nodes recurse to children while checking/unchecking
        for (int i = 0; i < item->childCount(); i++) {
            item->child(i)->setCheckState(0,checked);
        }

    } else {
        // Leaf nodes
        QString strItem = item->text(col);
        QStringList strlistItem = strItem.split(u' ');
        int n = strlistItem.at(0).toInt();
        int l = strlistItem.at(1).toInt();
        int m = strlistItem.at(2).toInt();
        
        if (checked) {
            // Add orbital to table
            QString strWeight = "1";
            bool found = false;

            // Find weight if it already exists in harmap
            for (auto& vecElem : mapCloudRecipes[n]) {
                if (vecElem.x == l && vecElem.y == m) {
                    strWeight = QString::number(vecElem.z);
                    this->numRecipes++;
                    found = true;
                    break;
                }
            }

            QTableWidgetItem *thisOrbital = new SortableOrbitalTa(strItem);
            QTableWidgetItem *thisWeight = new SortableOrbitalTa(strWeight);
            int intTableRows = tableOrbitalReport->rowCount();
            tableOrbitalReport->setRowCount(intTableRows + 1);
            tableOrbitalReport->setItem(intTableRows, 1, thisOrbital);
            tableOrbitalReport->setItem(intTableRows, 0, thisWeight);
            tableOrbitalReport->setRowHeight(intTableRows, aStyle.tableFontSize + 2);
            thisOrbital->setTextAlignment(Qt::AlignCenter);
            thisOrbital->setForeground(Qt::white);
            thisWeight->setTextAlignment(Qt::AlignCenter);
            thisWeight->setForeground(Qt::gray);
            thisOrbital->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsEnabled);
            thisWeight->setFlags(Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);

            // Add orbital to harmap if it doesn't already exist
            if (!found) {
                ivec3 lmw = ivec3(l, m, 1);
                mapCloudRecipes[n].push_back(lmw);
                this->numRecipes++;
            }

            // Because adding, enable buttons
            buttClearHarmonics->setEnabled(true);
            groupRecipeReporter->setStyleSheet("");

        } else {
            // Remove orbital from table
            int intItemRow = tableOrbitalReport->findItems(strItem, Qt::MatchExactly).first()->row();
            tableOrbitalReport->removeRow(intItemRow);

            // Remove orbital from harmap
            if (int v = findHarmapItem(n, l, m); v >= -1) {
                this->mapCloudRecipes[n].erase(this->mapCloudRecipes[n].begin() + v);
                this->numRecipes--;
            }

            // Because removing, handle zero-weighted orbitals and empty table cases
            if (int c = tableOrbitalReport->rowCount(); c <= 1) {
                if (c == 1 && tableOrbitalReport->item(0, 0)->text() == "0") {
                    QMessageBox dialogConfim(this);
                    dialogConfim.setText("The only weighted orbital cannot be zero. Removing remaining orbital.");
                    dialogConfim.setStandardButtons(QMessageBox::Ok);
                    dialogConfim.setDefaultButton(QMessageBox::Ok);
                    dialogConfim.exec();

                    QString strOrbital = tableOrbitalReport->item(0, 1)->text();
                    treeOrbitalSelect->findItems(strOrbital, Qt::MatchFixedString | Qt::MatchRecursive, 0).at(0)->setCheckState(0, Qt::Unchecked);
                } else if (c == 0) {
                    buttMorbHarmonics->setEnabled(false);
                    buttClearHarmonics->setEnabled(false);
                    groupRecipeReporter->setStyleSheet("QGroupBox { color: #FF7777; }");
                }
            }
        }
    }

    /* ALL Nodes make it here */
    // Since we've made any change, enable render button
    handleCloudConfigChanged();


    // If has parent and all siblings are now checked/unchecked, check/uncheck parent
    while (ptrParent) {
        int intSiblings = ptrParent->childCount();
        bool homo = false;

        for (int i = 0; i < intSiblings; i++) {
            if (ptrParent->child(i)->checkState(col) != checked) {
                break;
            }
            
            if (i == intSiblings - 1) {
                homo = true;
            }
        }
        const QSignalBlocker treeBlocker(treeOrbitalSelect);
        ptrParent->setCheckState(col, (homo) ? checked : Qt::PartiallyChecked);
        ptrParent = (SortableOrbitalTr *)ptrParent->parent();
    }

    tableOrbitalReport->setSortingEnabled(true);
}

void MainWindow::handleButtLockRecipes() {
    const QSignalBlocker blocker(tableOrbitalReport);

    for (int i = 0; i < tableOrbitalReport->rowCount(); i++) {
        QTableWidgetItem *thisOrbital = tableOrbitalReport->item(i, 1);
        QTableWidgetItem *thisWeight = tableOrbitalReport->item(i, 0);
        // thisOrbital->setForeground(Qt::white);
        // thisWeight->setForeground(Qt::white);

        QString strOrbital = thisOrbital->text();
        QString strWeight = thisWeight->text();

        QStringList strlistItem = strOrbital.split(u' ');
        int n = strlistItem.at(0).toInt();
        int l = strlistItem.at(1).toInt();
        int m = strlistItem.at(2).toInt();
        int w = strWeight.toInt();
        ivec3 lmw = ivec3(l, m, w);
        std::vector<ivec3> *vecElem = &mapCloudRecipes[n];

        if (std::find(vecElem->begin(), vecElem->end(), lmw) == vecElem->end()) {
            // Add item to harmap
            vecElem->push_back(lmw);
            this->numRecipes++;
        } else {
            // Look for partial match and update weight
            for (auto& recipe : mapCloudRecipes[n]) {
                if (recipe.x == l && recipe.y == m) {
                    recipe.z = w;
                    break;
                }
            }
        }
    }
    // buttResetRecipes->setEnabled(true);
    buttMorbHarmonics->setEnabled(true);

    groupRecipeReporter->setStyleSheet("QGroupBox { color: #77FF77; }");
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
    mapCloudRecipes.clear();
    this->numRecipes = 0;

    // groupRecipeLocked->setStyleSheet("QGroupBox { color: #FF7777; }");
    buttMorbHarmonics->setEnabled(false);
}

void MainWindow::handleButtConfigIO(int id) {
    bool wave = (id % 2);
    bool save = (id / 2);

    if (save) {
        // Save config (and recipes) to file
        SuperConfig config = (wave) ? SuperConfig{ mw_waveConfig } : SuperConfig{ mw_cloudConfig };
        QString title = (wave) ? tr("Save Wave Config") : tr("Save Harmonics Config");
        QString extension = (wave) ? "wave" : "cloud";
        BitFlag mode = (wave) ? BitFlag(mw::WAVE) : BitFlag(mw::CLOUD);

        QFileDialog fd(this, title, QString::fromStdString(fileHandler->atomixFiles.configs()));
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.setDefaultSuffix(extension);
        fd.selectFile("filename." + extension);
        if (fd.exec() == QDialog::Accepted) {
            QString strCfgFile = fd.selectedFiles().first();
            QString strCfgName = strCfgFile.split(QDir::separator()).last();
            fileHandler->saveConfigFile(strCfgFile, config, (wave) ? nullptr : &mapCloudRecipes);
            refreshConfigs(mode, strCfgName);
        }
    } else {
        // Delete config file
        QComboBox *box = (wave) ? comboWaveConfigFile : comboCloudConfigFile;
        int comboID = box->currentIndex();
        QString strCfgName = box->currentText();
        QString strCfgFile = (wave) ? fileHandler->getWaveFilesList().at(comboID) : fileHandler->getCloudFilesList().at(comboID);

        QMessageBox dialogConfim(this);
        dialogConfim.setText("Are you sure you want to delete \"" + strCfgName + "\"?");
        dialogConfim.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        dialogConfim.setDefaultButton(QMessageBox::Cancel);
        dialogConfim.exec();

        if (dialogConfim.result() == QMessageBox::Ok) {
            if (fileHandler->deleteFile(strCfgFile)) {
                refreshConfigs(BitFlag(mw::WAVE));
            } else {
                QMessageBox dialogError(this);
                dialogError.setText("Failed to delete \"" + strCfgName + "\".");
                dialogError.setStandardButtons(QMessageBox::Ok);
                dialogError.setDefaultButton(QMessageBox::Ok);
                dialogError.exec();
            }
        }
    }
}

void MainWindow::handleButtMorbWaves() {
    std::pair<bool, double> resultP, resultW;
    int oldWaves = mw_waveConfig.waves;
    int newWaves = std::clamp(entryOrbit->text().toInt(), 1, 8);
    std::pair<int, int> waveChange = { oldWaves, newWaves - oldWaves };
    
    mw_waveConfig.waves = newWaves;
    mw_waveConfig.amplitude = std::clamp(entryAmp->text().toDouble(), 0.001, 999.999);
    mw_waveConfig.resolution = std::clamp(entryResolution->text().toInt(), 1, 999);
    mw_waveConfig.parallel = slswPara->value();
    mw_waveConfig.superposition = slswSuper->value();
    mw_waveConfig.cpu = slswCPU->value();
    mw_waveConfig.sphere = slswSphere->value();
    mw_waveConfig.visibleOrbits = refreshOrbits(waveChange);

    resultP = _validateExprInput(entryPeriod);
    mw_waveConfig.period = std::clamp(resultP.second, 0.001, 999.999);

    resultW = _validateExprInput(entryWavelength);
    mw_waveConfig.wavelength = std::clamp(resultW.second, 0.001, 999.999);

    if (!resultP.first || !resultW.first) {
        return;
    }

    vkGraph->newWaveConfig(&mw_waveConfig);

    groupColors->setEnabled(true);
    groupOrbits->setEnabled(true);
    if (numRecipes > 0) {
        buttMorbHarmonics->setEnabled(true);
    }
    this->activeModel = true;
    if (notDefaultConfig) {
        buttSaveWaveConfig->setEnabled(true);
    }
}

void MainWindow::handleButtMorbHarmonics() {
    mw_cloudConfig.cloudLayDivisor = entryCloudLayers->text().toInt();
    mw_cloudConfig.cloudResolution = entryCloudRes->text().toInt();
    mw_cloudConfig.cloudTolerance = entryCloudMinRDP->text().toDouble();

    for (auto [key, vec] : mapCloudRecipes) {
        if (vec.size() == 0) {
            mapCloudRecipes.erase(key);
        }
    }

    uint vertex, opt, index;
    uint64_t total;
    vkGraph->estimateSize(&mw_cloudConfig, &mapCloudRecipes, &vertex, &opt, &index);
    total = vertex + opt + index;
    uint64_t oneGiB = 1024 * 1024 * 1024;

    if (total > oneGiB) {
        std::array<float, 4> bufs = { static_cast<float>(vertex), static_cast<float>(opt), static_cast<float>(index), static_cast<float>(total) };
        QStringList units = { " B", "KB", "MB", "GB" };
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
        dialogConfim.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        dialogConfim.setDefaultButton(QMessageBox::Ok);
        if (dialogConfim.exec() == QMessageBox::Cancel) { return; }
    }

    vkGraph->newCloudConfig(&this->mw_cloudConfig, &this->mapCloudRecipes, true);

    // groupRecipeLocked->setStyleSheet("QGroupBox { color: #FFFF77; }");
    groupGenVertices->setStyleSheet("QGroupBox { color: #FFFF77; }");
    groupHSlideCulling->setEnabled(true);
    groupVSlideCulling->setEnabled(true);
    groupRSlideCulling->setEnabled(true);
    buttMorbHarmonics->setEnabled(false);
    activeModel = true;
    if (comboCloudConfigFile->currentIndex() == 0) {
        buttSaveCloudConfig->setEnabled(true);
    }
}

void MainWindow::handleWeightChange(int row, [[maybe_unused]] int col) {
    // Haha weight change *cries in 38*
    QTableWidgetItem *thisOrbital = tableOrbitalReport->item(row, 1);
    QTableWidgetItem *thisWeight = tableOrbitalReport->item(row, 0);
    
    QString strOrbital = thisOrbital->text();
    QString strWeight = thisWeight->text();

    QStringList strlistItem = strOrbital.split(u' ');
    int n = strlistItem.at(0).toInt();
    int l = strlistItem.at(1).toInt();
    int m = strlistItem.at(2).toInt();
    int w = strWeight.toInt();

    if (w == 0) {
        if (tableOrbitalReport->rowCount() == 1) {
            QMessageBox dialogConfim(this);
            dialogConfim.setText("The only weighted orbital cannot be zero. Removing this orbital.");
            dialogConfim.setStandardButtons(QMessageBox::Ok);
            dialogConfim.setDefaultButton(QMessageBox::Ok);
            dialogConfim.exec();

            treeOrbitalSelect->findItems(strOrbital, Qt::MatchFixedString | Qt::MatchRecursive, 0).at(0)->setCheckState(0, Qt::Unchecked);
            return;
        }
    }

    std::vector<ivec3> *vecElem = &mapCloudRecipes[n];
    // Look for partial match and update weight
    if (int v = findHarmapItem(n, l, m); v != -1) {
        vecElem->at(v).z = w;
    }
    
    thisWeight->setForeground(Qt::yellow);
    
    buttMorbHarmonics->setEnabled(true);
    groupRecipeReporter->setStyleSheet("QGroupBox { color: #FFFF77; }");
}

void MainWindow::handleSwitchToggle(int id, bool checked) {
    enum CfgButt { PARA = 0, SUPER  = 1, CPU = 2, SPHERE = 3 };

    if (checked) {
        switch (id) {
            case PARA:
                // Parallel waves
                break;
            case SUPER:
                // Superposition
                buttGroupSwitch->button(PARA)->setChecked(true);
                buttGroupSwitch->button(CPU)->setChecked(true);
                break;
            case CPU:
                // CPU rendering
                break;
            case SPHERE:
                // Spherical wave pattern
                buttGroupSwitch->button(PARA)->setChecked(true);
                break;
            default:
                break;
        }
    } else {
        switch (id) {
            case PARA:
                // Orthogonal waves
                buttGroupSwitch->button(SUPER)->setChecked(false);
                buttGroupSwitch->button(SPHERE)->setChecked(false);
                break;
            case SUPER:
                // No superposition
                break;
            case CPU:
                // GPU rendering
                buttGroupSwitch->button(SUPER)->setChecked(false);
                break;
            case SPHERE:
                // Circular wave pattern
                break;
            default:
                break;
        }
    }

    handleWaveConfigChanged();
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

    QString rawHex = QString::fromStdString(std::format("{:08X}", colour));
    QString colourHex = "#" + rawHex.last(2) + rawHex.first(6);
    pmColour->fill(QColor::fromString(colourHex));
    buttGroupColors->button(id)->setIcon(QIcon(*pmColour));

    vkGraph->setColorsWaves(id, colour);
}

void MainWindow::handleSlideCullingX(int val) {
    float pct = (static_cast<float>(val) / static_cast<float>(aStyle.sliderTicks));
    this->mw_cloudConfig.cloudCull_x = pct;
}

void MainWindow::handleSlideCullingY(int val) {
    float pct = (static_cast<float>(val) / static_cast<float>(aStyle.sliderTicks));
    this->mw_cloudConfig.cloudCull_y = pct;
}

void MainWindow::handleSlideCullingR(int val) {
    int range = aStyle.sliderTicks;
    float pct = static_cast<float>(val) / static_cast<float>(range);

    this->mw_cloudConfig.cloudCull_rIn = 0.0f;
    this->mw_cloudConfig.cloudCull_rOut = 0.0f;

    if (val < 0) {
        this->mw_cloudConfig.cloudCull_rIn = -pct;
    } else if (val > 0) {
        this->mw_cloudConfig.cloudCull_rOut = pct;
    }
}

void MainWindow::handleSlideReleased() {
    if (!activeModel) { return; }

    if ((this->mw_cloudConfig.cloudCull_x != lastSliderSentX) || (this->mw_cloudConfig.cloudCull_y != lastSliderSentY) || (this->mw_cloudConfig.cloudCull_rIn != lastSliderSentRIn) || (this->mw_cloudConfig.cloudCull_rOut != lastSliderSentROut)) {
        vkGraph->newCloudConfig(&this->mw_cloudConfig, &this->mapCloudRecipes, false);

        lastSliderSentX = this->mw_cloudConfig.cloudCull_x;
        lastSliderSentY = this->mw_cloudConfig.cloudCull_y;
        lastSliderSentRIn = this->mw_cloudConfig.cloudCull_rIn;
        lastSliderSentROut = this->mw_cloudConfig.cloudCull_rOut;
    }
}

void MainWindow::handleSlideBackground(int val) {
    vkGraph->setBGColour((static_cast<float>(val) / static_cast<float>(aStyle.sliderTicks)));
}

int MainWindow::findHarmapItem(int n, int l, int m) {
    for (int i = 0; i < (int)this->mapCloudRecipes[n].size(); i++) {
        ivec3 vecElem = mapCloudRecipes[n][i];
        if (vecElem.x == l && vecElem.y == m) {
            return i;
        }
    }
    return -1;
}

int MainWindow::getHarmapSize() {
    size_t totalSize = 0;
    for (auto k : mapCloudRecipes) {
        totalSize += k.second.size();
    }
    return static_cast<int>(totalSize);
}

/* void MainWindow::printHarmap() {
    for (auto k : mapCloudRecipes) {
        std::cout << k.first << ": ";
        for (auto v : k.second) {
            std::cout << glm::to_string(v) << ", ";
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
} */

void MainWindow::printList() {
    int listSize = tableOrbitalReport->rowCount();
    for (int i = 0; i < listSize; i++) {
        QTableWidgetItem *item = tableOrbitalReport->item(i, 1);
        std::cout << item << ": " << item->text().toStdString() << "\n";
    }
    std::cout << std::endl;
}

void MainWindow::printLayout() {
    std::cout << "<=====[ Print Layout ]=====>\n" << "\n";
    std::cout << "MainWindow: " << std::setw(4) << mw_width << "x" << std::setw(4) << mw_height << "\n";
    std::cout << "Graph:      " << std::setw(4) << mw_graphWidth << "x" << std::setw(4) << mw_graphHeight << "\n";
    std::cout << "Tabs [" << mw_tabCount << "]:   " << std::setw(4) << mw_tabWidth << "x" << std::setw(4) << mw_tabHeight << "\n";
    
    for (int i = 0; i < wTabs->count(); i++) {
        QLayout *topLay = wTabs->widget(i)->layout();  
        std::cout << std::endl;      
        _printLayout(topLay, 1, i);
    }
    std::cout << std::endl;
}

void MainWindow::_printLayout(QLayout *lay, int lvl, int idx) {
    if (!lay) { return; }

    int dent = lvl * 4;
    std::string idxDent = "[" + std::to_string(idx) + "]" + ((idx <= 9) ? std::string(2, ' ') : std::string(1, ' '));
    std::string hint = std::to_string(lay->sizeHint().width()) + "x" + std::to_string(lay->sizeHint().height());
    std::string min = std::to_string(lay->minimumSize().width()) + "x" + std::to_string(lay->minimumSize().height());
    int children = lay->count();

    std::cout << std::string(dent, ' ') << idxDent << "Layout | SizeHint: " << std::left << std::setw(9) << hint << " | Layout MinSize : "
        << std::setw(9) << min << " | Items: " << children << "\n";

    if (!children) {
        return;
    } else {
        int nameLen = 0;
        for (int i = 0; i < children; i++) {
            QWidget *widget = lay->itemAt(i)->widget();
            if (widget) {
                nameLen = std::max(nameLen, int(widget->objectName().length()));
            }
        }

        for (int i = 0; i < lay->count(); i++) {
            _printChild(lay->itemAt(i), lvl + 1, i, nameLen);
        }
    }
}

void MainWindow::_printChild(QLayoutItem *child, int lvl, int idx, int nameLen) {
    if (!child) { return; }

    QLayout *lay = child->layout();
    QWidget *widget = child->widget();
    int dent = lvl * 4;
    std::string idxDent = "[" + std::to_string(idx) + "]" + ((idx <= 9) ? std::string(2, ' ') : std::string(1, ' '));

    if (widget) {
        std::string name = widget->objectName().toStdString();
        std::string hint = std::to_string(widget->sizeHint().width()) + "x" + std::to_string(widget->sizeHint().height());
        std::string min = std::to_string(widget->minimumSize().width()) + "x" + std::to_string(widget->minimumSize().height());
        bool hasLay = ((lay = widget->layout()) != nullptr);
        std::string hasLayStr = (hasLay) ? " | (Layout)" : "";

        std::cout << std::string(dent, ' ') << idxDent << "Widget: " << std::left << std::setw(nameLen) << name << " | SizeHint: "
            << std::setw(9) << hint << " | MinSize : " << std::setw(9) << min << hasLayStr << "\n";
        
        if (hasLay) {
            _printLayout(lay, lvl + 1);
        }
    } else if (lay) {
        _printLayout(lay, lvl, idx);
    } else {
        std::cout << std::string(dent, ' ') << idxDent << "S T R E T C H\n";
    }
}

void MainWindow::_initStyle() {
    // Add custom font(s)
    QString strFontMono = "Inconsolata";
    QString strMonoDefault = (isMacOS) ? "Monaco" : "Monospace";
    QFont fontMono;

    int id = QFontDatabase::addApplicationFont(QString::fromStdString(fileHandler->atomixFiles.fonts()) + strFontMono + "-Regular.ttf");
    QStringList fontList = QFontDatabase::applicationFontFamilies(id);
    if (fontList.contains(strFontMono)) {
        fontMono = QFont(strFontMono);
    } else {
        fontMono = QFont(strMonoDefault);
        strFontMono = strMonoDefault;
    }
    aStyle.setFonts(this->font(), fontMono, strFontMono);

    // Set defaults because we haven't added tabs or shown the window yet
    mw_tabWidth = int(mw_width * 0.2);
    mw_tabHeight = mw_height - this->style()->pixelMetric(QStyle::PM_TitleBarHeight);
    mw_tabCount = 8;

    _setStyle();
}

void MainWindow::_initGraphics() {
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
    if (isDebug) {
        std::cout << "Vulkan API version: " << version.toString().toStdString() << std::endl;
        std::cout << "Vulkan SPIRV version: 1." << VK_SPIRV_VERSION << std::endl;
    }
    
    vkInst.setApiVersion(version);
    vkInst.setLayers(layers);
    vkInst.setExtensions(extensions);
    if (!vkInst.create()) {
        qFatal("Failed to create Vulkan Instance: %d", vkInst.errorCode());
    }
    
    vkGraph = new VKWindow(this, fileHandler);
    vkGraph->setVulkanInstance(&vkInst);
    graph = QWidget::createWindowContainer(vkGraph);
    graph->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setCentralWidget(graph);
}

    /**
     * @brief Initializes all the widgets in the main window, including validators, dock GUI and status bar.
     *
     * Initializes input validators, sets up dock GUI, refreshes configuration and shader selection, loads the default configuration,
     * refreshes available orbitals, sets up the status bar, and sets up the details dock.
     */
void MainWindow::_initWidgets() {
    // Input Validators
    valIntSmall = new QIntValidator();  
    valIntSmall->setRange(1, 8);
    valIntLarge = new QIntValidator();
    valIntLarge->setRange(1, 999);
    valDoubleSmall = new QDoubleValidator();
    valDoubleSmall->setRange(0.0001, 0.9999, 4);
    valDoubleLarge = new QDoubleValidator();
    valDoubleLarge->setRange(0.001, 999.999, 3);

    // Setup Dock GUI
    setupTabs();
    
    refreshConfigs(BitFlag(mw::BOTH));
    refreshOrbits();

    setupDetails();
    setupLoading();
    setupStatusBar();
}

/**
 * @brief Connects GUI signals to their respective slots.
 *
 * Initializes all necessary signal-slot connections for the application's GUI.
 */
void MainWindow::_connectSignals() {
    /* 
     * User Interface
     */

    // Status Bar
    connect(vkGraph, &VKWindow::detailsChanged, this, &MainWindow::updateDetails);
    connect(vkGraph, &VKWindow::toggleLoading, this, &MainWindow::showLoading);

    /* 
     * Config Files
     */

    // Config Files
    connect(comboWaveConfigFile, &QComboBox::activated, this, &MainWindow::loadWaveConfig);
    connect(comboCloudConfigFile, &QComboBox::activated, this, &MainWindow::loadCloudConfig);
    connect(buttGroupConfig, &QButtonGroup::idClicked, this, &MainWindow::handleButtConfigIO);
    // connect(buttDeleteWaveConfig, &QPushButton::clicked, this, &MainWindow::handleButtDeleteConfig);
    // connect(buttSaveWaveConfig, &QPushButton::clicked, this, &MainWindow::handleButtSaveConfig);

    /*
     * Waves
     */
    
    // Wave Config Values
    connect(entryOrbit, &QLineEdit::editingFinished, this, &MainWindow::handleWaveConfigChanged);
    connect(entryAmp, &QLineEdit::editingFinished, this, &MainWindow::handleWaveConfigChanged);
    connect(entryPeriod, &QLineEdit::editingFinished, this, &MainWindow::handleWaveConfigChanged);
    connect(entryWavelength, &QLineEdit::editingFinished, this, &MainWindow::handleWaveConfigChanged);
    connect(entryResolution, &QLineEdit::editingFinished, this, &MainWindow::handleWaveConfigChanged);
    connect(entryOrbit, &QLineEdit::returnPressed, buttMorbWaves, &QPushButton::click);
    connect(entryAmp, &QLineEdit::returnPressed, buttMorbWaves, &QPushButton::click);
    connect(entryPeriod, &QLineEdit::returnPressed, buttMorbWaves, &QPushButton::click);
    connect(entryWavelength, &QLineEdit::returnPressed, buttMorbWaves, &QPushButton::click);
    connect(entryResolution, &QLineEdit::returnPressed, buttMorbWaves, &QPushButton::click);
    connect(buttGroupSwitch, &QButtonGroup::idToggled, this, &MainWindow::handleSwitchToggle);
    
    // Wave Render
    connect(buttMorbWaves, &QPushButton::clicked, this, &MainWindow::handleButtMorbWaves);

    // Wave Colours and Orbits
    connect(buttGroupColors, &QButtonGroup::idClicked, this, &MainWindow::handleButtColors);
    connect(buttGroupOrbits, &QButtonGroup::idToggled, vkGraph, &VKWindow::selectRenderedWaves, Qt::DirectConnection);

    /*
     * Harmonics
     */

    // Harmonic Recipes
    connect(treeOrbitalSelect, &QTreeWidget::itemChanged, this, &MainWindow::handleRecipeCheck);
    connect(treeOrbitalSelect, &QTreeWidget::itemDoubleClicked, this, &MainWindow::handleTreeDoubleClick);
    connect(tableOrbitalReport, &QTableWidget::cellChanged, this, &MainWindow::handleWeightChange);
    connect(tableOrbitalReport, &QTableWidget::cellDoubleClicked, this, &MainWindow::handleTableDoubleClick);
    
    // Harmonic Config Values
    connect(entryCloudLayers, &QLineEdit::editingFinished, this, &MainWindow::handleCloudConfigChanged);
    connect(entryCloudRes, &QLineEdit::editingFinished, this, &MainWindow::handleCloudConfigChanged);
    connect(entryCloudMinRDP, &QLineEdit::editingFinished, this, &MainWindow::handleCloudConfigChanged);
    connect(entryCloudLayers, &QLineEdit::returnPressed, buttMorbHarmonics, &QPushButton::click);
    connect(entryCloudRes, &QLineEdit::returnPressed, buttMorbHarmonics, &QPushButton::click);
    connect(entryCloudMinRDP, &QLineEdit::returnPressed, buttMorbHarmonics, &QPushButton::click);
    
    // Harmonic Render & Clear
    connect(buttMorbHarmonics, &QPushButton::clicked, this, &MainWindow::handleButtMorbHarmonics);
    connect(buttClearHarmonics, &QPushButton::clicked, this, &MainWindow::handleButtClearRecipes);
    
    // Harmonic Culling & Background
    connect(slideCullingX, &QSlider::valueChanged, this, &MainWindow::handleSlideCullingX);
    connect(slideCullingY, &QSlider::valueChanged, this, &MainWindow::handleSlideCullingY);
    connect(slideCullingR, &QSlider::valueChanged, this, &MainWindow::handleSlideCullingR);
    connect(slideCullingX, &QSlider::sliderReleased, this, &MainWindow::handleSlideReleased);
    connect(slideCullingY, &QSlider::sliderReleased, this, &MainWindow::handleSlideReleased);
    connect(slideCullingR, &QSlider::sliderReleased, this, &MainWindow::handleSlideReleased);
    connect(slideBackground, &QSlider::sliderMoved, this, &MainWindow::handleSlideBackground);
}

void MainWindow::_setStyle() {
    aStyle.setWindowSize(mw_width, mw_height);
    aStyle.setDockSize(mw_tabWidth, mw_tabHeight, mw_tabCount);
    aStyle.updateStyleSheet();
    
    this->setStyleSheet(aStyle.getStyleSheet());
    
    if (isDebug) {
        aStyle.printStyleSheet();
    }
}

void MainWindow::_dockResize() {
    QRect tabLoc = wTabs->geometry();
    mw_tabWidth = tabLoc.width();
    mw_tabHeight = tabLoc.height();

    _setStyle();

    // Wave GUI
    labelWaves->setFixedHeight(aStyle.labelDescHeight);
    labelWaves->setLineWidth(aStyle.borderWidth);
    labelWaves->setMargin(aStyle.spaceM);
    buttDeleteWaveConfig->setMaximumWidth(aStyle.fontAtomixWidth << 1);
    buttSaveWaveConfig->setMaximumWidth(aStyle.fontAtomixWidth << 1);
    layWaveConfigFile->setContentsMargins(aStyle.spaceS, aStyle.spaceS, aStyle.spaceS, aStyle.spaceS);
    layWaveConfigFile->setSpacing(aStyle.spaceS);
    layWaveConfig->setHorizontalSpacing(aStyle.spaceL);
    layWaveConfig->setVerticalSpacing(aStyle.spaceM);
    layColorPicker->setContentsMargins(aStyle.spaceS, aStyle.spaceS, aStyle.spaceS, aStyle.spaceS);
    layColorPicker->setSpacing(aStyle.spaceS);
    layOrbitSelect->setSpacing(aStyle.spaceS);
    layDockWaves->setContentsMargins(aStyle.spaceM, aStyle.spaceM, aStyle.spaceM, aStyle.spaceM);
    layDockWaves->setSpacing(aStyle.spaceM);

    delete pmColour;
    pmColour = new QPixmap(aStyle.baseFontSize, aStyle.baseFontSize);

    // Harmonic GUI
    treeOrbitalSelect->setIndentation(aStyle.fontMonoWidth);
    labelHarmonics->setFixedHeight(aStyle.labelDescHeight);
    labelHarmonics->setLineWidth(aStyle.borderWidth);
    labelHarmonics->setMargin(aStyle.spaceM);
    layGenVertices->setHorizontalSpacing(aStyle.spaceL);
    layGenVertices->setVerticalSpacing(aStyle.spaceM);
    layGenVertices->setContentsMargins(aStyle.spaceS, aStyle.spaceS, aStyle.spaceS, aStyle.spaceS);
    layDockHarmonics->setContentsMargins(aStyle.spaceM, aStyle.spaceM, aStyle.spaceM, aStyle.spaceM);
    layDockHarmonics->setSpacing(aStyle.spaceM);

    // Status Bar
    labelDetails->setFont(aStyle.fontMonoStatus);
    labelDetails->adjustSize();
    statBar->setStyleSheet(QString("font-family: %1; font-size: %2px;").arg(aStyle.strFontMono).arg(aStyle.statusFontSize));
}

void MainWindow::_resize() {
    int currentTabIdx = wTabs->currentIndex();

    int i = 0;
    while (i < wTabs->count()) {
        if (++currentTabIdx == wTabs->count()) {
            currentTabIdx = 0;
        }

        wTabs->setCurrentIndex(currentTabIdx);
        if (currentTabIdx == 0) {                               // [Waves]
            // We don't need to do anything here, anymore
        } else if (currentTabIdx == 1) {                        // [Harmonics]
            // We don't need to do anything here, anymore
        }

        i++;
    }
}

std::pair<bool, double> MainWindow::_validateExprInput(QLineEdit *entry) {
    QString eval = entry->text();
    bool valid = false;
    double value = 0;

    value = eval.toDouble(&valid);
    if (valid) {
        entry->setStyleSheet("");
        return std::make_pair(valid, value);
    }
    
    QRegularExpression re("[0-9()+\\-*/.pie\\s]*");
    QRegularExpression reMult("[0-9.]+pi");
    QRegularExpressionValidator val(re, 0);
    int pos = 0;

    QJSEngine evalEngine;
    evalEngine.globalObject().setProperty("pi", M_PI);
    evalEngine.globalObject().setProperty("e", M_E);

    if (val.validate(eval, pos) == QValidator::Acceptable) {
        if (reMult.match(eval).hasMatch()) {
            eval = eval.replace("pi", " * pi");
        }
        QJSValue evalResult = evalEngine.evaluate(eval);
        
        if (evalResult.isError()) {
            entry->setStyleSheet("color: #FF7777;");
            qDebug() << evalResult.toString();
        } else {
            entry->setStyleSheet("color: #77FF77;");
            valid = true;
            value = evalResult.toNumber();
            entry->setText(QString::number(value, 'f', 6));
        }
    } else {
        entry->setStyleSheet("color: #FF7777;");
    }

    return std::make_pair(valid, value);
}
