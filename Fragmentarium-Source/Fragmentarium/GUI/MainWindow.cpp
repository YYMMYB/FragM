#include <QCoreApplication>
#include <QtGui>
#include <QDir>
#include <QMenu>
#include <QString>
#include <QClipboard>
#include <QDesktopServices>
#include <QImageWriter>
#include <QTextBlockUserData>
#include <QStack>
#include <QImage>
#include <QPixmap>
#include <QDialogButtonBox>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QtNetwork/QtNetwork>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "MainWindow.h"
#include "TextEdit.h"
#include "TimeLine.h"
#include "VideoDialog.h"

#include "PreferencesDialog.h"
#include "OutputDialog.h"
#include "VariableEditor.h"
#include "ListWidgetLogger.h"
#include "Exception.h"
#include "Preprocessor.h"
#include "Misc.h"

#include <iostream>

#ifdef USE_OPEN_EXR
using namespace Imf;
using namespace Imath;
#endif

using namespace SyntopiaCore::Misc;
using namespace SyntopiaCore::Logging;
using namespace SyntopiaCore::Exceptions;
using namespace Fragmentarium::Parser;

namespace Fragmentarium {
namespace GUI {

MainWindow::MainWindow(QSplashScreen* splashWidget) : splashWidget(splashWidget)
{

    bufferXSpinBox = 0;
    bufferYSpinBox = 0;
    lastStoredTime = 0;
    bufferSizeMultiplier = 1;
    exrMode = false;
#ifdef USE_OPEN_EXR
    exrToolsMenu = 0;
#endif
    // M Benesi "Spray gun" MainWindow
    fragHasFeedbackVars = false;
    feedbackindex = 0;
    feedbackcount = 0;
    feedbackmaxindex = 102;
    feedbackcrds = new QVector3D [feedbackmaxindex];
    feedcontrol1 = new QVector4D [feedbackmaxindex];
    feedcontrol2 = new QVector4D [feedbackmaxindex];
    feedrotation = new QVector4D [feedbackmaxindex];
    zapCheck = NULL;
    zapLock = NULL;
    QDir::setCurrent(QCoreApplication::applicationDirPath ()); // Otherwise we cannot find examples + templates
    init();
}

void MainWindow::createCommandHelpMenu(QMenu* menu, QWidget* textEdit, MainWindow* mainWindow)
{
    QMenu *preprocessorMenu = new QMenu(tr("Host Preprocessor Commands"), 0);
    preprocessorMenu->addAction("#info sometext", textEdit , SLOT(insertText()));
    preprocessorMenu->addAction("#include \"some.frag\"", textEdit , SLOT(insertText()));
    preprocessorMenu->addAction("#camera 2D", textEdit , SLOT(insertText()));
    preprocessorMenu->addAction("#camera 3D", textEdit , SLOT(insertText()));
    preprocessorMenu->addAction("#group parameter_group_name", textEdit , SLOT(insertText()));
    preprocessorMenu->addAction("#preset preset_name", textEdit , SLOT(insertText()));
    preprocessorMenu->addAction("#endpreset", textEdit , SLOT(insertText()));
    preprocessorMenu->addAction("#define DontClearOnChange", textEdit , SLOT(insertText()));
    preprocessorMenu->addAction("#define IterationsBetweenRedraws 10", textEdit , SLOT(insertText()));
    preprocessorMenu->addAction("#define SubframeMax 20", textEdit , SLOT(insertText()));

    QMenu *textureFlagsMenu = new QMenu(tr("2D Texture Options"), 0);
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_MAG_FILTER GL_NEAREST", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_WRAP_S GL_CLAMP", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_WRAP_T GL_CLAMP", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_MAX_LEVEL 1000", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_WRAP_S GL_REPEAT", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_WRAP_T GL_REPEAT", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_MAG_FILTER GL_LINEAR", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_MIN_FILTER GL_LINEAR_MIPMAP_LINEAR", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_MIN_FILTER GL_LINEAR_MIPMAP_NEAREST", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_MIN_FILTER GL_NEAREST_MIPMAP_LINEAR", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_MIN_FILTER GL_NEAREST_MIPMAP_NEAREST", textEdit , SLOT(insertText()));
    textureFlagsMenu->addAction("#TexParameter textureName GL_TEXTURE_MAX_ANISOTROPY float(>1.0 <16.0)", textEdit , SLOT(insertText()));

    QMenu *uniformMenu = new QMenu(tr("Special Uniforms"), 0);
    uniformMenu->addAction("uniform float time;", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform int subframe;", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform vec2 pixelSize;", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform int i; slider[0,1,2]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform float f; slider[0,1,2]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform vec2 v; slider[(0,0),(1,1),(1,1)]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform vec3 v; slider[(0,0,0),(1,1,1),(1,1,1)]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform vec4 v; slider[(0,0,0,0),(1,1,1,1),(1,1,1,1)]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform bool b; checkbox[true]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform sampler2D tex; file[tex.jpg]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform samplerCube cubetex; file[cubetex.jpg]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform vec3 color; color[0.0,0.0,0.0]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform vec4 color; color[0.0,1.0,0.0,0.0,0.0,0.0]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform bool DepthToAlpha; checkbox[true]", textEdit , SLOT(insertText()));
    uniformMenu->addAction("uniform bool AutoFocus; checkbox[true]", textEdit , SLOT(insertText()));

    uniformMenu->insertMenu(0, textureFlagsMenu);
    
    QMenu *includeMenu = new QMenu(tr("Include (from Preferences Paths)"), 0);

    QStringList filter;
    filter << "*.frag";
//    readSettings();
    QStringList files = mainWindow->getFileManager()->getFiles(filter);
    foreach (QString s, files) {
        includeMenu->addAction(QString("#include \"%1\"").arg(s),mainWindow, SLOT(insertText()));
    }

    QAction* before = 0;
    if (menu->actions().count() > 0) before = menu->actions()[0];
    menu->insertMenu(before, preprocessorMenu);
    menu->insertMenu(before, uniformMenu);
    menu->insertMenu(before, includeMenu);

    menu->insertSeparator(before);
    menu->addAction(tr("Insert Preset from Current Settings"),mainWindow, SLOT(insertPreset()));
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    bool modification = false;
    for (int i = 0; i < tabInfo.size(); i++) {
        if (tabInfo[i].unsaved) modification = true;
    }

    if (modification) {
        QString mess = tr("There are tabs with unsaved changes.%1\r\nContinue and loose changes?").arg( variableEditor->hasEasing() ? "\r\nTo keep Easing curves you must\r\nadd a \"Range\" preset and save to file\r\nbefore closing!":"\r\n");
        int i = QMessageBox::warning(this, tr("Unsaved changes"), mess, QMessageBox::Ok, QMessageBox::Cancel);
        if (i == QMessageBox::Ok) {
            // OK
            ev->accept();
            return;
        } else {
            // Cancel
            ev->ignore();
            return;
        }
    }
    ev->accept();

    writeSettings();
}

void MainWindow::newFile()
{
    insertTabPage("");
}

void MainWindow::insertPreset() {
    bool ok = false;
    QString newPreset;
    QTextCursor tc = getTextEdit()->textCursor();

    if(tc.hasSelection() &&
            tc.selectedText().contains("#preset", Qt::CaseInsensitive) &&
            tc.selectedText().contains("#endpreset", Qt::CaseInsensitive)
      ) { /// if we have selected text try to extract name
        newPreset = tc.selection().toPlainText();
        QStringList tmp = newPreset.split('\n');
        newPreset=tmp.at( tmp.indexOf( QRegExp("^#[Pp]reset.*$") ) );
        tmp = newPreset.split(" ");
        newPreset=tmp.at(1);
        ok = true;
    }
    else { /// no block marked or not a preset so move to the end of script to add a new one
        tc.movePosition(QTextCursor::End);
        getTextEdit()->setTextCursor(tc);
        if( engine->cameraID() == "3D" )
          newPreset.sprintf("KeyFrame.%.3d", variableEditor->getKeyFrameCount()+1);
    }

    /// confirm keyframe name
    QString newPresetName = QInputDialog::getText(this, tr("Add Preset"),
                            tr("Change the name for Preset, KeyFrame or Range"), QLineEdit::Normal,
                              newPreset.toLatin1(), &ok);

    if (ok && !newPresetName.isEmpty()) {

        if(newPresetName.contains("KeyFrame.")) { /// adding keyframe
            getTextEdit()->insertPlainText("\n#preset "+newPresetName+"\n" + getCameraSettings() + "\n#endpreset\n");
            addKeyFrame(newPresetName);
        } else if(newPresetName.contains("Range", Qt::CaseInsensitive)) { /// adding parameter easing range
            if(variableEditor->hasEasing()) {
                getTextEdit()->insertPlainText("\n#preset "+newPresetName+"\n" + getEngine()->getCurveSettings().join("\n") + "\n#endpreset\n");
                addKeyFrame(newPresetName);
                return;
            } else {
                QMessageBox::warning ( this, tr("Warning!"), tr("Setup some parameter Easing Curves first!") );
                INFO(tr("%1 Failed!").arg(newPresetName));
                return;
            }
        } else { /// adding a named preset
            getTextEdit()->insertPlainText("\n#preset "+newPresetName+"\n" + getSettings() + "\n#endpreset\n");
        }

        needRebuild(true);
        initializeFragment();
        variableEditor->setPreset(newPresetName);

        INFO(tr("Added %1").arg(newPresetName));
    }
}

void MainWindow::open()
{
    QString filter = tr("Fragment Source (*.frag);;All Files (*.*)");
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(), filter);
    if (!fileName.isEmpty()) {
        loadFragFile(fileName);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* ev) {

    if (ev->key() == Qt::Key_Escape) {
        toggleFullScreen();
    } else if (ev->key() == Qt::Key_F6) {
        callRedraw();
    } else if (ev->key() == Qt::Key_Space) {
        if(engine->hasFocus()) {
            pausePlay = !pausePlay;
            if(pausePlay && (engine->getState() == DisplayWidget::Animation)) stop();
            else play();
        }
    } else {
        ev->ignore();
    }

}

void MainWindow::clearTextures()
{
    engine->clearTextureCache(0);
}

void MainWindow::bufferSpinBoxChanged(int)
{
    QToolTip::showText(bufferSizeControl->pos(),tr("Set combobox to 'custom-size' to apply size."), 0);
}


bool MainWindow::save()
{
    int index = tabBar->currentIndex();
    if (index == -1) {
        WARNING(tr("No open tab"));
        return false;
    }
    TabInfo t = tabInfo[index];

    if (t.hasBeenSavedOnce) {
        return saveFile(t.filename);
    } else {
        return saveAs();
    }
}

bool MainWindow::saveAs()
{
    int index = tabBar->currentIndex();
    if (index == -1) {
        WARNING(tr("No open tab"));
        return false;
    }

    TabInfo t = tabInfo[index];

    QString filter = tr("Fragment Source (*.frag);;All Files (*.*)");

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), t.filename, filter);
    if (fileName.isEmpty())
        return false;

