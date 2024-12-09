/**
 * mainwindow.hpp
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QtWidgets/QMainWindow>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QSlider>
#include <QtWidgets/QLabel>
#include <QFontMetrics>
#include <QPixmap>
#include <QIcon>

#include "slideswitch.hpp"
#include "filehandler.hpp"
#include "vkwindow.hpp"


const QString DEFAULT = QT_TR_NOOP("default.wave");
const QString CUSTOM = QT_TR_NOOP("<Custom>");
const QString SELECT = QT_TR_NOOP("<Select>");
const int MAX_ORBITS = 8;


struct AtomixStyle {
    void setWindowSize(int width, int height) {
        windowWidth = width;
        windowHeight = height;
    }

    void setDockSize(int width, int height, int tabCount) {
        dockWidth = width;
        dockHeight = height;
        tabLabelWidth = dockWidth / tabCount;
    }

    void setFonts(QFont baseFont, QFont fontMono, QString strMonoFont) {
        fontAtomix = baseFont;
        fontMonoStatus = fontMono;
        strFontMono = strMonoFont;
    }
    
    void scaleFonts() {
        baseFontSize = int(round(dockWidth * 0.04));
        descFontSize = int(round(baseFontSize * 1.333));
        tabSelectedFontSize = int(round(baseFontSize * 1.15));
        tabUnselectedFontSize = int(round(baseFontSize * 0.90));
        treeFontSize = baseFontSize + 3;
        tableFontSize = baseFontSize + 2;
        morbFontSize = descFontSize;
        statusFontSize = baseFontSize + 3;

        fontAtomix.setPixelSize(baseFontSize);
        QFontMetrics fmA(fontAtomix);
        fontAtomixWidth = fmA.horizontalAdvance("W");
        fontAtomixHeight = fmA.height();

        fontMono.setPixelSize(treeFontSize);
        QFontMetrics fmM(fontMono);
        fontMonoWidth = fmM.horizontalAdvance("W");
        fontMonoHeight = fmM.height();

        fontMonoStatus.setPixelSize(statusFontSize);
    }

    void scaleWidgets() {
        treeCheckSize = int(round(baseFontSize * 1.5));
        labelDescHeight = descFontSize * 5;
        sliderTicks = 20;
        sliderInterval = sliderTicks >> 2;
        borderWidth = (isMacOS) ? 1 : 3;

        spaceL = fontAtomixWidth;
        spaceM = fontAtomixWidth >> 1;
        spaceS = fontAtomixWidth >> 2;
    }

    void updateStyleSheet() {
        scaleFonts();
        scaleWidgets();

        // Note: WidgetItems must have border defined even as 0px for margin/padding to work
        if (qtStyle == "macos") {
            styleStringList = QStringList({
                "QWidget { font-size: %1px; } "
                "QDockWidget::title { border: 1px hidden gray; } "
                "QTabWidget { border: none; } "
                "QLabel#tabDesc { font-size: %2px; } "
                // "QGroupBox::title { subcontrol-position: top right; padding:2 4px; } "
                "QTreeWidget { font-family: %5; font-size: %3px; } "
                "QTableWidget { font-family: %5; font-size: %4px; } "
                "QPushButton#morb { font-size: %6px; } "
            });
        } else if (qtStyle == "fusion") {
            styleStringList = QStringList({
                "QWidget { font-size: %1px; } "
                "QTabBar::tab { height: 40px; width: %3px; font-size: %4px; } "
                "QTabBar::tab::selected { font-size: %5px; } "
                "QLabel#tabDesc { font-size: %2px; } "
                "QTreeWidget { font-family: %8; font-size: %6px; } "
                "QTableWidget { font-family: %8; font-size: %7px; } "
                "QPushButton#morb { font-size: %9px; text-align: center; padding: %1px; } "
            });
        }

        genStyleString();
    }

    void genStyleString() {
        if (qtStyle == "macos") {
            strStyle = styleStringList.join(" ")
                .arg(QString::number(baseFontSize))             // 1
                .arg(QString::number(descFontSize))             // 2
                .arg(QString::number(treeFontSize))             // 3
                .arg(QString::number(tableFontSize))            // 4
                .arg(strFontMono)                                // 5
                .arg(QString::number(morbFontSize));            // 6
        } else if (qtStyle == "fusion") {
            strStyle = styleStringList.join(" ")
                .arg(QString::number(baseFontSize))             // 1
                .arg(QString::number(descFontSize))             // 2
                .arg(QString::number(tabLabelWidth))            // 3
                .arg(QString::number(tabUnselectedFontSize))    // 4
                .arg(QString::number(tabSelectedFontSize))      // 5
                .arg(QString::number(treeFontSize))             // 6
                .arg(QString::number(tableFontSize))            // 7
                .arg(strFontMono)                                // 8
                .arg(QString::number(morbFontSize));            // 9
                // .arg(QString::number(statusFontSize));          // 10
        }
    }

    QString& getStyleSheet() {
        return strStyle;
    }

    void printStyleSheet() {
        std::cout << "\nStyleSheet:\n" << strStyle.toStdString() << "\n" << std::endl;
    }

    QString qtStyle;

    uint baseFontSize, tabSelectedFontSize, tabUnselectedFontSize, descFontSize, treeFontSize, tableFontSize, morbFontSize, statusFontSize;
    uint tabLabelWidth, labelDescHeight, sliderTicks, sliderInterval, borderWidth, treeCheckSize;
    uint spaceS, spaceM, spaceL;

    uint windowWidth, windowHeight, dockWidth, dockHeight, halfDock, quarterDock;

    QStringList styleStringList;
    QString strStyle, strFontMono;
    QFont fontMono, fontMonoStatus, fontAtomix;
    int fontMonoWidth, fontMonoHeight, fontAtomixWidth, fontAtomixHeight;
};

class SortableOrbitalTa : public QTableWidgetItem {
public:
    SortableOrbitalTa(int type = QTableWidgetItem::Type) : QTableWidgetItem(type) {}
    SortableOrbitalTa(QString &text, int type = QTableWidgetItem::Type) : QTableWidgetItem(text, type) {}
    ~SortableOrbitalTa() override = default;

    bool operator<(const QTableWidgetItem &other) const override {
        QString thisText = this->text().simplified().replace(" ", "");
        int textValA = thisText.first(2).toInt();
        int textValB = thisText.sliced(2).toInt();
        QString otherText = other.text().simplified().replace(" ", "");
        int otherTextValA = otherText.first(2).toInt();
        int otherTextValB = otherText.sliced(2).toInt();
        bool aLess = textValA < otherTextValA;
        bool aEqual = textValA == otherTextValA;
        bool bLess = textValB < otherTextValB;
        return ((aLess) || (aEqual && bLess));
    }
};

class SortableOrbitalTr : public QTreeWidgetItem {
public:
    SortableOrbitalTr(int type = QTreeWidgetItem::Type) : QTreeWidgetItem(type) {}
    SortableOrbitalTr(QTreeWidget *parent, int type = QTreeWidgetItem::Type) : QTreeWidgetItem(parent, type) {}
    SortableOrbitalTr(QTreeWidgetItem *parent, int type = QTreeWidgetItem::Type) : QTreeWidgetItem(parent, type) {}
    SortableOrbitalTr(const QStringList &strings, int type = QTreeWidgetItem::Type) : QTreeWidgetItem(strings, type) {}
    ~SortableOrbitalTr() override = default;

    bool operator<(const QTreeWidgetItem &other) const override {
        int col = this->columnCount() - 1;
        QString thisText = this->text(col).simplified().replace(" ", "");
        int textValA = thisText.first(1).toInt();
        int textValB = thisText.sliced(1, 1).toInt();
        int textValC = thisText.sliced(2).toInt();
        QString otherText = other.text(col).simplified().replace(" ", "");
        int otherTextValA = otherText.first(1).toInt();
        int otherTextValB = otherText.sliced(1, 1).toInt();
        int otherTextValC = otherText.sliced(2).toInt();
        bool aLess = textValA < otherTextValA;
        bool aEqual = textValA == otherTextValA;
        bool bLess = textValB < otherTextValB;
        bool bEqual = textValB == otherTextValB;
        bool cLess = textValC < otherTextValC;
        return ((aLess) || (aEqual && bLess) || (aEqual && bEqual && cLess));
    }
};

enum mw { WAVE = 1, CLOUD = 2, BOTH = 3 };


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();
    void init(QRect &windowSize);
    void postInit();
    void resetGeometry() { this->loadGeometry = false; }

    AtomixFiles& getAtomixFiles() { return fileHandler->atomixFiles; }

signals:
    void changeRenderedOrbits(uint selectedOrbits);

public slots:
    void updateDetails(AtomixInfo *info);
    void showLoading(bool loading);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void setupTabs();
    void setupDockWaves();
    void setupDockHarmonics();
    void refreshShaders();
    void refreshConfigs(BitFlag target, QString selection = DEFAULT);
    void loadWaveConfig();
    void refreshWaveConfigGUI(AtomixWaveConfig &cfg);
    void loadCloudConfig();
    void refreshCloudConfigGUI(AtomixCloudConfig &cfg);
    uint refreshOrbits();
    void setupStatusBar();
    void setupDetails();
    void setupLoading();
    void showDetails();
    void showReady();
    void loadSavedSettings();

    void handleWaveConfigChanged();
    void handleCloudConfigChanged();
    void handleTreeDoubleClick(QTreeWidgetItem *item, int col);
    void handleTableDoubleClick(int row, int col);
    void handleRecipeCheck(QTreeWidgetItem *item, int col);
    void handleButtLockRecipes();
    void handleButtClearRecipes();
    void handleButtResetRecipes();
    void handleButtConfigIO(int id);
    void handleButtDeleteConfig();
    void handleButtSaveConfig();
    void handleButtMorbWaves();
    void handleButtMorbHarmonics();
    void handleSwitchToggle(int id, bool checked);
    void handleButtColors(int id);
    void handleWeightChange(int row, int col);
    void handleSlideCullingX(int val);
    void handleSlideCullingY(int val);
    void handleSlideCullingR(int val);
    void handleSlideReleased();
    void handleSlideBackground(int val);

    int findHarmapItem(int n, int l, int m);
    int getHarmapSize();
    void printList();
    
    void printLayout();
    void _printLayout(QLayout *layout, int lvl, int idx = 0);
    void _printChild(QLayoutItem *child, int lvl, int idx = 0, int nameLength = 0);

    void _initStyle();
    void _initGraphics();
    void _initWidgets();
    void _connectSignals();
    void _setStyle();
    void _dockResize();
    void _resize();
    std::pair<bool, double> _validateExprInput(QLineEdit *entry);

    FileHandler *fileHandler = nullptr;
    AtomixWaveConfig waveConfig;
    AtomixCloudConfig cloudConfig;

    QTabWidget *wTabs = nullptr;
    QWidget *wTabWaves = nullptr;
    QWidget *wTabHarmonics = nullptr;
    QDockWidget *dockTabs = nullptr;

    QIntValidator *valIntSmall = nullptr;
    QIntValidator *valIntLarge = nullptr;
    QDoubleValidator *valDoubleSmall = nullptr;
    QDoubleValidator *valDoubleLarge = nullptr;

    QVBoxLayout *layDockWaves = nullptr;
    QVBoxLayout *layDockHarmonics = nullptr;
    QHBoxLayout *layWaveConfigFile = nullptr;
    QHBoxLayout *layCloudConfigFile = nullptr;
    QHBoxLayout *layColorPicker = nullptr;
    QGridLayout *layOrbitSelect = nullptr;
    QFormLayout *layWaveConfig = nullptr;
    QFormLayout *layGenVertices = nullptr;

    QLabel *labelWaves = nullptr;
    QLabel *labelHarmonics = nullptr;
    
    QLineEdit *entryOrbit = nullptr;
    QLineEdit *entryAmp = nullptr;
    QLineEdit *entryPeriod = nullptr;
    QLineEdit *entryWavelength = nullptr;
    QLineEdit *entryResolution = nullptr;
    SlideSwitch *slswPara = nullptr;
    SlideSwitch *slswSuper = nullptr;
    SlideSwitch *slswCPU = nullptr;
    SlideSwitch *slswSphere = nullptr;
    QComboBox *comboWaveConfigFile = nullptr;
    QComboBox *comboCloudConfigFile = nullptr;

    QButtonGroup *buttGroupConfig = nullptr;
    QButtonGroup *buttGroupSwitch = nullptr;
    QButtonGroup *buttGroupOrbits = nullptr;
    QButtonGroup *buttGroupColors = nullptr;
    
    QPushButton *buttSaveWaveConfig = nullptr;
    QPushButton *buttSaveCloudConfig = nullptr;
    QPushButton *buttDeleteWaveConfig = nullptr;
    QPushButton *buttDeleteCloudConfig = nullptr;
    QPushButton *buttMorbWaves = nullptr;
    QPushButton *buttClearHarmonics = nullptr;
    QPushButton *buttMorbHarmonics = nullptr;

    QGroupBox *groupOptions = nullptr;
    QGroupBox *groupColors = nullptr;
    QGroupBox *groupOrbits = nullptr;
    QGroupBox *groupRecipeBuilder = nullptr;
    QGroupBox *groupRecipeReporter = nullptr;
    QGroupBox *groupGenVertices = nullptr;
    QGroupBox *groupHSlideCulling = nullptr;
    QGroupBox *groupVSlideCulling = nullptr;
    QGroupBox *groupRSlideCulling = nullptr;
    QGroupBox *groupSlideBackground = nullptr;
    QTreeWidget *treeOrbitalSelect = nullptr;
    QTableWidget *tableOrbitalReport = nullptr;
    QLineEdit *entryCloudLayers = nullptr;
    QLineEdit *entryCloudRes = nullptr;
    QLineEdit *entryCloudMinRDP = nullptr;
    QSlider *slideBackground = nullptr;
    QSlider *slideCullingX = nullptr;
    QSlider *slideCullingY = nullptr;
    QSlider *slideCullingR = nullptr;

    QStatusBar *statBar = nullptr;
    QLabel *labelDetails = nullptr;
    QProgressBar *pbLoading = nullptr;

    VKWindow *vkGraph = nullptr;
    QVulkanInstance vkInst;
    QWidget *vkWinWidWrapper = nullptr;
    QWidget *graph = nullptr;
    QWindow *graphWin = nullptr;

    harmap mapCloudRecipes;
    int numRecipes = 0;
    bool activeModel = false;
    bool isLoading = false;
    bool showDebug = false;
    bool notDefaultConfig = false;
    bool loadGeometry = true;

    int mw_baseFontSize = 0;
    
    float lastSliderSentX = 0.0f;
    float lastSliderSentY = 0.0f;
    float lastSliderSentRIn = 0.0f;
    float lastSliderSentROut = 0.0f;

    int mw_width = 0;
    int mw_height = 0;
    int mw_x = 0;
    int mw_y = 0;
    int mw_graphWidth = 0;
    int mw_graphHeight = 0;
    int mw_titleHeight = 0;
    int mw_tabWidth = 0;
    int mw_tabHeight = 0;
    int mw_tabCount = 0;

    QPixmap *pmColour = nullptr;

    AtomixInfo dInfo;
    AtomixStyle aStyle;
};

#endif