    if(saveFile(fileName))
    {
        t.filename = fileName;
        return true;
    }
    return false;
}

void MainWindow::about()
{
    QString text = QString("<!DOCTYPE html><html lang=\"%1\">").arg(langID);

    text += tr("<h1>Fragmentarium</h1>"
    "<p>Version %1. </p>").arg(version.toLongString());

    text += tr("<p>An integrated environment for exploring GPU pixel graphics. </p>"
    "<p>Created by Mikael Hvidtfeldt Christensen.<br />Licensed and distributed under the LPGL or GPL license.</p>"
    "<p><b>Notice</b>: some fragment (GLSL) shaders are copyright by other authors, and may carry other licenses. Please check the fragment file header before redistributing."
    "<h1>Acknowledgement</h1>"
    "<p>"
    "Much of the inspiration and formulas for Fragmentarium came from the community at <a href=http://www.fractalforums.com>FractalForums.com</a>, including Tom Beddard, Jan Kadlec, Iñigo Quilez, Buddhi, Jesse, and others. Special thanks goes out to Knighty and Kali for their great fragments. All fragments should include information about their origins - please notify me, if I made any mis-attributions."
    "</p>"
    "<p>The icons used are part of the <a href=\"http://www.everaldo.com/crystal/\">Everaldo: Crystal</a> project. </p>"
    "<p>Fragmentarium is built using the <a href=\"http://trolltech.com/developer/downloads/qt/index\">Qt cross-platform GUI framework</a>. </p>"
    "</p>"
    "<p>"
    "<table cellspacing=20><th colspan=2 align=left>Translations by Fractal Forums users</th>"
    "<tr><td>Russian</td><td align=center>SCORPION</td></tr>"
    "<tr><td>Russian</td><td align=center>Crist-JRoger</td></tr>"
    "<tr><td>German</td><td align=center>Sabine</td></tr>"
    "<tr><td>Dutch</td><td align=center>Sabine</td></tr>"
    "</table>"
    "</p>");

    QMessageBox mb(this);
    mb.setText(text);
    mb.setWindowTitle(tr("About Fragmentarium"));
    mb.setIconPixmap(getMiscDir() + QDir::separator() + "Fragmentarium-sm.png");
    mb.exec();
}

void MainWindow::showControlHelp()
{
  QString text = QString("<!DOCTYPE html><html lang=\"%1\">").arg(langID);
  text += tr("<p>"
  "Notice: the 3D view must have keyboard focus!"
  "</p>"
  "<h2>2D</h2>"
  "<p>"
  "<ul>"
  "<li>Left mousebutton: translate center.</li>"
  "<li>Right mousebutton: zoom.</li>"
  "<li>Wheel: zoom</li>"
  "<li>A/D: left/right</li>"
  "<li>W/S: up/down</li>"
  "<li>Q/E: zoom in/out</li>"
  "</ul>"
  "</p>"
  "<h2>3D</h2>"
  "<p>"
  "<ul>"
  "<li>Left mousebutton: change camera direction.</li>"
  "<li>Right mousebutton: move camera in screen plane.</li>"
  "<li>Left+Right mousebutton: zoom.</li>"
  "<li>Shift+Left mousebutton: rotate object (around origin).</li>"
  "<li>Shift+Alt+Left mousebutton: rotate object (around target).</li>"
  "<li>Shift+Tilde (~) resets the view to look through origin (0,0,0)</li>"
  "<li>Wheel: Move forward/backward</li>"
  "<li>W/S: move forward/back.</li>"
  "<li>A/D: move left/right.</li>"
  "<li>Q/E: roll</li>"
  "<li>1/3: increase/decrease step size x2</li>"
  "<li>2/X: increase/decrease step size x10</li>"
  "<li>Shift+Wheel: change step size</li>"
  "<li>T/H: move up/down.</li>"
  "<li>R/F: yaw</li>"
  "<li>T/G: pitch</li>"
  "</ul>"
  "</p>"
  "<h2>Sliders</h2>"
  "<p>"
  "When a (float) slider recieves a Right Mouse Button Click it opens an input dialog to set the step size."
  "</p>");

  text += "</html>";

  QMessageBox mb(this);
  mb.setText(text);
  mb.setWindowTitle(tr("Mouse and Keyboard Control"));
  mb.setIconPixmap(getMiscDir() + QDir::separator() + "Fragmentarium-sm.png");
  mb.exec();
}

void MainWindow::showScriptingHelp()
{
  QString reqObj = sender()->objectName();

  QString text = QString("<!DOCTYPE html><html lang=\"%1\">").arg(langID);
  if( reqObj == "scriptingGeneralAction")
    text += tr("<h2>General commands</h2>"
    "<p>"
    "<ul>"
    "<p><li><b>void setFrame(int);</b></li>"
    "Sets the current frame number.</p>"

    "<p><li><b>int getFrame();</b></li>"
    "Returns the current frame number.</p>"

    "<p><li><b>void loadFragFile(String);</b></li>"
    "Opens a new editor tab, loads the named fragment file, initializes default preset,<br>"
    "initializes keyframes and easing curves if the file contains these settings.</p>"

    "<p><li><b>bool initializeFragment();</b></li>"
    "Returns success or fail.<br>"
    "Must be called after altering a locked variable before rendering an image.</p>"
    "</ul>"
    "</p>");
  if( reqObj == "scriptingParameterAction")
    text += tr("<h2>Parameter commands</h2>"
    "<p>"
    "<ul>"
    "<p><li><b>void setParameter(String);</b></li>"
    "Set a parameter from String in the form of \"parameter = value\" also accepts parameter file formated string.</p>"
    "<p><li><b>void setParameter(String, bool);</b></li>"
    "Sets a boolean parameter where String is the parameter name and bool is TRUE or FALSE</p>"
    "<p><li><b>void setParameter(String, int);</b></li>"
    "Sets an integer parameter where String is the parameter name and int is any integer.</p>"
    "<p><li><b>void setParameter(String, x);</b></li>"
    "Sets a float parameter where String is the parameter name and x is any floating point number.</p>"
    "<p><li><b>void setParameter(String, x, y);</b></li>"
    "Sets a float2 parameter where String is the parameter name and x,y are any floating point numbers.</p>"
    "<p><li><b>void setParameter(String, x, y, z);</b></li>"
    "Sets a float3 parameter where String is the parameter name and x,y,z are any floating point numbers.</p>"
    "<p><li><b>void setParameter(String, x, y, z, w);</b></li>"
    "Sets a float4 parameter where String is the parameter name and x,y,z,w are any floating point numbers.</p>"
    "<p><li><b>String getParameter(String);</b></li>"
    "Returns a string representing the value(s) for the named parameter, user must parse this into usable values.</p>"
    "<p><li><b>void applyPresetByName(String);</b></li>"
    "Applies the named preset.</p>"
    "</ul>"
    "</p>");
  if( reqObj == "scriptingHiresAction")
    text += tr("<h2>Hires image and animation dialog commands</h2>"
    "<p>"
    "<ul>"
    "<p><li><b>void setAnimationLength(int);</b></li>"
    "Sets the total animation duration in seconds.</p>"
    "<p><li><b>void setTileWidth(int);</b></li>"
    "<li><b>void setTileHeight(int);</b></li>"
    "Sets the tile width and height.</p>"
    "<p><li><b>void setTileMax(int);</b></li>"
    "Sets the number of row and column tiles, this value squared = total tiles.</p>"
    "<p><li><b>void setSubFrames(int);</b></li>"
    "Sets the number of frames to accumulate.</p>"
    "<p><li><b>void setOutputBaseFileName(String);</b></li>"
    "Sets the filename for saved image,<br>"
    "if script has total control this must be set by the script for every frame,<br>"
    "if animation is using frag file settings, keyframes etc., then this only needs to be set once to basename and Fragmentarium will add an index padded to 5 digits.</p>"
    "<p><li><b>void setFps(int);</b></li>"
    "Sets the frames per second for rendering.</p>"
    "<p><li><b>void setStartFrame(int);</b></li>"
    "Sets the start frame number for rendering a range of frames.</p>"
    "<p><li><b>void setEndFrame(int);</b></li>"
    "Sets the end frame number for rendering a range of frames.</p>"
    "<p><li><b>void setAnimation(bool);</b></li>"
    "FALSE sets animation to script control exclusively.<br>"
    "TRUE enables control from keyframes and easing curves.</p>"
    "<p><li><b>void setPreview(bool);</b></li>"
    "TRUE will preview frames in a window on the desktop instead of saving image files.<br>"
    "WARNING!!! this will open a window FOR EACH FRAME and close it when the next one is ready for display.</p>"
    "</ul>"
    "</p>");
  if( reqObj == "scriptingControlAction")
    text += tr("<h2>Control commands</h2>"
    "<p>"
    "<ul>"
    "<p><li><b>bool scriptRunning();</b></li>"
    "Returns FALSE when the user selects the [Stop] button in the script editor."
    "For user implemented test in script to break out of the script control loop.</p>"
    "<p><li><b>void stopScript();</b></li>"
    "For user implemented test in script to break out of the script control loop or error like file not found, initialization fail etc.</p>"
    "<p><li><b>void tileBasedRender();</b></li>"
    "Begins rendering the current frame or range of frames applying the current state for keyframes and active easing settings.</p>"

    "</ul>"
    "</p>");

  text += "</html>";

  QMessageBox mb(this);
  mb.setText(text);
  mb.setWindowTitle(tr("Fragmentarium script commands."));
  mb.setIconPixmap(getMiscDir() + QDir::separator() + "Fragmentarium-sm.png");
  mb.exec();

}

void MainWindow::documentWasModified()
{
    if (tabBar->currentIndex() < 0) return;
    // when all is undone
    if(tabInfo[tabBar->currentIndex()].textEdit->document()->availableUndoSteps() == 0)
        tabInfo[tabBar->currentIndex()].unsaved = false;
    else
        tabInfo[tabBar->currentIndex()].unsaved = true;

    if (tabBar->currentIndex() > tabInfo.size()) return;

    TabInfo t = tabInfo[tabBar->currentIndex()];
    QString tabTitle = QString("%1%2").arg(strippedName(t.filename)).arg(t.unsaved ? "*" : "");
    setWindowTitle(QString("%1 - %2").arg(tabTitle).arg("Fragmentarium"));
    stackedTextEdits->setCurrentWidget(t.textEdit);
    tabBar->setTabText(tabBar->currentIndex(), tabTitle);

    highlightBuildButton(tabInfo[tabBar->currentIndex()].unsaved);

    if(tabInfo[tabBar->currentIndex()].textEdit->fh->changeGLSLVersion()) {
        tabInfo[tabBar->currentIndex()].textEdit->fh->rehighlight();
    }
}

void MainWindow::init()
{
    MaxRecentFiles = 5;

    lastStoredTime = 0;
    engine = 0;

    setAcceptDrops(true);
    lastTime = new QTime();
    lastTime->start();

    needRebuild(true);
    hasBeenResized = true;

    oldDirtyPosition = -1;
    setFocusPolicy(Qt::StrongFocus);

    version = Version(2, 0, 0, 180102, " (\"bourbon\")");
    setAttribute(Qt::WA_DeleteOnClose);

    splitter = new QSplitter(this);
    splitter->setObjectName(QString::fromUtf8("splitter"));
    splitter->setOrientation(Qt::Horizontal);

    stackedTextEdits = new QStackedWidget(splitter);

    /// Default QGLFormat settings
    //    Double buffer: Enabled.
    //    Depth buffer: Enabled.
    //    RGBA: Enabled (i.e., color index disabled).
    //    Alpha channel: Disabled.
    //    Accumulator buffer: Disabled.
    //    Stencil buffer: Enabled.
    //    Stereo: Disabled.
    //    Direct rendering: Enabled.
    //    Overlay: Disabled.
    //    Plane: 0 (i.e., normal plane).
    //    Multisample buffers: Disabled.

    QGLFormat fmt;
    fmt.setDoubleBuffer(false);
    fmt.setStencil(false);
    fmt.setDepthBufferSize(32);
    QSettings settings;
    int i = settings.value("refreshRate", 20).toInt();
    fmt.setSwapInterval(i);

    engine = new DisplayWidget(fmt, this, splitter);
    engine->makeCurrent();
    engine->show();
    if(!engine->init()) CRITICAL(tr("Engine failed to start!"));
    engine->updateRefreshRate();

    tabBar = new QTabBar(this);

    tabBar->setTabsClosable(true);
    connect(tabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

    fpsLabel = new QLabel(this);
    statusBar()->addPermanentWidget(fpsLabel);

    QFrame* f = new QFrame(this);
    frameMainWindow = new QVBoxLayout();
    frameMainWindow->setSpacing(0);
    frameMainWindow->setMargin(4);
    f->setLayout(frameMainWindow);
    f->layout()->addWidget(tabBar);
    f->layout()->addWidget(splitter);
    setCentralWidget(f);

    createActions();

    QDir d(getExamplesDir());

    // Log widget (in dockable window)
    dockLog = new QDockWidget(this);
    dockLog->setWindowTitle(tr("Log"));
    dockLog->setObjectName(QString::fromUtf8("dockWidget"));
    dockLog->setAllowedAreas(Qt::BottomDockWidgetArea);
    QWidget* dockLogContents = new QWidget(dockLog);
    dockLogContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
    QVBoxLayout* vboxLayout1 = new QVBoxLayout(dockLogContents);
    vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
    vboxLayout1->setContentsMargins(0, 0, 0, 0);

    logger = new ListWidgetLogger(dockLog);
    vboxLayout1->addWidget(logger->getListWidget());
    dockLog->setWidget(dockLogContents);
    addDockWidget(Qt::BottomDockWidgetArea, dockLog);

    // Variable editor (in dockable window)
    editorDockWidget = new QDockWidget(this);
    editorDockWidget->setMinimumWidth(250);
    editorDockWidget->setWindowTitle(tr("Variable Editor (uniforms)"));
    editorDockWidget->setObjectName(QString::fromUtf8("editorDockWidget"));
    editorDockWidget->setAllowedAreas(Qt::RightDockWidgetArea);
    QWidget* editorLogContents = new QWidget(dockLog);
    editorLogContents->setObjectName(QString::fromUtf8("editorLogContents"));
    QVBoxLayout* vboxLayout2 = new QVBoxLayout(editorLogContents);
    vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
    vboxLayout2->setContentsMargins(0, 0, 0, 0);

    variableEditor = new VariableEditor(editorDockWidget, this);
    variableEditor->setMinimumWidth(250);
    vboxLayout2->addWidget(variableEditor);
    editorDockWidget->setWidget(editorLogContents);
    addDockWidget(Qt::RightDockWidgetArea, editorDockWidget);
    connect(variableEditor, SIGNAL(changed(bool)), this, SLOT(variablesChanged(bool)));
    connect(editorDockWidget, SIGNAL(topLevelChanged(bool)), this, SLOT(veDockChanged(bool))); // 05/22/17 Sabine ;)
    
    editorDockWidget->setHidden(true);
    setMouseTracking(true);

    INFO(tr("Welcome to Fragmentarium version %1. A Syntopia Project.").arg(version.toLongString()));
    WARNING(tr("This is an experimental 3Dickulus build."));
    INFO("");

    fullScreenEnabled = false;
    createOpenGLContextMenu();

    connect(this->tabBar, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    readSettings();

    {
        QSettings settings;
        if (settings.value("firstRun", true).toBool()) {
            removeSplash();
            showWelcomeNote();
        }
        settings.setValue("firstRun", false);
    }

    {
        QSettings settings;
        if (settings.value("isStarting", false).toBool()) {
            QString s = tr("It looks like the Fragmentarium program crashed during the last startup.\n"
                        "\nTo prevent repeated crashes, you may choose to disable 'Autorun on Load'."
                        "\nThis option may be re-enabled through Preferences");


            removeSplash();
            QMessageBox msgBox;
            msgBox.setText(s);
            QAbstractButton* b = msgBox.addButton(tr("Disable Autorun"),QMessageBox::AcceptRole);
            msgBox.addButton(tr("Enable Autorun"),QMessageBox::RejectRole);

            msgBox.exec();
            if (msgBox.clickedButton() == b) {
                settings.setValue("autorun", false);
            } else {
                settings.setValue("autorun", true);
            }
        }
        settings.setValue("isStarting", true);
    }

    createToolBars();
    createStatusBar();
    createMenus();
    renderModeChanged();

    // test for nVidia card and GL > 4.0
#ifdef NVIDIAGL4PLUS
    if( !(engine->context()->format().majorVersion() > 3) || !engine->foundnV) {
        if( !(engine->context()->format().minorVersion() > 0) ) {
            QMessageBox::critical(0, tr("OpenGL features missing"),
                                  tr("Failed to resolve OpenGL functions required"" to enable AsmBrowser"));
        }
        if(asmAction)editMenu->removeAction(asmAction);
    }
#endif // NVIDIAGL4PLUS

#ifdef USE_OPEN_EXR
#ifndef Q_OS_WIN
initTools();
#endif // UNIX
#endif // USE_OPEN_EXR

    highlightBuildButton( !(QSettings().value("autorun", true).toBool()) );
    setupScriptEngine();
    play();
}

#ifdef USE_OPEN_EXR
void MainWindow::initTools() {

    QStringList filters;
    
    if(exrToolsMenu == 0)
      exrToolsMenu = menuBar()->addMenu(tr("EXR &Tools"));

    QDir exrbp( exrBinaryPath.first() );
    
    while (!exrbp.exists()) {
        exrBinaryPath.removeFirst();
        exrbp.setPath( exrBinaryPath.first() );
    }

    if (!exrbp.exists()) {
        QAction* a = new QAction(tr("Unable to locate: ")+exrbp.absolutePath(), this);
        a->setEnabled(false);
        exrToolsMenu->addAction(a);
    } else {

        exrToolsMenu->clear();

        // -- OpenEXR binary tools Menu --
        if(exrToolsMenu != 0) {
            filters.clear();
            filters << "exr*";

            QString path = exrbp.absolutePath();
            QDir dir(path);

            dir.setNameFilters(filters);

            QStringList sl = dir.entryList();
            if(sl.size() == 0) {
                QAction* a = new QAction(tr("Unable to locate OpenEXR binaries !!!"), this);
                a->setEnabled(false);
                exrToolsMenu->addAction(a);
            } else {
                for (int i = 0; i < sl.size(); i++) {
                    QAction* a = new QAction(sl[i], this);
                    QString absPath = QDir(path ).absoluteFilePath(sl[i]);

                    a->setData(absPath);
                    a->setObjectName(absPath);

                    connect(a, SIGNAL(triggered()), this, SLOT(runTool()));
                    exrToolsMenu->addAction(a);
                }
            }
        }
    }
}

void MainWindow::runTool() {
    QString cmnd = sender()->objectName();
        // execute once with -h option and capture the output
        cmnd += " -h &> .htxt"; // > filename 2>&1
        system( cmnd.toStdString().c_str() );

        // open the resulting textfile and parse for command information
        QFile file(".htxt");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;
        // grab the general help text
        QString helpText;
        while (!file.atEnd()) {
            QByteArray line = file.readLine();
            if( QString(line).contains("Options") && QString(line).contains(":")) break;
            helpText += QString(line);
        }
        // grab the option details
        QString detailedText;
        while (!file.atEnd()) {
            QByteArray line = file.readLine();
            if( QString(line).contains("Options:") ) continue;
            detailedText += QString(line);
        }
        file.remove();
        
        // display for user
        QMessageBox msgBox;
        msgBox.setText(helpText);
        msgBox.setDetailedText(detailedText);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
}

#endif // USE_OPEN_EXR

void MainWindow::showWelcomeNote() {
    QString s =
        tr("This is your first run of Fragmentarium.\nPlease read this:\n\n"
        "(1) Fragmentarium requires a decent GPU, preferably a NVIDIA or ATI discrete graphics card with recent drivers.\n\n"
        "(2) On Windows Vista and 7, there is a built-in GPU watchdog, requiring each frame to render in less than 2 seconds. Some fragments may exceed this limit, especially on low-end graphics cards. It is possible to circumvent this, see the Fragmentarium FAQ (in the Help menu) for more information.\n\n"
        "(3) Many examples in Fragmentarium use progressive rendering, which requires Fragmentarium to run in Continuous mode. When running in this mode, Fragmentarium uses 100% GPU power (but you may use the 'Subframe : Max' spinbox to limit the number of frames rendered. A value of zero means there is no maximum count.)\n\n");
    QMessageBox msgBox;
    msgBox.setText(s);
    msgBox.exec();

}

// M Benesi "Spray gun" grabFeedbackSettings
QVector4D MainWindow::grabFeedbackSettings() {

    VariableEditor *ve = getVariableEditor();
    int appI=0;
    IntWidget* widgeti = dynamic_cast<IntWidget*>(ve->getWidgetFromName("ApplyOnIteration"));
    if (!widgeti) WARNING("Could not find ApplyOnIteration interface widget");
    else appI = widgeti->getValue();

    int form=0;
    widgeti = dynamic_cast<IntWidget*>(ve->getWidgetFromName("FormulaType"));
    if (!widgeti ) WARNING("Could not find FormulaType interface widget");
    else form = widgeti->getValue();

    int appT=0;
    widgeti = dynamic_cast<IntWidget*>(ve->getWidgetFromName("ApplicationType"));
    if (!widgeti) WARNING("Could not find ApplicationType interface widget");
    else appT = widgeti->getValue();

    double fv1=0.0;
    FloatWidget* widgetf = dynamic_cast<FloatWidget*>(ve->getWidgetFromName("FeedbackVariable1"));
    if (!widgetf) WARNING("Could not find FeedbackVariable1 interface widget");
    else fv1 = widgetf->getValue();

    QVector4D floop = QVector4D(appI,form,appT,fv1);
    return floop;

}

// M Benesi "Spray gun" grabFeedbackSettings2
QVector4D MainWindow::grabFeedbackSettings2() {

    VariableEditor *ve = getVariableEditor();
    double fr=0.0;
    FloatWidget* widget = dynamic_cast<FloatWidget*>(ve->getWidgetFromName("FeedbackRadius"));
    if (!widget) WARNING("Could not find FeedbackRadius interface widget");
    else fr = widget->getValue();

    double fs=0.0;
    widget = dynamic_cast<FloatWidget*>(ve->getWidgetFromName("FeedbackStrength"));
    if (!widget ) WARNING("Could not find FeedbackStrength interface widget");
    else fs = widget->getValue();
//     if( engine->wantZappaMinus ) {
//         fs=-fs;
//     }

    double fv=0.0;
    widget = dynamic_cast<FloatWidget*>(ve->getWidgetFromName("FeedbackVariable2"));
    if (!widget) WARNING("Could not find FeedbackVariable2 interface widget");
    else fv = widget->getValue();

    double fv2=0.0;
    widget = dynamic_cast<FloatWidget*>(ve->getWidgetFromName("FeedbackVariable3"));
    if (!widget) WARNING("Could not find FeedbackVariable3 interface widget");
    else fv2 = widget->getValue();

    QVector4D floop = QVector4D(fr,fs,fv,fv2);
    return floop;

}

// M Benesi "Spray gun" grabFeedbackRotation
QVector4D MainWindow::grabFeedbackRotation() {

    VariableEditor *ve = getVariableEditor();
    QVector3D fr=QVector3D(0,0,0);
    Float3Widget* widget = dynamic_cast<Float3Widget*>(ve->getWidgetFromName("FBRotVector"));
    if (!widget) WARNING("Could not find FBRotVector interface widget");
    else fr = widget->getValue();

    double fs=0.0;
    FloatWidget* fwidget = dynamic_cast<FloatWidget*>(ve->getWidgetFromName("FBRotAngle"));
    if (!fwidget ) WARNING("Could not find FBRotAngle interface widget");
    else fs = fwidget->getValue();

    IntWidget* widgeti = dynamic_cast<IntWidget*>(ve->getWidgetFromName("FeedBackCutOff"));
    if (!widgeti) WARNING("Could not find FeedBackCutOff interface widget");
    else feedbackmaxindex = widgeti->getValue();

    QVector4D floop = QVector4D(fr.x(),fr.y(),fr.z(),fs);
    return floop;

}

void MainWindow::setFeedbackArrayData() {
    QVector4D feedbacksettings = grabFeedbackSettings();
    feedcontrol1[feedbackindex] = QVector4D (feedbacksettings.x(),feedbacksettings.y(),feedbacksettings.z(),feedbacksettings.w());
    QVector4D feedbacksettings2 = grabFeedbackSettings2();
    feedcontrol2[feedbackindex] = QVector4D (feedbacksettings2.x(),feedbacksettings2.y(),feedbacksettings2.z(),feedbacksettings2.w());
    QVector4D feedbackrotation = grabFeedbackRotation();
    feedrotation[feedbackindex] = QVector4D (feedbackrotation.x(),feedbackrotation.y(),feedbackrotation.z(),feedbackrotation.w());
}

void MainWindow::setFeedbackParameters() {
    variableEditor->blockSignals(true);
    setParameter("ApplyOnIteration", (int)feedcontrol1[feedbackindex].x());
    setParameter("FormulaType", (int)feedcontrol1[feedbackindex].y());
    setParameter("ApplicationType", (int)feedcontrol1[feedbackindex].z());
    setParameter("FeedbackVariable1", (float)feedcontrol1[feedbackindex].w());
    setParameter("FeedbackRadius", (float)feedcontrol2[feedbackindex].x());
    setParameter("FeedbackStrength", (float)feedcontrol2[feedbackindex].y());
    setParameter("FeedbackVariable2", (float)feedcontrol2[feedbackindex].z());
    setParameter("FeedbackVariable3", (float)feedcontrol2[feedbackindex].w());
    setParameter("FBRotVector",feedrotation[feedbackindex].x(),feedrotation[feedbackindex].y(),feedrotation[feedbackindex].z());
    setParameter("FBRotAngle", (float)feedrotation[feedbackindex].w());
    variableEditor->blockSignals(false);
}

void MainWindow::enableZappaTools( bool doZap )
{
    if(doZap) {
        zappaToolBar->show();
        if(!fileMenu->actions().contains(loadfdbkAction))
            fileMenu->addAction(loadfdbkAction);
        if(!fileMenu->actions().contains(savefdbkAction))
            fileMenu->addAction(savefdbkAction);
        if(!zappaToolBar->actions().contains(zapLock))
            zappaToolBar->addAction(zapLock);
        if(!zappaToolBar->actions().contains(zapIndx))
            zappaToolBar->addAction(zapIndx);

        zapIndex->setMaximum(feedbackcount);
        zapIndex->setValue(feedbackindex);

        if(zapCheck->isChecked())
            statusBar()->showMessage(QString("Current Zappa:%1, %2 : %3,%4,%5").
                                     arg(feedbackindex).
                                     arg(feedbackcount).
                                     arg(feedbackcrds[feedbackindex].x()).
                                     arg(feedbackcrds[feedbackindex].y()).
                                     arg(feedbackcrds[feedbackindex].z()), 0);

        setZapLock(zapCheck->isChecked());

        if(!zappaToolBar->actions().contains(zapClr))
            zappaToolBar->addAction(zapClr);

    } else {

        if(fileMenu->actions().contains(loadfdbkAction))
            fileMenu->removeAction(loadfdbkAction);
        if(fileMenu->actions().contains(savefdbkAction))
            fileMenu->removeAction(savefdbkAction);
        if(zappaToolBar->actions().contains(zapLock))
            zappaToolBar->removeAction( zapLock );
        if(zappaToolBar->actions().contains(zapIndx))
            zappaToolBar->removeAction( zapIndx );
        if(zappaToolBar->actions().contains(zapClr))
            zappaToolBar->removeAction( zapClr );
        zappaToolBar->hide();
        setZapLock(false);
    }

}

void MainWindow::setUserUniforms(QOpenGLShaderProgram* shaderProgram) {
    
    if (!variableEditor || !shaderProgram) return;    
    variableEditor->setUserUniforms(shaderProgram);
    if(feedbackcount != 0)
    setFeedbackUniforms(shaderProgram);
}

void MainWindow::setFeedbackUniforms(QOpenGLShaderProgram* shaderProgram) {

    // M Benesi "Spray gun" setFeedbackUniforms
    // this sets the feedback uniform to mouseXY + depth@mouseXY
    int uloc1 = shaderProgram->uniformLocation( "feedbackcrds" );
    int uloc2 = shaderProgram->uniformLocation( "feedbackcontrol1" );
    int uloc3 = shaderProgram->uniformLocation( "feedbackcontrol2" );
    int uloc4 = shaderProgram->uniformLocation( "feedbackcount" );
    int uloc5 = shaderProgram->uniformLocation( "feedbackrotation" );
    int uloc6 = shaderProgram->uniformLocation( "FeedBackCutOff" );

    // need to test for these too
    // int ApplyOnIteration
    // int FormulaType
    // int ApplicationType
    // float FeedbackVariable1

    // float FeedbackRadius
    // float FeedbackStrength
    // float FeedbackVariable2
    // float FeedbackVariable3

    // QVector3D FBRotVector
    // float FBRotAngle

    fragHasFeedbackVars=false;

    if( uloc1 != -1 && uloc2 != -1 && uloc3 != -1 && uloc4 != -1 && uloc5 != -1 && uloc6 != -1 ) {
        fragHasFeedbackVars=true;

        if( engine->wantZappaClear ) {
            int i=0;
            feedbackindex = 0;
            feedbackcount = 0;
            while (i<feedbackmaxindex) {
                feedbackcrds[i]=QVector3D(333,0,0);
                feedcontrol1[i]=feedcontrol2[i]=feedrotation[i]=QVector4D(333,0,0,0);
                i++;
            }
        }
        else if( engine->wantZappaDelete) {

            feedbackcrds[feedbackcount]=QVector3D(333,0,0);
            feedcontrol1[feedbackcount]=feedcontrol2[feedbackcount]=feedrotation[feedbackcount]=QVector4D(333,0,0,0);
            feedbackcount--;

            if(feedbackcount<0) feedbackcount=0;
            if(feedbackindex>feedbackcount)feedbackindex=feedbackcount;

        }
        else if( (engine->wantZappaAdd) && (feedbackindex<feedbackmaxindex) ) {
            feedbackindex++;
            QPoint XY = engine->getMXY();
            double zAtXY = engine->getZAtMXY();
            QVector3D coords = engine->getCameraControl()->screenTo3D(XY.x(),XY.y(),zAtXY);
            if(!engine->getZapLock())
                feedbackcrds[feedbackindex] = QVector3D(coords.x(),coords.y(),coords.z());
            QVector4D feedbacksettings = grabFeedbackSettings();
            feedcontrol1[feedbackindex] = QVector4D (feedbacksettings.x(),feedbacksettings.y(),feedbacksettings.z(),feedbacksettings.w());
            QVector4D feedbacksettings2 = grabFeedbackSettings2();
            feedcontrol2[feedbackindex] = QVector4D (feedbacksettings2.x(),feedbacksettings2.y(),feedbacksettings2.z(),feedbacksettings2.w());
            QVector4D feedbackrotation = grabFeedbackRotation();
            feedrotation[feedbackindex] = QVector4D (feedbackrotation.x(),feedbackrotation.y(),feedbackrotation.z(),feedbackrotation.w());

            if(feedbackindex>feedbackcount)feedbackcount=feedbackindex;
            if(feedbackcount>feedbackmaxindex-1) feedbackcount=feedbackmaxindex-1;
            if(feedbackindex>feedbackmaxindex-1)feedbackindex=0;

        }

        shaderProgram->setUniformValueArray(uloc1, feedbackcrds , feedbackmaxindex);
        shaderProgram->setUniformValueArray(uloc2, feedcontrol1 , feedbackmaxindex);
        shaderProgram->setUniformValueArray(uloc3, feedcontrol2 , feedbackmaxindex);
        shaderProgram->setUniformValueArray(uloc5, feedrotation , feedbackmaxindex);
        shaderProgram->setUniformValue(uloc4, feedbackcount);

        enableZappaTools(fragHasFeedbackVars);
    } else engine->setZapLock(false);

}

void MainWindow::variablesChanged(bool lockedChanged) {
    if(lockedChanged) highlightBuildButton(true);
    if( fragHasFeedbackVars ) setFeedbackArrayData();
    engine->uniformsHasChanged();
}

void MainWindow::createOpenGLContextMenu() {
    openGLContextMenu = new QMenu();
    openGLContextMenu->addAction(fullScreenAction);
    openGLContextMenu->addAction(screenshotAction);
    engine->setContextMenu(openGLContextMenu);
}

void MainWindow::toggleFullScreen() {
    if (fullScreenEnabled) {
        frameMainWindow->setMargin(4);
        showNormal();
        fullScreenEnabled = false;
        fullScreenAction->setChecked(false);
        stackedTextEdits->show();
        dockLog->show();
        menuBar()->show();
        statusBar()->show();
        tabBar->show();
    } else {
        frameMainWindow->setMargin(0);
        fullScreenAction->setChecked(true);
        fullScreenEnabled = true;

        tabBar->hide();
        stackedTextEdits->hide();
        dockLog->hide();
        menuBar()->hide();
        statusBar()->hide();
        showFullScreen();
    }
}

void MainWindow::createActions()
{
    fullScreenAction = new QAction(tr("Fullscreen (ESC key toggles)"), this);
    connect(fullScreenAction, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));

    screenshotAction = new QAction(tr("Save Screen Shot..."), this);
    connect(screenshotAction, SIGNAL(triggered()), this, SLOT(makeScreenshot()));

    newAction = new QAction(QIcon(":/Icons/new.png"), tr("&New"), this);
    newAction->setShortcut(tr("Ctrl+N"));
    newAction->setStatusTip(tr("Create a new file"));
    connect(newAction, SIGNAL(triggered()), this, SLOT(newFile()));

    openAction = new QAction(QIcon(":/Icons/open.png"), tr("&Open..."), this);
    openAction->setShortcut(tr("Ctrl+O"));
    openAction->setStatusTip(tr("Open an existing file"));
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

    saveAction = new QAction(QIcon(":/Icons/save.png"), tr("&Save"), this);
    saveAction->setShortcut(tr("Ctrl+S"));
    saveAction->setStatusTip(tr("Save the script to disk"));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAction = new QAction(QIcon(":/Icons/filesaveas.png"), tr("Save &As..."), this);
    saveAsAction->setShortcut(tr("Ctrl+Shift+S"));
    saveAsAction->setStatusTip(tr("Save the script under a new name"));
    connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));

// M Benesi "spray gun" load/save data
    loadfdbkAction = new QAction(QIcon(":/Icons/open.png"), tr("Load &Zappas"), this);
    loadfdbkAction->setShortcut(tr("Ctrl+Alt+L"));
    loadfdbkAction->setStatusTip(tr("Load the Zappas from disk"));
    connect(loadfdbkAction, SIGNAL(triggered()), this, SLOT(loadFeedback()));
    savefdbkAction = new QAction(QIcon(":/Icons/save.png"), tr("Save &Zappas"), this);
    savefdbkAction->setShortcut(tr("Ctrl+Alt+S"));
    savefdbkAction->setStatusTip(tr("Save the Zappas to disk"));
    connect(savefdbkAction, SIGNAL(triggered()), this, SLOT(saveFeedback()));

    closeAction = new QAction(QIcon(":/Icons/fileclose.png"), tr("&Close Tab"), this);
    closeAction->setShortcut(tr("Ctrl+W"));
    closeAction->setStatusTip(tr("Close this tab"));
    connect(closeAction, SIGNAL(triggered()), this, SLOT(closeTab()));

    exitAction = new QAction(QIcon(":/Icons/exit.png"), tr("E&xit Application"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    cutAction = new QAction(QIcon(":/Icons/cut.png"), tr("Cu&t"), this);
    cutAction->setShortcut(tr("Ctrl+X"));
    cutAction->setStatusTip(tr("Cut the current selection's contents to the "
                               "clipboard"));
    connect(cutAction, SIGNAL(triggered()), this, SLOT(cut()));

    copyAction = new QAction(QIcon(":/Icons/copy.png"), tr("&Copy"), this);
    copyAction->setShortcut(tr("Ctrl+C"));
    copyAction->setStatusTip(tr("Copy the current selection's contents to the "
                                "clipboard"));
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));

    pasteAction = new QAction(QIcon(":/Icons/paste.png"), tr("&Paste"), this);
    pasteAction->setShortcut(tr("Ctrl+V"));
    pasteAction->setStatusTip(tr("Paste the clipboard's contents into the current ""selection"));
    connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste()));

    findAction = new QAction(QIcon(":/Icons/find.png"), tr("&Find"), this);
    findAction->setShortcut(tr("Ctrl+F"));
    findAction->setStatusTip(tr("Finds the current selection"));
    connect(findAction, SIGNAL(triggered()), this, SLOT(search()));
#ifdef NVIDIAGL4PLUS
    asmAction = new QAction(QIcon(":/Icons/find.png"), tr("Shader &Asm"), this);
    asmAction->setShortcut(tr("Ctrl+A"));
    asmAction->setStatusTip(tr("NV objdump"));
    connect(asmAction, SIGNAL(triggered()), this, SLOT(dumpShaderAsm()));
#endif // NVIDIAGL4PLUS
    scriptAction = new QAction(tr("&Edit Cmd Script"), this);
    scriptAction->setShortcut(tr("Ctrl+R"));
    scriptAction->setStatusTip(tr("Edit the currently loaded script"));
    connect(scriptAction, SIGNAL(triggered()), this, SLOT(editScript()));

    renderAction = new QAction(QIcon(":/Icons/render.png"), tr("&Build System"), this);
    renderAction->setShortcut(tr("F5"));
    renderAction->setStatusTip(tr("Render the current ruleset"));
    connect(renderAction, SIGNAL(triggered()), this, SLOT(initializeFragment()));
    // not sure why but connecting twice makes textures persitent ???
    connect(renderAction, SIGNAL(triggered()), this, SLOT(initializeFragment()));

    videoEncoderAction = new QAction(QIcon(":/Icons/render.png"), tr("&Video Encoding"), this);
    videoEncoderAction->setStatusTip(tr("Encode rendered frames to video"));
    connect(videoEncoderAction, SIGNAL(triggered()), this, SLOT(videoEncoderRequest()));

    aboutAction = new QAction(QIcon(":/Icons/documentinfo.png"), tr("&About"), this);
    aboutAction->setStatusTip(tr("Shows the About box"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    welcomeAction = new QAction(QIcon(":/Icons/documentinfo.png"), tr("Show Welcome Note"), this);
    connect(welcomeAction, SIGNAL(triggered()), this, SLOT(showWelcomeNote()));

    controlAction = new QAction(QIcon(":/Icons/documentinfo.png"), tr("&Mouse and Keyboard Help"), this);
    controlAction->setStatusTip(tr("Shows information about how to control Fragmentarium"));
    connect(controlAction, SIGNAL(triggered()), this, SLOT(showControlHelp()));

    scriptingGeneralAction = new QAction(QIcon(":/Icons/documentinfo.png"), tr("&Scripting General Help"), this);
    scriptingGeneralAction->setStatusTip(tr("Shows information about how to control Fragmentarium via Script"));
    scriptingGeneralAction->setObjectName("scriptingGeneralAction");
    connect(scriptingGeneralAction, SIGNAL(triggered()), this, SLOT(showScriptingHelp()));

    scriptingParameterAction = new QAction(QIcon(":/Icons/documentinfo.png"), tr("&Scripting Parameter Help"), this);
    scriptingParameterAction->setStatusTip(tr("Shows information about how to control Fragmentarium via Script"));
    scriptingParameterAction->setObjectName("scriptingParameterAction");
    connect(scriptingParameterAction, SIGNAL(triggered()), this, SLOT(showScriptingHelp()));

    scriptingHiresAction = new QAction(QIcon(":/Icons/documentinfo.png"), tr("&Scripting Image Anim Dialog Help"), this);
    scriptingHiresAction->setStatusTip(tr("Shows information about how to control Fragmentarium via Script"));
    scriptingHiresAction->setObjectName("scriptingHiresAction");
    connect(scriptingHiresAction, SIGNAL(triggered()), this, SLOT(showScriptingHelp()));

    scriptingControlAction = new QAction(QIcon(":/Icons/documentinfo.png"), tr("&Scripting Control Help"), this);
    scriptingControlAction->setStatusTip(tr("Shows information about how to control Fragmentarium via Script"));
    scriptingControlAction->setObjectName("scriptingControlAction");
    connect(scriptingControlAction, SIGNAL(triggered()), this, SLOT(showScriptingHelp()));

    clearTexturesAction = new QAction(tr("Clear Texture Cache"), this);
    connect(clearTexturesAction, SIGNAL(triggered()), this, SLOT(clearTextures()));

    sfHomeAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("&Project Homepage (web link)"), this);
    sfHomeAction->setStatusTip(tr("Open the project page in a browser."));
    connect(sfHomeAction, SIGNAL(triggered()), this, SLOT(launchSfHome()));

    referenceAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("&Fragmentarium@FractalForums (web link)"), this);
    referenceAction->setStatusTip(tr("Open a FractalForums.com Fragmentarium web page in a browser."));
    connect(referenceAction, SIGNAL(triggered()), this, SLOT(launchReferenceHome()));

    referenceAction2 = new QAction(QIcon(":/Icons/agt_internet.png"), tr("&Fragmentarium 3Dickulus (web link)"), this);
    referenceAction2->setStatusTip(tr("Open a Fragmentarium reference web page in a browser."));
    connect(referenceAction2, SIGNAL(triggered()), this, SLOT(launchReferenceHome2()));

    galleryAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("&Flickr Fragmentarium Group (web link)"), this);
    galleryAction->setStatusTip(tr("Opens the main Flickr group for Fragmentarium creations."));
    connect(galleryAction, SIGNAL(triggered()), this, SLOT(launchGallery()));

    glslHomeAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("&GLSL Specifications (web link)"), this);
    glslHomeAction->setStatusTip(tr("The official specifications for all GLSL versions."));
    connect(glslHomeAction, SIGNAL(triggered()), this, SLOT(launchGLSLSpecs()));

    introAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("Introduction to Distance Estimated Fractals (web link)"), this);
    connect(introAction, SIGNAL(triggered()), this, SLOT(launchIntro()));

    faqAction = new QAction(QIcon(":/Icons/agt_internet.png"), tr("Fragmentarium FAQ (web link)"), this);
    connect(faqAction, SIGNAL(triggered()), this, SLOT(launchFAQ()));

    for (int i = 0; i < MaxRecentFiles; ++i) {
        QAction* a = new QAction(this);
        a->setVisible(false);
        connect(a, SIGNAL(triggered()), this, SLOT(openFile()));
        recentFileActions.append(a);
    }

    qApp->setWindowIcon(QIcon(":/Icons/fragmentarium.png"));
}

void MainWindow::createMenus()
{
    // -- File Menu --
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAction);
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);

    recentFileSeparator = fileMenu->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i) fileMenu->addAction(recentFileActions[i]);
    fileMenu->addSeparator();
    fileMenu->addAction(closeAction);
    fileMenu->addAction(exitAction);

    // -- Edit Menu --
    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAction);
    editMenu->addAction(copyAction);
    editMenu->addAction(pasteAction);
    editMenu->addAction(findAction);
#ifdef NVIDIAGL4PLUS
    editMenu->addAction(asmAction);
#endif // NVIDIAGL4PLUS
    editMenu->addSeparator();
    editMenu->addAction(tr("Indent Script"), this, SLOT(indent()));
    QMenu* m = editMenu->addMenu(tr("Insert Command"));
    createCommandHelpMenu(m, this, this);
    editMenu->addAction(tr("Add Easing Curve"), this, SLOT(setEasing()), QKeySequence("F7"));
    editMenu->addAction(tr("Add Key Frame"), this, SLOT(insertPreset()), QKeySequence("F8"));
    editMenu->addAction(tr("Select Key Frame"), this, SLOT(selectPreset()), QKeySequence("F9"));
    editMenu->addSeparator();
    editMenu->addAction(tr("Preferences..."), this, SLOT(preferences()));

    // -- Render Menu --
    renderMenu = menuBar()->addMenu(tr("&Render"));
    renderMenu->addAction(renderAction);
    renderMenu->addAction(QIcon(":/Icons/render.png"),tr("High Resolution and Animation Render"), this, SLOT(tileBasedRender()));
    renderMenu->addSeparator();
    renderMenu->addAction(tr("Output Preprocessed Script (for Debug)"), this, SLOT(showDebug()));
    renderMenu->addSeparator();
    renderMenu->addAction(fullScreenAction);
    renderMenu->addAction(screenshotAction);
    renderMenu->addAction(scriptAction);
    renderMenu->addAction(videoEncoderAction);

    // -- Parameters Menu --
    parametersMenu = menuBar()->addMenu(tr("&Parameters"));
    parametersMenu->addAction(tr("Reset All"), variableEditor, SLOT(resetUniforms()), QKeySequence("F1"));
    parametersMenu->addSeparator();
    parametersMenu->addAction(tr("Copy Settings"), variableEditor, SLOT(copy()), QKeySequence("F2"));
    parametersMenu->addAction(tr("Copy Group"), variableEditor, SLOT(copyGroup()), QKeySequence("Shift+F2"));
    parametersMenu->addAction(tr("Paste from Clipboard"), variableEditor, SLOT(paste()), QKeySequence("F3"));
    parametersMenu->addAction(tr("Paste from Selected Text"), this, SLOT(pasteSelected()), QKeySequence("F4"));
    parametersMenu->addSeparator();
    parametersMenu->addAction(tr("Save to File"), this, SLOT(saveParameters()));
    parametersMenu->addAction(tr("Load from File"), this, SLOT(loadParameters()));
    parametersMenu->addSeparator();

    // -- Examples Menu --
    QStringList filters;
    QMenu* examplesMenu = menuBar()->addMenu(tr("&Examples"));
    // Scan examples dir...
    QDir d(getExamplesDir());
    filters.clear();
    filters << "*.frag";
    d.setNameFilters(filters);
    if (!d.exists()) {
        QAction* a = new QAction(tr("Unable to locate: ")+d.absolutePath(), this);
        a->setEnabled(false);
        examplesMenu->addAction(a);
    } else {
        // we will recurse the dirs...
        QStack<QString> pathStack;
        pathStack.append(QDir(getExamplesDir()).absolutePath());

        QMap< QString , QMenu* > menuMap;
        while (!pathStack.isEmpty()) {

            QMenu* currentMenu = examplesMenu;
            QString path = pathStack.pop();
            if (menuMap.contains(path)) currentMenu = menuMap[path];
            QDir dir(path);

            QStringList sl = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (int i = 0; i < sl.size(); i++) {
                QMenu* menu = new QMenu(sl[i]);
                QString absPath = QDir(path + QDir::separator() +  sl[i]).absolutePath();
                menuMap[absPath] = menu;
                currentMenu->addMenu(menu);
                menu->setIcon(QIcon(":/Icons/folder.png"));
                pathStack.push(absPath);
            }

            dir.setNameFilters(filters);

            sl = dir.entryList();
            for (int i = 0; i < sl.size(); i++) {
                QAction* a = new QAction(sl[i], this);
                a->setIcon(QIcon(":/Icons/mail_new.png"));


                QString absPath = QDir(path ).absoluteFilePath(sl[i]);

                a->setData(absPath);
                connect(a, SIGNAL(triggered()), this, SLOT(openFile()));
                currentMenu->addAction(a);
            }
        }
    }

    // RMB in menu bar for "windows" menu access
    QMenu* mc = createPopupMenu();
    mc->setTitle(tr("Windows"));
    menuBar()->addMenu(mc);
    
    helpMenu = menuBar()->addMenu(tr("&Help"));

    helpMenu->addAction(aboutAction);
    helpMenu->addAction(welcomeAction);
    helpMenu->addAction(controlAction);
    helpMenu->addAction(scriptingGeneralAction);
    helpMenu->addAction(scriptingParameterAction);
    helpMenu->addAction(scriptingHiresAction);
    helpMenu->addAction(scriptingControlAction);

    helpMenu->addSeparator();
    helpMenu->addMenu(mc); // "windows" menu
    helpMenu->addAction(clearTexturesAction);
    helpMenu->addSeparator();

    helpMenu->addAction(sfHomeAction);
    helpMenu->addAction(referenceAction);
    helpMenu->addAction(referenceAction2);
    helpMenu->addAction(galleryAction);
    helpMenu->addAction(glslHomeAction);
    helpMenu->addAction(faqAction);
    helpMenu->addAction(introAction);
}

QString MainWindow::makeImgFileName(int timeStep, int timeSteps, QString fileName) {
    QString name = fileName;

    if (timeSteps > 1) {
        int lastPoint = fileName.lastIndexOf(".");
        name = QString("%1.%2.%3").arg(fileName.left(lastPoint))
               .arg((int)timeStep,5,10,QChar('0'))
               .arg(fileName.right(fileName.size()-lastPoint-1));
    }
    return name;
}

void MainWindow::tileBasedRender() {

    OutputDialog od(this);
retry:
    od.setMaxTime(timeMax);

    bool runFromScript = runningScript;
    QString subdirName;

    if(!runFromScript) {
        if(od.exec() != QDialog::Accepted) return;
    }
    else {
        od.readOutputSettings();
    }

    int w = od.getTileWidth();
    bufferXSpinBox->setValue(w);
    int h = od.getTileHeight();
    bufferYSpinBox->setValue(h);
    int maxTiles = od.getTiles();

    if (od.doSaveFragment()) {
        QString fileName = od.getFragmentFileName();
        logger->getListWidget()->clear();
        if (tabBar->currentIndex() == -1) {
            WARNING(tr("No open tab"));
            return;
        }
        QString inputText = getTextEdit()->toPlainText();
        readSettings();
        Preprocessor p(&fileManager);

        try {
            QString file = tabInfo[tabBar->currentIndex()].filename;
            FragmentSource fs = p.createAutosaveFragment(inputText,file);
            // if the first line is the #version preprocessor command it must stay as the first line
            QString firstLine = fs.source[0].trimmed() + "\n";
            if (firstLine.startsWith("#version")) {
                fs.source.removeAt(0);
                fs.lines.removeAt(0);
            } else firstLine = "";

            QString prepend =  firstLine + tr("// Output generated from file: ") + file + "\n";
            prepend += tr("// Created: ") + QDateTime::currentDateTime().toString() + "\n";
            QString append = "\n\n#preset Default\n" + variableEditor->getSettings() + "\n";

            append += "#endpreset\n\n";

            QString final = prepend + fs.getText() + append;

//             QString f = od.getFileName();
//             QDir oDir(QFileInfo(f).absolutePath());
//
//             subdirName = f.left(f.indexOf(".")) + tr("_Files");
//
//             if (!oDir.mkdir(subdirName)) {
//
//                 QMessageBox::warning(this, tr("Fragmentarium"),
//                                      tr("Could not create directory %1:\n.")
//                                      .arg(oDir.filePath(subdirName)));
//                 return;
//             }
//             subdirName = oDir.filePath(subdirName); // full name

            QString f = od.getFileName();
            QDir oDir(QFileInfo(f).absolutePath());
            QString subdirName = od.getFolderName();
            if (!oDir.mkdir(subdirName)) {

              QMessageBox::warning(this, tr("Fragmentarium"),
                                   tr("Could not create directory %1:\n.")
                                   .arg(oDir.filePath(subdirName)));
              return;
            }
            subdirName = oDir.filePath(subdirName); // full name
// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //

            QFile fileStream(subdirName + QDir::separator() + fileName);
            if (!fileStream.open(QFile::WriteOnly | QFile::Text)) {
                QMessageBox::warning(this, tr("Fragmentarium"),
                                     tr("Cannot write file %1:\n%2.")
                                     .arg(fileName)
                                     .arg(fileStream.errorString()));
                return;
            }

            QTextStream out(&fileStream);
            out << final;
            INFO(tr("Saved fragment + settings as: ") + subdirName + QDir::separator() + fileName);

            // Copy files.
            QStringList ll = p.getDependencies();
            foreach (QString l, ll) {
                QString from = l;
                QString to =  QDir(subdirName).absoluteFilePath( QFileInfo(l).fileName() );
                if (!QFile::copy(from,to)) {
                    QMessageBox::warning(this, tr("Fragmentarium"),
                                         tr("Could not copy dependency:\n'%1' to \n'%2'.")
                                         .arg(from)
                                         .arg(to));
                    return;
                }

            }
        } catch (Exception& e) {
            WARNING(e.getMessage());
        }
    }

    engine->makeCurrent();
    DisplayWidget::DrawingState oldState = engine->getState();
    engine->setState(DisplayWidget::Tiled);
    engine->clearTileBuffer();

    double padding = od.getPadding();
    int maxSubframes = od.getSubFrames();
    QString fileName = od.getFileName();
    int fps = od.getFPS();
    int maxTime = od.getMaxTime();
    int timeSteps = fps*maxTime;
    bool preview = od.preview();
    int startTime = od.startAtFrame();
    int endTime = od.endAtFrame();

    bool imageSaved = false;
#ifdef USE_OPEN_EXR
    exrMode = fileName.endsWith(".exr", Qt::CaseInsensitive);
    engine->setEXRmode(exrMode);
#endif

    if( (w*maxTiles>32768 || h*maxTiles > 32768) && !exrMode ) {
        QMessageBox msgBox;
        msgBox.setText( QString("%1x%2 %3").arg(w*maxTiles).arg(h*maxTiles).arg(tr("is too large!\nMust be less than 32769x32769")));
        msgBox.setInformativeText(tr("Do you want to try again?"));
        msgBox.setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Retry);
        int ret = msgBox.exec();
        switch (ret) {
        case QMessageBox::Retry:
            goto retry;
            break;
        case QMessageBox::Cancel:
            return;
            break;
        default:
            // should never be reached
            break;
        }
    }

    if (timeSteps==0) {
        startTime = getFrame();
        timeSteps = startTime+1;
    }
    else if (endTime > startTime) {
        timeSteps = endTime;
    }

    int totalSteps= timeSteps*maxTiles*maxTiles*maxSubframes;
    int steps = startTime*maxTiles*maxTiles*maxSubframes;
    
    engine->tileAVG = 0;
    engine->renderAVG = 0;
    engine->renderETA =  "";
    engine->framesToRender = endTime - startTime;

    QProgressDialog progress(tr("Rendering"), tr("Abort"), 0, totalSteps, this);
    progress.setValue ( 0 );
    progress.move((width()-progress.width())/2,(height()-progress.height())/2);
    progress.setWindowModality(Qt::WindowModal);
    progress.setValue(steps);
    progress.show();
    
    for (int timeStep = startTime; timeStep<timeSteps ; timeStep++) {
        double time = (double)timeStep/(double)fps;

        if (progress.wasCanceled() || (runFromScript != runningScript)) {
            break;
        }
        if((totalSteps/maxTiles/maxTiles/maxSubframes) > 1) {
            if(variableEditor->hasEasing()) {
                engine->updateEasingCurves( timeStep ); // current frame
            }

            if ( variableEditor->hasKeyFrames() ) {
                if (engine->eyeSpline != NULL) {
                    int index = timeStep+1;
                    QVector3D e = engine->eyeSpline->getSplinePoint(index);
                    QVector3D t = engine->targetSpline->getSplinePoint(index);
                    QVector3D u = engine->upSpline->getSplinePoint(index);
                    if( !e.isNull() && !t.isNull() && !u.isNull() ) {
                        setCameraSettings( e,t,u );
                    }
                }
            }
        }

        QVector<QImage> cachedTileImages;

        QString name=fileName; // prevent double numbering in file name when under script control
        if(!fileName.contains(QRegExp(".[0-9]{5,5}.")))
            name=makeImgFileName(timeStep, timeSteps, fileName);

        if (od.doSaveFragment() || od.doAnimation()) {
          subdirName = od.getFolderName();
          // save the image(s) in the frags folder unless the image name indicates it's own folder
          if(!name.contains(QDir::separator())) {
            name = (QFileInfo(name).absolutePath() + QDir::separator() + subdirName + QDir::separator() + QFileInfo(name).fileName());
            QDir dir(QFileInfo(name).absolutePath());
            if(!dir.exists()) dir.mkdir(QFileInfo(name).absolutePath());
          }
        }
        
        QTime frametime;
        frametime.start();

#ifdef USE_OPEN_EXR
        if(exrMode && !preview) {
            //
            // Write a tiled image with one level using a tile-sized framebuffer.
            //
            TiledRgbaOutputFile out (name.toLatin1(),
                                     w*maxTiles, h*maxTiles,            // image size
                                     w, h,                              // tile size
                                     ONE_LEVEL);                        // level mode
            //                           ROUND_DOWN,                        // rounding mode
            //                           WRITE_RGBA,                        // channels in file
            //                           1,                                 // float pixelAspectRatio
            //                           IMATH_NAMESPACE::V2f (0, 0),       // const IMATH_NAMESPACE::V2f screenWindowCenter
            //                           1,                                 // float screenWindowWidth
            //                           INCREASING_Y,                      // LineOrder
            //                           ZIP_COMPRESSION,                   // Compression
            //                           globalThreadCount() );             // int numThreads

            Array2D<Rgba> pixels (h, w);

            for (int tile = 0; tile<maxTiles*maxTiles; tile++) {

              QTime tiletime;
              tiletime.start();
              
              if (!progress.wasCanceled()) {

                    QImage im(w,h,QImage::Format_ARGB32); im.fill(Qt::black);
                    engine->renderTile(padding,time, maxSubframes, w,h, tile, maxTiles, &progress, &steps, &im);

                    if (padding>0.0)  {
                        int w = im.width();
                        int h = im.height();
                        int nw = (int)(w / (1.0 + padding));
                        int nh = (int)(h / (1.0 + padding));
                        int ox = (w-nw)/2;
                        int oy = (h-nh)/2;
                        im = im.copy(ox,oy,nw,nh);
                    }

                    if(w*maxTiles<32769 && h*maxTiles < 32769)
                        cachedTileImages.append(im);

                    int dx = (tile / maxTiles);
                    int dy = (maxTiles-1)-(tile % maxTiles);
                    int xoff = dx*w;
                    int yoff = dy*h;

                    engine->getRGBAFtile( pixels, w, h );
                    Box2i range = out.dataWindowForTile (dx, dy);
                    out.setFrameBuffer (&pixels[-range.min.y][-range.min.x],
                                        1,  // xStride
                                        w); // yStride

                    out.writeTile (dx, dy);

                    // display tiles while rendering if the tiles fit the window
                    if(engine->width() >= im.width()*maxTiles && engine->height() >= im.height()*maxTiles) {
                        QPainter painter(engine);
                        QRect target(xoff, yoff, w, h);
                        QRect source(0, 0, w, h);
                        painter.drawImage(target, im, source);
                    }
                } else {
                  stopScript();
                  tile = maxTiles*maxTiles;
                }
                engine->tileAVG += tiletime.elapsed();
                // calculate the ETA for one frame
                if(!od.doAnimation() || timeStep == startTime) {
                  int estRenderMS = ((maxTiles*maxTiles)-tile)*(engine->tileAVG/(tile+1));
                  QTime t(0,0,0,0);
                  t=t.addMSecs(estRenderMS);
                  engine->renderETA=t.toString("hh:mm:ss");
                  
                  if( estRenderMS > 86400000 ) // takes longer than 24 hours
                    engine->renderETA = QString(" %1:%2").arg((int)( ((double)estRenderMS / 86400000) )).arg(engine->renderETA);
                }
                
            }

            imageSaved = out.isValidLevel(0,0);

        } else
#endif
        {
                    
          for (int tile = 0; tile<maxTiles*maxTiles; tile++) {

                QTime tiletime;
                tiletime.start();
                
                if (!progress.wasCanceled()) {

                    QImage im(w,h,QImage::Format_ARGB32); im.fill(Qt::black);
                    engine->renderTile(padding,time, maxSubframes, w,h, tile, maxTiles, &progress, &steps, &im);
                    
                    if (padding>0.0)  {
                        int w = im.width();
                        int h = im.height();
                        int nw = (int)(w / (1.0 + padding));
                        int nh = (int)(h / (1.0 + padding));
                        int ox = (w-nw)/2;
                        int oy = (h-nh)/2;
                        im = im.copy(ox,oy,nw,nh);
                    }

                    if(w*maxTiles<32769 && h*maxTiles < 32769)
                        cachedTileImages.append(im);

                    // display tiles while rendering if the tiles fit the window
                    if(engine->width() >= im.width()*maxTiles && engine->height() >= im.height()*maxTiles) {
                        QPainter painter(engine);
                        int dx = (tile / maxTiles);
                        int dy = (maxTiles-1)-(tile % maxTiles);
                        int xoff = dx*w;
                        int yoff = dy*h;
                        QRect target(xoff, yoff, w, h);
                        QRect source(0, 0, w, h);
                        painter.drawImage(target, im, source);
                    }
                    else // display single tiles if tile is same size or smaller
                        if ( engine->width() >= im.width() && engine->height() >= im.height()) {
                            int w = im.width();
                            int h = im.height();
                            QPainter painter ( engine );
                            QRect source ( 0, 0, w, h );
                            QRect target ( 0, 0, w, h );
                            painter.drawImage ( target, im, source );
                        }
                } else {
                    stopScript();
                    tile = maxTiles*maxTiles;
                }
                engine->tileAVG += tiletime.elapsed();
                // calculate the ETA for one frame
                if(!od.doAnimation() || timeStep == startTime) {
                    int estRenderMS = ((maxTiles*maxTiles)-tile)*(engine->tileAVG/(tile+1));
                    QTime t(0,0,0,0);
                    t=t.addMSecs(estRenderMS);
                    engine->renderETA=t.toString("hh:mm:ss");

                    if( estRenderMS > 86400000 ) // takes longer than 24 hours
                      engine->renderETA = QString(" %1:%2").arg((int)( ((double)estRenderMS / 86400000) )).arg(engine->renderETA);
                }
            }
        }
        
        engine->tileAVG /= maxTiles*maxTiles;
        engine->renderAVG += frametime.elapsed();
        
        if(od.doAnimation() && timeStep != startTime) {
          int estRenderMS = (engine->renderAVG/timeStep) * (timeSteps-timeStep);
          QTime t(0,0,0,0);
          t=t.addMSecs(estRenderMS);
          engine->renderETA=t.toString("hh:mm:ss");
          
          if( estRenderMS > 86400000 ) // takes longer than 24 hours
            engine->renderETA = QString(" %1:%2").arg((int)( ((double)estRenderMS / 86400000) )).arg(engine->renderETA);
        }

        // Now assemble image
        if (!progress.wasCanceled() &&
                (!exrMode || (preview && w*maxTiles<32769 && h*maxTiles < 32769)) ) {
            int w = cachedTileImages[0].width();
            int h = cachedTileImages[0].height();
            QImage finalImage(w*maxTiles,h*maxTiles,cachedTileImages[0].format());
            // There IS a Qt function to copy entire images!
            QPainter painter(&finalImage);
            for (int i = 0; i < maxTiles*maxTiles; i++) {
                int dx = (i / maxTiles);
                int dy = (maxTiles-1)-(i % maxTiles);
                int xoff = dx*w;
                int yoff = dy*h;
                QRect target(xoff, yoff, w, h);
                QRect source(0, 0, w, h);
                painter.drawImage(target, cachedTileImages[i], source);
            }

            INFO(QString("Created combined image (%1,%2)").arg(w*maxTiles).arg(h*maxTiles));

            if (preview) {
                static QDialog* qd;
                /// prevent multiple previews
                if(findChild<QDialog*>("PREVIEW")) {
                    qd = findChild<QDialog*>("PREVIEW");
                    qd->close();
                    qd->~QDialog();
                }
                qd = new QDialog(this);
                qd->setObjectName("PREVIEW");

                QVBoxLayout *l = new QVBoxLayout;

                QLabel* label = new QLabel();
                label->setObjectName("previewImage");
                label->setPixmap(QPixmap::fromImage(finalImage));

                QScrollArea* scrollArea = new QScrollArea;
                scrollArea->setBackgroundRole(QPalette::Dark);
                scrollArea->setWidget(label);
                l->addWidget(scrollArea);

                QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
                connect(buttonBox, SIGNAL(rejected()), qd, SLOT(reject()));
                connect(buttonBox, SIGNAL(accepted()), this, SLOT(savePreview()));
                l->addWidget(buttonBox);

                qd->setLayout(l);
                qd->show();

            } else if( !exrMode )
            {
              finalImage.setText("frAg", variableEditor->getSettings());
              imageSaved=finalImage.save(name);
            }

        }
        if(!preview) {
            if(imageSaved && !progress.wasCanceled()) {
                INFO(tr("Saved file : ") + name);
            }
            else WARNING(tr("Save file failed! : ") + name);
        }
    }

    engine->tilesCount = 0;
    engine->setState(oldState);
    progress.setValue(totalSteps);
    if (preview || progress.wasCanceled()) engine->requireRedraw(true);
    engine->clearTileBuffer();
    engine->updateBuffers();
    
}

void MainWindow::savePreview() {

    QDialog *qd;
    if(findChild<QDialog*>("PREVIEW")) {
        qd = findChild<QDialog*>("PREVIEW");

        QStringList extensions;
        QList<QByteArray> a = QImageWriter::supportedImageFormats();
        foreach ( QByteArray s, a ) {
            extensions.append ( QString( "%1.%2" ).arg("*").arg( QString(s) ) );
        }
        QString ext = QString(tr("Images (")) + extensions.join ( " " ) + tr(")");

        QString fn;
        fn = QFileDialog::getSaveFileName(qd,tr("Save preview image..."),"preview.png",ext);
        if(!fn.isEmpty()) {
            QLabel* label = qd->findChild<QLabel*>("previewImage");
            if(label) {
                QImage img = label->pixmap()->toImage();
                img.setText("frAg", variableEditor->getSettings());
                img.save(fn);
                qd->close();
                INFO(tr("Saved file : ") + fn);
            }
        }
    }
}

void MainWindow::pasteSelected() {
    QString settings = getTextEdit()->textCursor().selectedText();
    // Note: If the selection obtained from an editor spans a line break,
    // the text will contain a Unicode U+2029 paragraph separator character instead of a newline \n character. Use QString::replace() to replace these characters with newlines
    settings = settings.replace(QChar::ParagraphSeparator,"\n");
    variableEditor->setSettings(settings);
    INFO(tr("Pasted selected settings"));
}

void MainWindow::saveParameters() {
    QString filter = tr("Fragment Parameters (*.fragparams);;All Files (*.*)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), "", filter);

    saveParameters(fileName);

}

void MainWindow::saveParameters(QString fileName) {

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Fragmentarium"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << variableEditor->getSettings();
    INFO(tr("Settings saved to file"));
}

void MainWindow::loadParameters(QString fileName) {
    QFile file(fileName);
    if (fileName.toLower().endsWith(".png") && file.exists()) {
      variableEditor->setSettings(QImage(fileName).text("frAg"));
      return;
    }
      else
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Fragmentarium"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QString settings = in.readAll();
    variableEditor->setSettings(settings);
    INFO(tr("Settings loaded from file") );
}

void MainWindow::loadParameters() {
    QString filter = tr("Fragment Parameters (*.fragparams);;All Files (*.*)");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load"), "", filter);
    if (fileName.isEmpty())
        return;

    loadParameters(fileName);
}

// M Benesi "Spray gun" saveFeedback
void MainWindow::saveFeedback() {
    // set filename extention filter
  QString filter = tr("Fragment Parameters (*.fdbk);;All Files (*.*)");
    // get a name from user
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), "", filter);

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Fragmentarium"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << "// feedback data for: " + windowTitle().split(" ").at(0) + "\n";
    out << "#feedbackvars\n";
    out << "feedbackindex=" + QString("%1\n").arg(feedbackindex);
    out << "feedbackcount=" + QString("%1\n").arg(feedbackcount);
    out << "feedbackmaxindex=" + QString("%1\n").arg(feedbackmaxindex);
    out << "#endfeedbackvars\n\n";
    out << "//index , coords , ctrl1 , ctrl2 , rot \n";
    out << "#feedbackdata\n";
    for( int i=0; i<feedbackcount+1; i++ ) {
        out << i << ",";
        out << feedbackcrds[i].x() << " " << feedbackcrds[i].y() << " " << feedbackcrds[i].z() << " " << ",";
        out << feedcontrol1[i].x() << " " << feedcontrol1[i].y() << " " << feedcontrol1[i].z() << " " << feedcontrol1[i].w() << ",";
        out << feedcontrol2[i].x() << " " << feedcontrol2[i].y() << " " << feedcontrol2[i].z() << " " << feedcontrol2[i].w() << ",";
        out << feedrotation[i].x() << " " << feedrotation[i].y() << " " << feedrotation[i].z() << " " << feedrotation[i].w() << "\n";
    }
    out << "#endfeedbackdata\n";

    file.close();

    statusBar()->showMessage(tr("Feedback saved to file"), 2000);
}

// M Benesi "Spray gun" loadFeedback
void MainWindow::loadFeedback() {
    // set filename extention filter
    QString filter = tr("Fragment Parameters (*.fdbk);;All Files (*.*)");
    // get a name from user
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load From"), "", filter);

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Fragmentarium"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QString line = in.readLine();

    while(line != "#feedbackvars") line = in.readLine();

    if(line == "#feedbackvars") {
        statusBar()->showMessage(tr("Reading Feedback vars..."), 2000);
        while(line != "#endfeedbackvars") {
            line = in.readLine();
            if(line == "#endfeedbackvars") break;
            if(line.startsWith("feedbackindex="))feedbackindex=line.split("=").at(1).toInt();
            if(line.startsWith("feedbackcount="))feedbackcount=line.split("=").at(1).toInt();
            if(line.startsWith("feedbackmaxindex="))feedbackmaxindex=line.split("=").at(1).toInt();
        }
    }

    while(line != "#feedbackdata") line = in.readLine();
    int dataCount = 0;
    if(line == "#feedbackdata") {
        statusBar()->showMessage(tr("Reading Feedback data..."), 2000);
        while(line != "#endfeedbackdata") {
            line = in.readLine();          //index , coords , ctrl1 , ctrl2 , rot
            if(line == "#endfeedbackdata") break;
            QStringList dataLine = line.split(",");
            QStringList data = dataLine.at(1).split(" ");
            QVector3D coord(data.at(0).toFloat(),data.at(1).toFloat(),data.at(2).toFloat());
            feedbackcrds[dataCount] = coord;
            data = dataLine.at(2).split(" ");
            QVector4D feed(data.at(0).toFloat(),data.at(1).toFloat(),data.at(2).toFloat(),data.at(3).toFloat());
            feedcontrol1[dataCount] = feed;
            data = dataLine.at(3).split(" ");
            feed = QVector4D(data.at(0).toFloat(),data.at(1).toFloat(),data.at(2).toFloat(),data.at(3).toFloat());
            feedcontrol2[dataCount] = feed;
            data = dataLine.at(4).split(" ");
            feed = QVector4D(data.at(0).toFloat(),data.at(1).toFloat(),data.at(2).toFloat(),data.at(3).toFloat());
            feedrotation[dataCount] = feed;
            dataCount++;
        }
    }
    if(dataCount != feedbackcount+1) qDebug() << "Warning! feedbackcount=" << feedbackcount << " feedbackdatacount=" << dataCount;
    else
        statusBar()->showMessage(tr("Feedback loaded from file"), 2000);
    engine->requireRedraw ( true );

}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File Toolbar"));
    fileToolBar->addAction(newAction);
    fileToolBar->addAction(openAction);
    fileToolBar->addAction(saveAction);
    fileToolBar->addAction(saveAsAction);
    fileToolBar->setObjectName("FileToolbar");
//     fileToolBar->hide();
    QSettings settings;
    settings.value("showFileToolbar").toBool() ? fileToolBar->show() : fileToolBar->hide();
    
    editToolBar = addToolBar(tr("Edit Toolbar"));
    editToolBar->addAction(cutAction);
    editToolBar->addAction(copyAction);
    editToolBar->addAction(pasteAction);
    editToolBar->setObjectName("EditToolbar");
//     editToolBar->hide();
    settings.value("showEditToolbar").toBool() ? editToolBar->show() : editToolBar->hide();
    
    bufferToolBar = addToolBar(tr("Buffer Dimensions"));
    bufferToolBar->addWidget(new QLabel(tr("Buffer Size. X: "), this));
    bufferToolBar->setToolTip(tr("Set combobox to 'custom-size' to apply size."));
    bufferToolBar->setToolTipDuration(5000);
    bufferXSpinBox = new QSpinBox(bufferToolBar);
    bufferXSpinBox->setRange(0,8000);
    bufferXSpinBox->setValue(10);
    bufferXSpinBox->setSingleStep(1);
    bufferToolBar->addWidget(bufferXSpinBox);
    bufferToolBar->addWidget(new QLabel(tr("Y: "), this));
    bufferYSpinBox = new QSpinBox(bufferToolBar);
    bufferYSpinBox->setRange(0,8000);
    bufferYSpinBox->setValue(10);
    bufferYSpinBox->setSingleStep(1);
    connect(bufferXSpinBox, SIGNAL(valueChanged(int)), this, SLOT(bufferSpinBoxChanged(int)));
    connect(bufferYSpinBox, SIGNAL(valueChanged(int)), this, SLOT(bufferSpinBoxChanged(int)));
    bufferSizeControl = new QPushButton(tr("Lock to window size"), bufferToolBar);
    QMenu* menu = new QMenu();
    bufferAction1 = menu->addAction(tr("Lock to window size"));
    bufferAction1_2 = menu->addAction(tr("Lock to 1/2 window size"));
    bufferAction1_4 = menu->addAction(tr("Lock to 1/4 window size"));
    bufferAction1_6 = menu->addAction(tr("Lock to 1/6 window size"));
    menu->addSeparator();
    bufferActionCustom = menu->addAction(tr("Custom size"));
    bufferSizeControl->setMenu(menu);

    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(bufferActionChanged(QAction*)));

    bufferToolBar->addWidget(bufferYSpinBox);
    bufferToolBar->addWidget(bufferSizeControl);
    bufferToolBar->setObjectName("BufferDimensions");


    renderToolBar = addToolBar(tr("Render Toolbar"));
    renderToolBar->addAction(renderAction);
    buildLabel = new QLabel(tr("Build"), this);
    renderToolBar->addWidget(buildLabel);
    renderToolBar->setObjectName("RenderToolbar");

    renderModeToolBar = addToolBar(tr("Rendering Mode"));

    //     renderModeToolBar->addWidget(new QLabel("Render mode:", this));

    progressiveButton = new QPushButton( tr("Progressive"),renderModeToolBar);
    progressiveButton->setCheckable(true);
    progressiveButton->setChecked(true);
    animationButton = new QPushButton( tr("Animation"),renderModeToolBar);
    animationButton->setCheckable(true);

    QButtonGroup* bg =new QButtonGroup(renderModeToolBar);
    bg->addButton(progressiveButton);
    bg->addButton(animationButton);

    connect(progressiveButton, SIGNAL(clicked()), this, SLOT(renderModeChanged()));
    connect(animationButton, SIGNAL(clicked()), this, SLOT(renderModeChanged()));

    renderModeToolBar->addWidget(progressiveButton);
    renderModeToolBar->addWidget(animationButton);


    rewindAction = new QAction(QIcon(":/Icons/player_rew.png"), tr("Rewind"), this);
    rewindAction->setShortcut(tr("F10"));
    rewindAction->setStatusTip(tr("Rewinds animation."));
    connect(rewindAction, SIGNAL(triggered()), this, SLOT(rewind()));

    playAction = new QAction(QIcon(":/Icons/player_play.png"), tr("Start"), this);
    playAction->setShortcut(tr("F11"));
    playAction->setStatusTip(tr("Starts animation or subframe rendering."));
    connect(playAction, SIGNAL(triggered()), this, SLOT(play()));

    stopAction = new QAction(QIcon(":/Icons/player_stop.png"), tr("Stop"), this);
    stopAction->setShortcut(tr("F12"));
    stopAction->setStatusTip(tr("Stops animation or subframe rendering."));
    connect(stopAction, SIGNAL(triggered()), this, SLOT(stop()));
    stopAction->setEnabled(false);

    renderModeToolBar->addAction(rewindAction);
    renderModeToolBar->addAction(playAction);
    renderModeToolBar->addAction(stopAction);

    subframeLabel = new QLabel(tr(" Subframe Max: "), renderModeToolBar);
    renderModeToolBar->addWidget(subframeLabel);
    frameSpinBox = new QSpinBox(renderModeToolBar);
    frameSpinBox->setRange(0,10000);
    frameSpinBox->setValue(10);
//     frameSpinBox->setValue(QSettings().value("subframes").toInt());
    frameSpinBox->setSingleStep(5);

    connect(frameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(maxSubSamplesChanged(int)));

    renderModeToolBar->addWidget(frameSpinBox);

    frameLabel = new QLabel(tr(" 0 rendered."), renderModeToolBar);
    renderModeToolBar->addWidget(frameLabel);
    renderModeToolBar->setObjectName("RenderingMode");

    // M Benesi "spray gun
    zappaToolBar = addToolBar(tr("Zappa Tools"));
    zappaToolBar->setObjectName("ZappaToolBar");
    zappaToolBar->layout()->setSpacing(10);

    zapCheck = new QCheckBox(this);
    zapCheck->setToolTip(tr("Enable Zappa Control"));
    connect(zapCheck, SIGNAL(toggled(bool)), this, SLOT(setZapLock(bool)));
    zapLock = zappaToolBar->addWidget(zapCheck);

    zapIndex = new QSpinBox(this);
    zapIndex->setToolTip(tr("Current Zappa Index"));
    zapIndex->setMaximum(0);
    zapIndex->setMinimum(0);
    connect(zapIndex, SIGNAL(valueChanged(int)), this, SLOT(setFeedbackIndex(int)));
    zapIndx = zappaToolBar->addWidget(zapIndex);

    zapClear = new QPushButton(this);
    zapClear->setToolTip(tr("Clear All Zappas"));
    zapClear->setText(tr("Clear"));
    connect(zapClear, SIGNAL(clicked()), this, SLOT(setZapClear()));
    zapClr = zappaToolBar->addWidget(zapClear);
    zappaToolBar->hide();

    addToolBarBreak();
    timeToolBar = addToolBar(tr("Time"));

    timeLabel = new QLabel(tr("Time: 0s "), renderModeToolBar);
    timeLabel->setFixedWidth(100);
    timeToolBar->addWidget(timeLabel);

    timeSlider = new QSlider(Qt::Horizontal, this);
    timeSlider->setMinimum(0);
    timeSlider->setValue(0);

    timeSlider->setMaximum(10*renderFPS); // seconds * frames per second = length of anim
    connect(timeSlider, SIGNAL(valueChanged(int)), this, SLOT(timeChanged(int)));
    timeToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(timeToolBar, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(timeLineRequest(QPoint)));

    timeToolBar->addWidget(timeSlider);

    timeMaxSpinBox = new QSpinBox(renderModeToolBar);
    timeMaxSpinBox->setRange(0,10000);
    timeMaxSpinBox->setValue(timeMax);
    connect(timeMaxSpinBox, SIGNAL(valueChanged(int)), this, SLOT(timeMaxChanged(int)));
    timeToolBar->addWidget(timeMaxSpinBox);
    timeToolBar->setObjectName("Time");

    timeMaxChanged(timeMaxSpinBox->value()); // call when timeMax changes to setup the time slider for frame accuracy
    autoFocusEnabled = false;
}

void MainWindow::setTimeSliderValue(int v) {
    // timeslider max = animation length in frames set here to keep in sync with framerate/duration changes
    timeSlider->setMaximum(timeMaxSpinBox->value()*renderFPS);

    if(v >= timeMaxSpinBox->value()*renderFPS) {
        if(wantLoopPlay && engine->getState()==DisplayWidget::Animation && !pausePlay) {
            rewind();
            play();
            return;
        }
        playAction->setEnabled(true);
        stopAction->setEnabled(false);
        lastStoredTime = v;
        engine->setContinuous(false);
        v = timeMaxSpinBox->value()*renderFPS;
    }
    timeSlider->setValue(v);
}

void MainWindow::bufferActionChanged(QAction* action) {
    bufferSizeControl->setText(action->text());
    if (action == bufferAction1) {
        bufferSizeMultiplier = 1;
    } else if (action == bufferAction1_2) {
        bufferSizeMultiplier = 2;
    } else if (action == bufferAction1_4) {
        bufferSizeMultiplier = 4;
    } else if (action == bufferAction1_6) {
        bufferSizeMultiplier = 6;
    } else if (action == bufferActionCustom) {
        bufferSizeMultiplier = 0;
    }
    engine->updateBuffers();
}

void MainWindow::timeLineRequest(QPoint ) {

  TimeLineDialog *TimeDialog = new TimeLineDialog(this);
  TimeDialog->exec();

};

void MainWindow::videoEncoderRequest() {

  VideoDialog *vDialog = new VideoDialog(this);
  vDialog->exec();

};

void MainWindow::timeChanged(int) {
    lastTime->restart();
    lastStoredTime = getTimeSliderValue();
    getTime();
    engine->requireRedraw(true);
}

void MainWindow::timeMaxChanged(int v) {
    lastTime->restart();
    lastStoredTime = getTimeSliderValue();
    getTime();
    timeSlider->setMaximum(v*renderFPS);        // timeslider max = animation length in frames
    timeSlider->setSingleStep(1);                       // should be one frame
    timeSlider->setPageStep(renderFPS); // should be one second
    timeMax=v;

    if(variableEditor->hasKeyFrames()) initKeyFrameControl();
}

void MainWindow::rewind() {
    lastTime->restart();
    lastStoredTime = 0;
    getTime();
}

void MainWindow::play() {
    playAction->setEnabled(false);
    stopAction->setEnabled(true);
    lastTime->restart();
    engine->setContinuous(true);
    engine->setFocus();
    getTime();
    pausePlay=false;
}

void MainWindow::stop() {

    playAction->setEnabled(true);
    stopAction->setEnabled(false);
    lastStoredTime = getTime();

    if(engine->getState() == DisplayWidget::Animation)
    INFO(QString("%1 %2").arg(tr("Stopping: last stored time set to")).arg((double)lastStoredTime/renderFPS));
    //statusBar()->showMessage(QString("%1 %2").arg(tr("Stopping: last stored time set to")).arg((double)lastStoredTime/renderFPS));

    engine->setContinuous(false);
    getTime();
    pausePlay=true;
}

void MainWindow::maxSubSamplesChanged(int i) {
    engine->setMaxSubFrames(i);
}

void MainWindow::setSubframeMax(int i) {
    frameSpinBox->setValue(i);
}

double MainWindow::getTime() {
    DisplayWidget::DrawingState state = engine->getState();

    int time = 0;
    if (!engine->isContinuous() || state == DisplayWidget::Tiled) {
        // The engine is not in 'running' mode. Return last stored paused time.
        time = lastStoredTime;
    } else {
        if (state == DisplayWidget::Progressive) {
            time = lastStoredTime;
        } else if (state == DisplayWidget::Animation) {
            time = lastStoredTime + ((lastTime->elapsed()/1000.0)*renderFPS);
        }
    }
    int ct = time != 0 ? time/renderFPS : 0;
    timeLabel->setText(QString("%1 %2s:%3f ").arg(tr("Time:") ).arg(ct,3,'g',-1,'0').arg((((double)time/(double)renderFPS)-ct)*renderFPS,2,'g',-1,'0'));
    timeSlider->blockSignals(true);
    setTimeSliderValue(time);
    timeSlider->blockSignals(false);
    return time;
}

void MainWindow::renderModeChanged() {
    engine->setMaxSubFrames(frameSpinBox->value());
    setFPS(-1);
    QObject* o = QObject::sender();
    if (o == 0 || o == progressiveButton) {
        lastStoredTime = getTime();
        engine->setState(DisplayWidget::Progressive);
        getTime();
    } else if (o == animationButton) {
        lastStoredTime = getTime();
        engine->setState(DisplayWidget::Animation);
        lastTime->restart();
        getTime();
    }

    engine->setFocus();
    engine->setDisableRedraw(false);
}

void MainWindow::setSubFrameDisplay(int i) {
    frameLabel->setText(QString(" %1 %2").arg(tr("Done")).arg(i));
}

void MainWindow::callRedraw() {
    bool state = engine->isRedrawDisabled();
    engine->setDisableRedraw(false);
    engine->setDisableRedraw(state);
}

void MainWindow::disableAllExcept(QWidget* w) {
    disabledWidgets.clear();
    disabledWidgets = findChildren<QWidget *>("");
    while (w) {
        disabledWidgets.removeAll(w);
        w=w->parentWidget();
    }

    foreach (QWidget* w, disabledWidgets) w->setEnabled(false);
    processGuiEvents();
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings;
    static bool first = true;
    if(first) {
        restoreGeometry(settings.value("geometry").toByteArray());
        restoreState(settings.value("windowState").toByteArray());
        first = false;
        splitter->restoreState(settings.value("splitterSizes").toByteArray());
    }
    renderFPS = settings.value("fps", 25).toInt();
    timeMax = settings.value("timeMax", 10).toInt();
    wantGLPaths = settings.value("drawGLPaths", true).toBool();
    wantSplineOcclusion = settings.value("splineOcc", true).toBool();
    wantLineNumbers = settings.value("lineNumbers", true).toBool();
    wantLoopPlay = settings.value("loopPlay", true).toBool();
    editorStylesheet = settings.value("editorStylesheet", "font: 9pt Courier;").toString();
    variableEditor->updateGeometry();
    variableEditor->setSaveEasing(settings.value("saveEasing", true).toBool());
    fileManager.setIncludePaths(settings.value("includePaths", "Examples/Include;").toString().split(";", QString::SkipEmptyParts));
#ifdef USE_OPEN_EXR
    exrBinaryPath = settings.value("exrBinPaths", "/usr/bin;bin;").toString().split(";", QString::SkipEmptyParts);
#endif // USE_OPEN_EXR
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("fps", renderFPS);
    settings.setValue("timeMax", timeMax);
    settings.setValue("drawGLPaths", wantGLPaths);
    settings.setValue("splineOcc", wantSplineOcclusion);
    settings.setValue("lineNumbers", wantLineNumbers);
    settings.setValue("loopPlay", wantLoopPlay);
    settings.setValue("editorStylesheet", editorStylesheet);
    settings.setValue("splitterSizes", splitter->saveState());
    QString ipaths = fileManager.getIncludePaths().join(";");
    if(fileManager.getIncludePaths().count() == 1) ipaths += ";";
    settings.setValue("includePaths", ipaths);
#ifdef USE_OPEN_EXR
    QString ebpaths = exrBinaryPath.join(";");
    if(exrBinaryPath.count() == 1) ebpaths += ";";
    settings.setValue("exrBinPaths", ebpaths);
#endif // USE_OPEN_EXR

    settings.setValue("showFileToolbar", !fileToolBar->isHidden() );
    settings.setValue("showEditToolbar", !editToolBar->isHidden() );
}

void MainWindow::openFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        loadFragFile(action->data().toString());
    } else {
        WARNING(tr("No data!"));
    }
}

void MainWindow::loadFragFile(const QString &fileName)
{
  if (fileName.toLower().endsWith(".frag") && QFile(fileName).exists()) {

    insertTabPage(fileName);

    DisplayWidget::DrawingState oldstate = engine->getState();
    engine->setState(DisplayWidget::Progressive);
    bool pp = pausePlay;
    stop();

    QString inputText = getTextEdit()->toPlainText();
    if (inputText.startsWith("#donotrun")) variableEditor->resetUniforms(false);
    if (QSettings().value("autorun", true).toBool() && initializeFragment()) {
        bool requiresRecompile = variableEditor->setDefault();
        if (requiresRecompile || rebuildRequired) {
            INFO(tr("Build to update locking..."));
        }
        initializeFragment();
        variableEditor->setDefault();
    }
    QSettings settings;
    settings.setValue("isStarting", false);
    engine->setState(oldstate);
    pp?stop():play();
  }
}

bool MainWindow::saveFile(const QString &fileName)
{
    if (tabBar->currentIndex() == -1) {
        WARNING(tr("No open tab"));
        return false;
    }

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Fragmentarium"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << getTextEdit()->toPlainText();
    QApplication::restoreOverrideCursor();

    tabInfo[tabBar->currentIndex()].hasBeenSavedOnce = true;
    tabInfo[tabBar->currentIndex()].unsaved = false;
    tabInfo[tabBar->currentIndex()].filename = fileName;
    tabChanged(tabBar->currentIndex()); // to update displayed name;

    statusBar()->showMessage(tr("File saved"), 2000);
    setRecentFile(fileName);

    return true;
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::showDebug() {

    logger->getListWidget()->clear();
    if (tabBar->currentIndex() == -1) {
        WARNING(tr("No open tab"));
        return;
    }
    INFO(tr("Showing preprocessed output in new tab"));
    QString inputText = getTextEdit()->toPlainText();
    QString filename = tabInfo[tabBar->currentIndex()].filename;
    QSettings settings;
    bool moveMain = settings.value("moveMain", true).toBool();
    readSettings();
    Preprocessor p(&fileManager);
    try {
        FragmentSource fs = p.parse(inputText,filename,moveMain);
//         QString prepend =  "#define highp\n"
//                            "#define mediump\n"
//                            "#define lowp\n";
        variableEditor->substituteLockedVariables(&fs);
        insertTabPage("")->setPlainText(/*prepend+*/fs.getText());
    } catch (Exception& e) {
        WARNING(e.getMessage());
    }
}

void MainWindow::highlightBuildButton(bool value) {
    QWidget* w = buildLabel->parentWidget();
    if (value) {
        QPalette pal = buildLabel->palette();
        pal.setColor(buildLabel->backgroundRole(), Qt::yellow);
        buildLabel->setPalette(pal);
        buildLabel->setAutoFillBackground(true);
        w->setPalette(pal);
        w->setAutoFillBackground(true);
    } else {
        buildLabel->setPalette(QApplication::palette(buildLabel));
        buildLabel->setAutoFillBackground(false);
        w->setPalette(QApplication::palette(w));
        w->setAutoFillBackground(false);
    }
    needRebuild(value);
}

bool MainWindow::initializeFragment() {
 
    fragHasFeedbackVars=false;

    DisplayWidget::DrawingState oldState = engine->getState();
    bool pause = pausePlay;
    engine->setState(DisplayWidget::Progressive);
    stop();

    logger->getListWidget()->clear();
    if (tabBar->currentIndex() == -1) {
        WARNING(tr("No open tab"));
        return false;
    }
    QString inputText = getTextEdit()->toPlainText();
    if (inputText.startsWith("#donotrun")) {
        INFO(tr("Not a runnable fragment."));
        return false;
    }
    QString filename = tabInfo[tabBar->currentIndex()].filename;
    QSettings settings;
    bool moveMain = settings.value("moveMain", true).toBool();
    //     readSettings();
    QTime start = QTime::currentTime();
    fileManager.setOriginalFileName(filename);

    if(variableEditor->hasKeyFrames())
        clearKeyFrames();

    Preprocessor p(&fileManager);
    QString camSet = getCameraSettings(); // BUG Fixs Up vector
    bool showGUI = false;
    highlightBuildButton(false);
    variableEditor->locksUseDefines( QSettings().value("useDefines", true).toBool() );
    
    try {
        FragmentSource fs = p.parse(inputText,filename,moveMain);
        variableEditor->updateFromFragmentSource(&fs, &showGUI); // BUG Up vector gets trashed on Build or Save
        variableEditor->substituteLockedVariables(&fs);
        variableEditor->updateTextures(&fs, &fileManager);
        engine->setFragmentShader(fs);
    } catch (Exception& e) {
        WARNING(e.getMessage());
        highlightBuildButton(true);
    }
    if(getCameraSettings() != camSet) variableEditor->setSettings(camSet); // BUG Fixs Up vector
    editorDockWidget->setHidden(!showGUI);
    variableEditor->updateCamera(engine->getCameraControl());
    engine->requireRedraw(true);
    engine->resetTime();

    int ms = start.msecsTo(QTime::currentTime());
    if (engine->hasShader()) {
        if(!engine->hasBufferShader())
            setSubframeMax(1);

        INFO(tr("Compiled script in %1 ms.").arg(ms));
        engine->setState(oldState);
        pause ? stop() : play();

        /// hide unused widgets unless they are lockable
        QStringList wnames = variableEditor->getWidgetNames();
        for (int i = 0; i < wnames.count(); i++) {
            // find a widget in the variable editor
            VariableWidget* vw = variableEditor->findChild<VariableWidget*>(wnames.at(i));
            if(vw != 0) {
                /// get the uniform location from the shader
                int uloc = vw->uniformLocation(engine->getShader());
                if(uloc == -1 && engine->hasBufferShader())
                    /// get the uniform location from the buffershader
                    uloc = vw->uniformLocation(engine->getBufferShader());
                /// locked widgets are transformed into const or #define so don't show up as uniforms
                /// AutoFocus is a dummy so does not exist inside shader program
                /// feedback vars are a special case these are transfered to an Array then used in the frag
                /// and will get optimized out by the GPU compiler, if they exist we should not attempt to hide widgets
                if(uloc == -1 &&
                        !(vw->getLockType() == Parser::Locked ||
                          vw->getDefaultLockType() == Parser::AlwaysLocked ||
                          vw->getDefaultLockType() == Parser::NotLockable) &&
                        !wnames.at(i).contains("AutoFocus")  &&
                        !(vw->getGroup().contains("Feedback") || vw->getName().contains("Feedback")))  {
                    vw->hide();
                } else {
                    vw->show();
                }
            }
        }
        // this adjusts variable widgets, should be handled by layout container?
        variableEditor->tabChanged(0);

        return true;
    } else {
        WARNING(tr("Failed to compile script (%1 ms).").arg(ms));
        return false;
    }

}

namespace {
// Returns the first valid directory.
QString findDirectory(QStringList guesses) {
    QStringList invalid;
    for (int i = 0; i < guesses.size(); i++) {
        if (QFile::exists(guesses[i])) return guesses[i];
        invalid.append(QFileInfo(guesses[i]).absoluteFilePath());
    }

    // not found.
    WARNING(QCoreApplication::tr("Could not locate directory in: ") + invalid.join(",") + ".");
    return QCoreApplication::tr("[not found]");
}
}

// Mac needs to step two directies up, when debugging in XCode...
QString MainWindow::getExamplesDir() {
    QStringList examplesDir;
    examplesDir << "Examples" << "../Examples" << "../../Examples";
    return findDirectory(examplesDir);
}

QString MainWindow::getMiscDir() {
    QStringList miscDir;
    miscDir << "Misc" << "../Misc" << "../../Misc";
    return findDirectory(miscDir);
}

TextEdit* MainWindow::getTextEdit() {
    return (TextEdit*)(stackedTextEdits->currentWidget() ? stackedTextEdits->currentWidget() : 0);
}

void MainWindow::cursorPositionChanged() {
    TextEdit *te = this->getTextEdit();
    if (!te) return;
    int pos = te->textCursor().position();
    int blockNumber = te->textCursor().blockNumber();

    // Do reverse look up...
    FragmentSource* fs = engine->getFragmentSource();
    QString x;
    QStringList ex;
    QString filename = tabInfo[tabBar->currentIndex()].filename;

    for (int i = 0; i < fs->lines.count(); i++) {
        // fs->sourceFiles[fs->sourceFile[i]]->fileName()
        if (fs->lines[i] == blockNumber &&
                QString::compare(filename,fs->sourceFileNames[fs->sourceFile[i]], Qt::CaseInsensitive)==0
           ) ex.append(QString::number(i+4));
    }
    if (ex.count()) {
        x = tr(" Line in preprocessed script: ") + ex.join(",");
    } else {
        x = tr(" (Not part of current script) ");
    }

    statusBar()->showMessage(tr("Position: %1, Line: %2.").arg(pos).arg(blockNumber+1)+x, 5000);
}

TextEdit* MainWindow::insertTabPage(QString filename) {

    TextEdit* textEdit = new TextEdit(this);
    textEdit->setStyleSheet(editorStylesheet);

    connect(textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));

    textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    textEdit->setTabStopWidth(20);
    textEdit->fh = new FragmentHighlighter(textEdit->document());

    QString s = tr("// Write fragment code here...\r\n");
/*    
    s = "#include \"DE-Raytracer.frag\" \r\n\
\r\n\
float DE(vec3 pos) {\r\n\
	return abs(length(abs(pos)+vec3(-1.0))-1.2);\r\n\
}\r\n\
\r\n\
#preset Default\r\n\
FudgeFactor = 0.9\r\n\
BoundingSphere = 5\r\n\
Detail = -0.8\r\n\
Specular = 2.25\r\n\
SpecularExp = 33.332\r\n\
SpotLight = 1,1,1,0.02174\r\n\
SpotLightDir = -0.5619,0.06666\r\n\
Glow = 1,1,1,0.21053\r\n\
OrbitStrength = 0\r\n\
#endpreset\r\n";
*/
    textEdit->setPlainText(s);

    bool loadingSucceded = false;
    if (!filename.isEmpty()) {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            textEdit->setPlainText(tr("Cannot read file %1:\n%2.").arg(filename).arg(file.errorString()));
        } else {
            QTextStream in(&file);
            QApplication::setOverrideCursor(Qt::WaitCursor);
            textEdit->setPlainText(in.readAll());
            QApplication::restoreOverrideCursor();
            INFO(tr("Loaded file: %1").arg(filename));
            loadingSucceded = true;
        }
    }

    QString displayName = filename;
    if (displayName.isEmpty()) {
        // Find a new name
        displayName = tr("Unnamed");
        QString suggestedName = displayName;

        bool unique = false;
        int counter = 1;
        while (!unique) {
            unique = true;
            for (int i = 0; i < tabInfo.size(); i++) {
                if (tabInfo[i].filename == suggestedName) {
                    //INFO("equal");
                    unique = false;
                    break;
                }
            }
            if (!unique) suggestedName = displayName + " " + QString::number(counter++);
        }
        displayName = suggestedName;
    }

    stackedTextEdits->addWidget(textEdit);

    if (loadingSucceded) {
        tabInfo.append(TabInfo(displayName, textEdit, false, true));
        setRecentFile(filename);
        textEdit->saveSettings( variableEditor->getSettings() );

    } else {
        tabInfo.append(TabInfo(displayName, textEdit, true));
    }

    QString tabTitle = QString("%1%3").arg(strippedName(displayName)).arg(!loadingSucceded? "*" : "");
    tabBar->setCurrentIndex(tabBar->addTab(strippedName(tabTitle)));

    connect(textEdit->document(), SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));

    return textEdit;
}

void MainWindow::resetCamera(bool fullReset) {
    engine->resetCamera(fullReset);
}

void MainWindow::tabChanged(int index) {
    if (index > tabInfo.size()) return;
    if (index < 0) return;

    TextEdit *te = getTextEdit();
    te->saveSettings( variableEditor->getSettings() );

    TabInfo ti = tabInfo[index];
    QString tabTitle = QString("%1%3").arg(strippedName(ti.filename)).arg(ti.unsaved ? "*" : "");
    setWindowTitle(QString("%1 - %2").arg(tabTitle).arg("Fragmentarium"));
    stackedTextEdits->setCurrentWidget(ti.textEdit);
    tabBar->setTabText(tabBar->currentIndex(), tabTitle);

    clearKeyFrames();
    needRebuild(true);

    if (!(QSettings().value("autorun", true).toBool())) {
        WARNING(tr("Auto run is disabled! You must select \"Build\" and apply a preset."));
        WARNING(tr("If the preset alters locked variables \"Build\" will be required again."));
        highlightBuildButton(true);
        return;
    }

    initializeFragment();
    // this bit of fudge resets the tab to its last settings
    if(stackedTextEdits->count() > 1 ) {
        te = getTextEdit(); // the currently active one
        variableEditor->setSettings(te->lastSettings());
        initializeFragment();
    }
    // this makes textures persistent ???
    initializeFragment();
}

void MainWindow::closeTab() {
    int index = tabBar->currentIndex();
    closeTab(index);
}

void MainWindow::closeTab(int index) {

    TabInfo t = tabInfo[index];
    if (t.unsaved) {
        QString mess = tr("There are unsaved changes.%1\r\nClose this tab without saving changes?").arg( variableEditor->hasEasing() ? "\r\nTo keep Easing curves you must\r\nadd a preset named \"Range\"\r\nand save before closing!":"\r\n");
        int answer = QMessageBox::warning(this, tr("Unsaved changes"), mess, tr("OK"), tr("Cancel"));
        if (answer == 1) return;
    }

    tabInfo.remove(index);
    tabBar->removeTab(index);

    stackedTextEdits->removeWidget(t.textEdit);
    delete(t.textEdit); // widget is gone but textedit remains so manually delete it

    clearKeyFrames();
    // if no more tabs don't try to reset to last saved settings
    if (tabBar->currentIndex() == -1) return;
    // this bit of fudge resets the tab to its last settings
    initializeFragment();
    TextEdit *te = getTextEdit();
    variableEditor->setSettings(te->lastSettings());
}

void MainWindow::clearKeyFrames() {
    // clear the easingcurve settings
    if(variableEditor->hasEasing()) {
        engine->setCurveSettings( QStringList() );
        variableEditor->setEasingEnabled(false);
    }
    // clear the spline data
    if(variableEditor->hasKeyFrames())
        clearKeyFrameControl();
}

void MainWindow::launchSfHome() {
    INFO(tr("Launching web browser..."));
    bool s = QDesktopServices::openUrl(QUrl("http://syntopia.github.com/Fragmentarium/"));
    if (!s) WARNING(tr("Failed to open browser..."));
}

void MainWindow::launchGLSLSpecs() {
    INFO(tr("Launching web browser..."));
    bool s = QDesktopServices::openUrl(QUrl("http://www.opengl.org/registry/"));
    if (!s) WARNING(tr("Failed to open browser..."));
}

void MainWindow::launchIntro() {
    INFO("Launching web browser...");
    bool s = QDesktopServices::openUrl(QUrl("http://blog.hvidtfeldts.net/index.php/2011/06/distance-estimated-3d-fractals-part-i/"));
    if (!s) WARNING(tr("Failed to open browser..."));
}

void MainWindow::launchFAQ() {
    INFO("Launching web browser...");
    bool s = QDesktopServices::openUrl(QUrl("http://blog.hvidtfeldts.net/index.php/2011/12/fragmentarium-faq/"));
    if (!s) WARNING(tr("Failed to open browser..."));
}

void MainWindow::launchReferenceHome() {
    INFO("Launching web browser...");
    bool s = QDesktopServices::openUrl(QUrl("http://www.fractalforums.com/fragmentarium/"));
    if (!s) WARNING(tr("Failed to open browser..."));
}

void MainWindow::launchReferenceHome2() {
    INFO("Launching web browser...");
    bool s = QDesktopServices::openUrl(QUrl("http://www.digilanti.org/fragmentarium/"));
    if (!s) WARNING(tr("Failed to open browser..."));
}

void MainWindow::launchGallery() {
    INFO("Launching web browser...");
    bool s = QDesktopServices::openUrl(QUrl("http://flickr.com/groups/fragmentarium/"));
    if (!s) WARNING(tr("Failed to open browser..."));
}

void MainWindow::makeScreenshot() {
    engine->updateGL();
    saveImage(engine->grabFrameBuffer());
}

void MainWindow::saveImage(QImage image) {
    QString filename = GetImageFileName(this, tr("Save screenshot as:"));
    if (filename.isEmpty()) return;

    image.setText("frAg", variableEditor->getSettings());

    bool succes = image.save(filename);
    if (succes) {
        INFO(tr("Saved screenshot as: ") + filename);
    } else {
        WARNING(tr("Save failed! Filename: ") + filename);
    }
}

void MainWindow::copy() {
    if (tabBar->currentIndex() == -1) {
        WARNING(tr("No open tab"));
        return;
    }
    getTextEdit()->copy();
}

void MainWindow::cut() {
    if (tabBar->currentIndex() == -1) {
        WARNING(tr("No open tab"));
        return;
    }
    getTextEdit()->cut();
}

void MainWindow::paste() {
    if (tabBar->currentIndex() == -1) {
        WARNING(tr("No open tab"));
        return;
    }
    getTextEdit()->paste();
}

void MainWindow::search() {
    if (tabBar->currentIndex() == -1) {
        WARNING(tr("No open tab"));
        return;
    }

    QString text;
    bool ok = false;
    /// do we have selected text ? use it or ask for search term
    if(getTextEdit()->textCursor().hasSelection()) {
        text = getTextEdit()->textCursor().selectedText();
    } else {
        text = QInputDialog::getText(this, tr("Search"),
                                     tr("Text to find"), QLineEdit::Normal,
                                     text, &ok);
    }
restart:
    if(!getTextEdit()->find(text)) {
        text = QInputDialog::getText(this, tr("Not found"),
                                     tr("Try again from the start?"), QLineEdit::Normal,
                                     text, &ok);
        /// move to beginning and search again
        if(ok && !text.isEmpty()) {
            QTextCursor cursor(getTextEdit()->textCursor());
            cursor.setPosition(0);
            getTextEdit()->setTextCursor( cursor );
            goto restart;
        }
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        ev->acceptProposedAction();
    } else {
        INFO(tr("Cannot accept MIME object: ") + ev->mimeData()->formats().join(" - "));
    }
}

void MainWindow::dropEvent(QDropEvent *ev) {
    if (ev->mimeData()->hasUrls()) {
        QList<QUrl> urls = ev->mimeData()->urls();
        for (int i = 0; i < urls.size() ; i++) {
            QString file = urls[i].toLocalFile();
            INFO(tr("Loading: ") + file);
            if (file.toLower().endsWith(".fragparams")) loadParameters(file);
            else if (file.toLower().endsWith(".frag")) loadFragFile(file);
            else if (file.toLower().endsWith(".png")) {
              variableEditor->setSettings(QImage(file).text("frAg"));
            }
            else INFO(tr("Must be a .frag or .fragparams file."));
        }
    } else INFO(tr("Cannot accept MIME object: ") + ev->mimeData()->formats().join(" - "));
}

void MainWindow::setRecentFile(const QString &fileName)
{
    QSettings settings;

    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles) files.removeLast();

    settings.setValue("recentFileList", files);

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
      QString text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
        recentFileActions[i]->setText(text);
        QString absPath = QFileInfo(files[i]).absoluteFilePath();
        recentFileActions[i]->setData(absPath);
        recentFileActions[i]->setVisible(true);
    }

    for (int j = numRecentFiles; j < MaxRecentFiles; ++j) recentFileActions[j]->setVisible(false);

    recentFileSeparator->setVisible(numRecentFiles > 0);
}

void MainWindow::setSplashWidgetTimeout(QSplashScreen* w) {
    splashWidget = w;
    QTimer::singleShot(2000, this, SLOT(removeSplash()));
}

void MainWindow::removeSplash() {
    if (splashWidget) splashWidget->finish(this);
    splashWidget = 0;
}

void MainWindow::insertText() {
    if (tabBar->currentIndex() == -1) {
        WARNING(tr("No open tab"));
        return;
    }

    QString text = ((QAction*)sender())->iconText(); // iconText is the menu text without hotkey char
    getTextEdit()->insertPlainText(text.section("//",0,0)); // strip comments
}

void MainWindow::preferences() {
    PreferencesDialog pd(this);
    pd.exec();
    readSettings();
    engine->updateRefreshRate();
    getTextEdit()->setStyleSheet(editorStylesheet);
#ifdef USE_OPEN_EXR
    initTools();
#endif // USE_OPEN_EXR
}

void MainWindow::getBufferSize(int w, int h, int& bufferSizeX, int& bufferSizeY, bool& fitWindow) {
    if (engine && engine->getState()==DisplayWidget::Tiled) {
        bufferSizeX = bufferXSpinBox->value();
        bufferSizeY =  bufferYSpinBox->value();
        return;
    }
    if (!bufferXSpinBox || !bufferYSpinBox) return;
    if (bufferSizeMultiplier>0) {
        // Locked to a fraction of the window size
        bufferSizeX = w/bufferSizeMultiplier;
        bufferSizeY = h/bufferSizeMultiplier;
        bufferXSpinBox->blockSignals(true);
        bufferYSpinBox->blockSignals(true);
        bufferXSpinBox->setValue(bufferSizeX);
        bufferYSpinBox->setValue(bufferSizeY);
        bufferXSpinBox->blockSignals(false);
        bufferYSpinBox->blockSignals(false);
        fitWindow = true;
    } else if (bufferSizeMultiplier<=0) {
        bufferSizeX = bufferXSpinBox->value();
        bufferSizeY =  bufferYSpinBox->value();

        double f = (double)bufferSizeX/(double)w;
        //bool downsized = false;
        if (f>1.0) {
            //downsized = true;
            bufferSizeX/=f;
            bufferSizeY/=f;
        }

        f = (double)bufferSizeY/(double)h;
        if (f>1.0) {
            //downsized = true;
            bufferSizeX/=f;
            bufferSizeY/=f;
        }

        fitWindow = false;
    }
}

void MainWindow::indent() {
    if (tabBar->currentIndex() == -1) {
        WARNING(tr("No open tab"));
        return;
    }

    TextEdit *te = getTextEdit();
    int hValue =  te->horizontalScrollBar()->value();
    int vValue =  te->verticalScrollBar()->value();
    int cPos = te->textCursor().position();
    QStringList l = te->toPlainText().split("\n");
    QStringList out;
    int indent = 0;
    foreach (QString s, l) {
        int offset = s.trimmed().startsWith("}") ? -1 : 0;
        QString newString = s.trimmed();
        for (int i = 0; i < indent+offset; i++) newString.push_front("\t");
        out.append(newString);
        indent = indent + s.count("{")+s.count("(") - s.count("}") -s.count(")");
    }
    te->setPlainText(out.join("\n"));

    te->horizontalScrollBar()->setValue(hValue);
    te->verticalScrollBar()->setValue(vValue);
    QTextCursor tc = te->textCursor();
    tc.setPosition(cPos);
    te->setTextCursor(tc);

}

void MainWindow::setFPS(float fps) {
    if (fps>0) {
        fpsLabel->setText("FPS: " + QString::number(fps, 'f' ,1) + " (" +  QString::number(1.0/fps, 'g' ,1) + "s)");
    } else {
        fpsLabel->setText("FPS: n.a.");
    }
}

QString MainWindow::getCameraSettings() {
    QString settings = variableEditor->getSettings();
    QStringList l = settings.split("\n");
    QStringList r;
    // added " =" to Eye because Axolotl has Eyes!
    QString camId = engine->getCameraControl()->getID();
    if(camId == "3D")
      r << l.filter("FOV") << l.filter("Eye =") << l.filter("Target") << l.filter("Up");
    else if(camId == "2D")
      r << l.filter("Center") << l.filter("Zoom");
    return r.join("\n");
}

void MainWindow::setCameraSettings(QVector3D e, QVector3D t, QVector3D u) {
    QString r = QString("Eye = %1,%2,%3\nTarget = %4,%5,%6\nUp = %7,%8,%9\n")
                .arg(e.x()).arg(e.y()).arg(e.z()).arg(t.x()).arg(t.y()).arg(t.z()).arg(u.x()).arg(u.y()).arg(u.z());
    variableEditor->blockSignals(true);
    if(engine->getFragmentSource()->autoFocus) { // widget detected
        BoolWidget *btest = dynamic_cast<BoolWidget*>(variableEditor->getWidgetFromName("AutoFocus"));
        if(btest != NULL)
            if(btest->isChecked()) {
                double d = e.distanceToPoint(t);
                r += QString("FocalPlane = %1\n").arg(d);
            }
    }
    variableEditor->setSettings(r);
    variableEditor->blockSignals(false);
}

void MainWindow::initKeyFrameControl() {
    if( engine->eyeSpline != NULL || engine->targetSpline != NULL || engine->upSpline != NULL)
        clearKeyFrameControl();

    int c = variableEditor->getPresetCount();
    int k = variableEditor->getKeyFrameCount();

    if(k>0) engine->setHasKeyFrames(true);

    if(k>1) {
        variableEditor->setKeyFramesEnabled(true);
        timeSlider->setTickInterval( timeSlider->maximum()/(k-1));
        timeSlider->setTickPosition(QSlider::TicksBelow);
        /// more than 1? try to spline
        /// setup splines for Eye Target and Up vectors
        if(c>1) {
            for(int i =0; i<c; i++) {
                QString presetname = variableEditor->getPresetName(i);
                if(presetname.contains("KeyFrame")) { /// found a keyframe, add to list
                    addKeyFrame(presetname);
                    /// for each key frame add ctrl point to the list
                    QStringList ps = variableEditor->getPresetByName(presetname);

                    if(engine->cameraID() == "3D" ) {
                      QStringList in = ps.filter("Eye ").at(0).split("=").at(1).split(",");
                      QVector3D eyeCp = QVector3D(in.at(0).toFloat(),in.at(1).toFloat(),in.at(2).toFloat());

                      in = ps.filter("Target").at(0).split("=").at(1).split(",");
                      QVector3D tarCp = QVector3D(in.at(0).toFloat(),in.at(1).toFloat(),in.at(2).toFloat());

                      in = ps.filter("Up").at(0).split("=").at(1).split(",");
                      QVector3D upCp = QVector3D(in.at(0).toFloat(),in.at(1).toFloat(),in.at(2).toFloat());

                      engine->addControlPoint(eyeCp,tarCp,upCp);
                    }
                }
            }

            if( engine->cameraID() == "3D" ) {
                QVector3D *eyeCp = engine->getControlPoints(1);
                QVector3D *tarCp = engine->getControlPoints(2);
                QVector3D *upCp =  engine->getControlPoints(3);

                if(eyeCp != NULL && tarCp != NULL && upCp != NULL) {
                    int segs = timeSlider->maximum()+1; // time 0:0 = frame 00:01
                    engine->eyeSpline = new QtSpline(engine,k,segs, eyeCp);
                    engine->targetSpline = new QtSpline(engine,k,segs, tarCp);
                    engine->upSpline = new QtSpline(engine,k,segs, upCp);
                    engine->eyeSpline->setSplineColor( QColor("red") );
                    engine->eyeSpline->setControlColor( QColor("green"));
                    engine->targetSpline->setSplineColor( QColor("blue"));
                    engine->targetSpline->setControlColor( QColor("green"));
                }
            }
        }
    }
}

void MainWindow::clearKeyFrameControl() {
    variableEditor->setKeyFramesEnabled(false);
    if( engine->eyeSpline != NULL || engine->targetSpline != NULL || engine->upSpline != NULL) {
        keyFrameList = QStringList();
        engine->clearControlPoints();
        engine->eyeSpline = NULL;
        engine->targetSpline = NULL;
        engine->upSpline = NULL;
        engine->setHasKeyFrames(false);
        timeSlider->setTickPosition(QSlider::NoTicks);
    }
}

void MainWindow::addKeyFrame(QString name) {
    //check to see if the frame name already exists
    int i = keyFrameList.indexOf(name);
    if ( i == -1 ) { // not found
        // add it to the list
        keyFrameList << name;
    } else //found so replace
        keyFrameList.replace(i,name);
}

void MainWindow::selectPreset() {

    QString pName;

    pName = QString("#preset %1").arg(variableEditor->getPresetName());

    TextEdit* te = getTextEdit();

    QTextCursor tc= te->textCursor();
    tc.setPosition(0);

    te->setTextCursor(tc);

    bool found = te->find( pName );
    if(found ) {
      tc = te->textCursor();
      tc.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor);
      found = te->find("#endpreset");
      if(found) {
        tc.setPosition(te->textCursor().position()+1, QTextCursor::KeepAnchor);
        te->setTextCursor(tc);
      } else statusBar()->showMessage(tr("#endpreset not found!"));
    } else statusBar()->showMessage(tr( QString(pName + " not found!").toStdString().c_str() ));
}

void MainWindow::processGuiEvents() {
  // Immediately dispatches all queued events
  qApp->sendPostedEvents();
  // Processes all pending events until there are no more events to process
#ifdef Q_OS_UNIX
  while(qApp->hasPendingEvents())
#endif
    qApp->processEvents();

}

#ifdef NVIDIAGL4PLUS
void MainWindow::dumpShaderAsm() {
    if(engine->hasShader())
        AsmBrowser::showPage(engine->shaderAsm(true), "Shader Program");
    if(engine->hasBufferShader())
        AsmBrowser::showPage(engine->shaderAsm(false), "Buffershader Program");
}
#endif // NVIDIAGL4PLUS

void MainWindow::saveCmdScript() {

    QTextEdit *e = sender()->parent()->findChild<QTextEdit*>("cmdScriptEditor", Qt::FindChildrenRecursively);
    scriptText = e->toPlainText();

    QString filter = tr("Cmd Script (*.fqs);;All Files (*.*)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), QString(), filter);
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Fragmentarium"),
                             tr("Cannot write CmdScript %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << scriptText;
    file.close();
    INFO(tr("Cmd Script saved to file:")+fileName);
}

void MainWindow::loadCmdScript() {
    QString filter = tr("Cmd Script (*.fqs);;All Files (*.*)");
    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(), filter);
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("Fragmentarium"),
                                 tr("Cannot read file %1:\n%2.")
                                 .arg(fileName)
                                 .arg(file.errorString()));
            return;
        }

        QTextStream in(&file);
        scriptText = in.readAll();
        file.close();
        INFO(tr("Cmd Script loaded from file: ") + fileName);
    }
}

void MainWindow::editScript() {

    if(scriptText.isEmpty()) loadCmdScript();

    // we need a dialog
    QDialog *d;
    d = new QDialog();
    // with a text editor
    QTextEdit *t;
    t = new QTextEdit();
    // name these objects
    d->setObjectName("cmdScriptDialog");
    t->setObjectName("cmdScriptEditor");
    // setup some buttons
    QPushButton *saveButton = new QPushButton(tr("&Save"));
    QPushButton *executeButton = new QPushButton(tr("&Execute"));
    QPushButton *stopButton = new QPushButton(tr("&Stop"));
    QPushButton *closeButton = new QPushButton(tr("&Close"));
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(saveButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(executeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(stopButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    // setup the main layout with text editor
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(t);
    mainLayout->addLayout(buttonLayout);
    d->setLayout(mainLayout);
    // give the buttons something to do
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveCmdScript()));
    connect(executeButton, SIGNAL(clicked()), this, SLOT(executeScript()));
    connect(stopButton, SIGNAL(clicked()), this, SLOT(stopScript()));
    connect(closeButton, SIGNAL(clicked()), d, SLOT(close()));
    // display the script text
    t->setText(scriptText);
    // cmdScriptLineNumber != 0 indicates an error from the last execute cycle
    // so move the cursor and highlight line
    if(cmdScriptLineNumber != 0) {
        QTextCursor tc = t->textCursor();
        tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,cmdScriptLineNumber);
        tc.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor,1);
        t->setTextCursor(tc);
    }
    // start with this size
    d->resize(640,480);
    // open the dialog and take control
    d->exec();
    // closed the window so...
    runningScript=false;

}

void MainWindow::executeScript()
{
    QTextEdit *e = NULL;

    QSettings settings;
    QString name = settings.value("filename").toString();

    if(sender() != 0) {
        e = sender()->parent()->findChild<QTextEdit*>("cmdScriptEditor", Qt::FindChildrenRecursively);
        scriptText = e->toPlainText();
    }

    runningScript=true;

    QScriptValue result = scriptEngine.evaluate( scriptText );

    if (result.isError())
    {
        QString err = result.toString();
        cmdScriptLineNumber = scriptEngine.uncaughtExceptionLineNumber();
        QString msg = tr("Error %1 at line %2").arg(err).arg(cmdScriptLineNumber);
        INFO(msg);
        // highlight the error line
        if(cmdScriptLineNumber != 0 && e != NULL) {
            QTextCursor tc = e->textCursor();
            tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
            tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,cmdScriptLineNumber-1);
            tc.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor,1);
            e->setTextCursor(tc);
        }
        runningScript=false;
        return;
    } else cmdScriptLineNumber = 0;

    settings.setValue("filename",name);

}

void MainWindow::setupScriptEngine(void)
{
    runningScript=false;
    // expose these widgets to the script
    appContext = scriptEngine.newQObject(this);
    scriptEngine.globalObject().setProperty("app", appContext);
}

/// BEGIN 3DTexture
// void MainWindow::setObjFile( QString ofn )
// {
//    engine->setObjFileName( ofn );
// }
/// END 3DTexture

}
}

