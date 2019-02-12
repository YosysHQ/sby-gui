/*
 *  sby-gui -- SymbiYosys GUI
 *
 *  Copyright (C) 2019  Miodrag Milanovic <miodrag@symbioticeda.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "mainwindow.h"
#include <QAction>
#include <QFileDialog>
#include <QGridLayout>
#include <QIcon>
#include <QRadioButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTabBar>
#include <QProgressBar>
#include <QHBoxLayout>
#include "SciLexer.h"
#include "ScintillaEdit.h"
#include "sbyparser.h"
#include <fstream>

std::vector<boost::filesystem::path> getFilesInDir(const std::string &path, const std::string &extension){
    std::vector<boost::filesystem::path> files;
    boost::filesystem::path dir(path);
    if(boost::filesystem::exists(path) && boost::filesystem::is_directory(path)) {
        boost::filesystem::directory_iterator it(path);
        boost::filesystem::directory_iterator endit;
        while (it != endit) {
            if(boost::filesystem::is_regular_file(*it) && (extension=="")?true:it->path().extension() == extension) {
                files.push_back(it->path());
            }
            ++it;
        }
    }
    return files;
}

static void initBasenameResource() { Q_INIT_RESOURCE(base); }

void MainWindow::removeLayoutItems(QLayout* layout)
{
    while(layout->count()!=0)
    {
        QLayoutItem* child = layout->takeAt(0);
        if(child->layout() != 0)
        {
            removeLayoutItems(child->layout());
        }
        else if(child->widget() != 0)
        {
            delete child->widget();
        }

        delete child;
    }
}

void MainWindow::openLocation(QString path)
{
    std::vector<boost::filesystem::path> files;
    if(boost::filesystem::exists(path.toStdString())) {
        if (boost::filesystem::is_directory(path.toStdString())) {
            currentFolder = path;
            files = getFilesInDir(currentFolder.toStdString().c_str(), ".sby");
        } else {
            boost::filesystem::path filepath(path.toStdString());
            currentFolder = filepath.parent_path().string().c_str();
            files.push_back(filepath);
        }
    } 
    removeLayoutItems(grid);
    // create new widgets
    int cnt = 0;
    for(auto file : files) {
        grid->addWidget(generateFileBox(file), cnt++, 0);
    }

}
MainWindow::MainWindow(QString path, QWidget *parent) : QMainWindow(parent)
{
    initBasenameResource();
    qRegisterMetaType<std::string>();

    setObjectName(QStringLiteral("MainWindow"));
    resize(1024, 768);

    createMenusAndBars();

    QWidget *centralWidget = new QWidget(this);

    QGridLayout *gridLayout = new QGridLayout(centralWidget);
    gridLayout->setSpacing(2);
    gridLayout->setContentsMargins(2, 2, 2, 2);

    QSplitter *splitter_h = new QSplitter(Qt::Horizontal, centralWidget);
    QSplitter *splitter_v = new QSplitter(Qt::Vertical, splitter_h);

    grid = new QGridLayout;

    QWidget *container = new QWidget();    
    container->setLayout(grid);

    QScrollArea *scrollArea = new QScrollArea();
    // scrollArea->setBackgroundRole(QPalette::Mid);
    scrollArea->setWidget(container);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setMinimumWidth(400);
    scrollArea->setMaximumWidth(400);
    scrollArea->setWidgetResizable(true);

    splitter_h->addWidget(scrollArea);
    splitter_h->addWidget(splitter_v);

    gridLayout->addWidget(splitter_h, 0, 0, 1, 1);

    setCentralWidget(centralWidget);

    tabWidget = new QTabWidget();
    tabWidget->setMinimumHeight(200);
    centralTabWidget = new QTabWidget();
    centralTabWidget->setTabsClosable(true);
    connect(centralTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

    splitter_v->addWidget(centralTabWidget);
    splitter_v->addWidget(tabWidget);

    /*for(auto f : files) {
        ScintillaEdit *editor = new ScintillaEdit();
        editor->styleClearAll();
        editor->setMarginWidthN(0, 35);
        editor->setScrollWidth(200);
        editor->setScrollWidthTracking(1);
        QFile file(f.string().c_str());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray contents = file.readAll();
            editor->get_doc()->insert_string(0, contents);
        }
        centralTabWidget->addTab(editor, f.filename().c_str());
        centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);
    }*/
        
    openLocation(path);
}

MainWindow::~MainWindow() {}

void MainWindow::closeTab(int index) { centralTabWidget->removeTab(index); }

void MainWindow::createMenusAndBars()
{
    QAction *actionAbout = new QAction("About", this);

    menuBar = new QMenuBar();
    QMenu *menu_File = new QMenu("&File", menuBar);
    QMenu *menu_Edit = new QMenu("&Edit", menuBar);
    QMenu *menu_Search = new QMenu("&Search", menuBar);
    QMenu *menu_Help = new QMenu("&Help", menuBar);

    menuBar->addAction(menu_File->menuAction());
    menuBar->addAction(menu_Edit->menuAction());
    menuBar->addAction(menu_Search->menuAction());
    menuBar->addAction(menu_Help->menuAction());
    setMenuBar(menuBar);

    mainToolBar = new QToolBar();
    addToolBar(Qt::TopToolBarArea, mainToolBar);

    statusBar = new QStatusBar();
    setStatusBar(statusBar);

    actionNew = new QAction("New", this);
    actionNew->setIcon(QIcon(":/icons/resources/document-new.png"));
    actionNew->setShortcuts(QKeySequence::New);
    actionNew->setStatusTip("New project file");
    menu_File->addAction(actionNew);
    actionOpen = new QAction("Open SBY...", this);
    actionOpen->setIcon(QIcon(":/icons/resources/document-open.png"));
    actionOpen->setShortcuts(QKeySequence::Open);
    actionOpen->setStatusTip("Open existing SBY file");
    connect(actionOpen, &QAction::triggered, this, &MainWindow::open_sby);

    menu_File->addAction(actionOpen);
    actionOpenFolder = new QAction("Open Folder...", this);
    actionOpenFolder->setIcon(QIcon(":/icons/resources/folder-open.png"));
    actionOpenFolder->setStatusTip("Open folder with SBY file(s)");
    connect(actionOpenFolder, &QAction::triggered, this, &MainWindow::open_folder);
    menu_File->addAction(actionOpenFolder);
    actionSave = new QAction("Save", this);
    actionSave->setIcon(QIcon(":/icons/resources/document-save.png"));
    actionSave->setShortcuts(QKeySequence::Save);
    actionSave->setStatusTip("Save SBY file(s)");
    menu_File->addAction(actionSave);
    actionSaveAs = new QAction("Save As...", this);
    actionSaveAs->setIcon(QIcon(":/icons/resources/document-save-as.png"));
    actionSaveAs->setStatusTip("Save SBY under different name");
    menu_File->addAction(actionSaveAs);
    menu_File->addAction(new QAction("Save All", this));
    menu_File->addAction(new QAction("Rename...", this));
    menu_File->addAction(new QAction("Close", this));
    menu_File->addAction(new QAction("Close all", this));
    menu_File->addAction(new QAction("Close More", this));
    menu_File->addSeparator();
    menu_File->addAction(new QAction("recent.txt", this));
    menu_File->addSeparator();
    menu_File->addAction(new QAction("Restore Recent Closed File", this));
    menu_File->addAction(new QAction("Open All Recent Files", this));
    menu_File->addAction(new QAction("Empty Recent Files List", this));
    menu_File->addSeparator();
    actionExit = new QAction("Exit", this);
    actionExit->setIcon(QIcon(":/icons/resources/system-log-out.png"));
    actionExit->setShortcuts(QKeySequence::Quit);
    actionExit->setStatusTip("Exit application");
    connect(actionExit, &QAction::triggered, this, &MainWindow::close);
    menu_File->addAction(actionExit);

    actionUndo = new QAction("Undo", this);
    actionUndo->setIcon(QIcon(":/icons/resources/edit-undo.png"));
    actionUndo->setShortcuts(QKeySequence::Undo);
    menu_Edit->addAction(actionUndo);
    actionRedo = new QAction("Redo", this);
    actionRedo->setIcon(QIcon(":/icons/resources/edit-redo.png"));
    actionRedo->setShortcuts(QKeySequence::Redo);
    menu_Edit->addAction(actionRedo);
    menu_Edit->addSeparator();
    actionCut = new QAction("Cut", this);
    actionCut->setIcon(QIcon(":/icons/resources/edit-cut.png"));
    actionCut->setShortcuts(QKeySequence::Cut);
    menu_Edit->addAction(actionCut);
    actionCopy = new QAction("Copy", this);
    actionCopy->setIcon(QIcon(":/icons/resources/edit-copy.png"));
    actionCopy->setShortcuts(QKeySequence::Copy);
    menu_Edit->addAction(actionCopy);
    actionPaste = new QAction("Paste", this);
    actionPaste->setIcon(QIcon(":/icons/resources/edit-paste.png"));
    actionPaste->setShortcuts(QKeySequence::Paste);
    menu_Edit->addAction(actionPaste);
    menu_Edit->addAction(new QAction("Delete", this));
    menu_Edit->addAction(new QAction("Select All", this));

    menu_Search->addAction(new QAction("Find...", this));
    menu_Search->addAction(new QAction("Find Next", this));
    menu_Search->addAction(new QAction("Find Previous", this));
    menu_Search->addAction(new QAction("Replace...", this));
    menu_Search->addAction(new QAction("Go to...", this));

    menu_Help->addAction(new QAction("About", this));

    mainToolBar->addAction(actionNew);
    mainToolBar->addAction(actionOpen);
    mainToolBar->addAction(actionOpenFolder);
    mainToolBar->addAction(actionSave);
}

void MainWindow::open_sby()
{
    QString fileName =
            QFileDialog::getOpenFileName(this, QString("Open SBY"), QString(),
                                         QString("SBY files (*.sby)"));
    if (!fileName.isEmpty()) {
        openLocation(fileName);
    }
}

void MainWindow::open_folder()
{
    QString folderName =
            QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                QString(),
                                                QFileDialog::ShowDirsOnly
                                                | QFileDialog::DontResolveSymlinks);
    if (!folderName.isEmpty()) {
        openLocation(folderName);
    }
}
QGroupBox *MainWindow::generateFileBox(boost::filesystem::path path)
{
    QGroupBox *fileBox = new QGroupBox(path.filename().c_str());

    QString styleFileBox = "QGroupBox { border: 3px solid gray; border-radius: 3px; margin-top: 0.5em; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }";
    fileBox->setStyleSheet(styleFileBox);     
    fileBox->setMinimumWidth(370);
    fileBox->setMaximumWidth(370);

    SBYParser parser;
    std::fstream fs;
    fs.open(path.string().c_str(), std::fstream::in);
    parser.parse(fs);
    fs.close();
    
    QVBoxLayout *vboxFile = new QVBoxLayout;

    QHBoxLayout *hboxFile = new QHBoxLayout;

    QProgressBar *fileProgressBar = new QProgressBar();
    fileProgressBar->setValue(100);
    QToolBar *toolBarFile = new QToolBar();
    QAction *actionPlayFile = new QAction("Play", this);
    actionPlayFile->setIcon(QIcon(":/icons/resources/media-playback-start.png"));    
    toolBarFile->addAction(actionPlayFile);
    QAction *actionPauseFile = new QAction("Pause", this);
    actionPauseFile->setIcon(QIcon(":/icons/resources/media-playback-pause.png"));    
    toolBarFile->addAction(actionPauseFile);
    QAction *actionStopFile = new QAction("Stop", this);
    actionStopFile->setIcon(QIcon(":/icons/resources/media-playback-stop.png"));    
    toolBarFile->addAction(actionStopFile);

    hboxFile->addWidget(fileProgressBar);
    hboxFile->addWidget(toolBarFile);
    QWidget *dummyFile = new QWidget();
    dummyFile->setLayout(hboxFile);
    vboxFile->addWidget(dummyFile);

    QString styleTaskBox = "QGroupBox { border: 1px solid gray; border-radius: 3px; margin-top: 0.5em; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }";
    for (auto task : parser.get_tasks())
    {
        QGroupBox *groupBox = new QGroupBox(task.c_str());
        groupBox->setStyleSheet(styleTaskBox);     

        QVBoxLayout *vbox = new QVBoxLayout;        
        QProgressBar *progressBar = new QProgressBar();
        progressBar->setValue(0);

        QHBoxLayout *hbox = new QHBoxLayout;

        QToolBar *toolBar = new QToolBar();
        QAction *actionPlay = new QAction("Play", this);
        actionPlay->setIcon(QIcon(":/icons/resources/media-playback-start.png"));    
        toolBar->addAction(actionPlay);
        QAction *actionPause = new QAction("Pause", this);
        actionPause->setIcon(QIcon(":/icons/resources/media-playback-pause.png"));    
        toolBar->addAction(actionPause);
        QAction *actionStop = new QAction("Stop", this);
        actionStop->setIcon(QIcon(":/icons/resources/media-playback-stop.png"));    
        toolBar->addAction(actionStop);

        hbox->addWidget(progressBar);
        hbox->addWidget(toolBar);
        QWidget *dummy = new QWidget();
        dummy->setLayout(hbox);
        vbox->addWidget(dummy);
        groupBox->setLayout(vbox);
        vboxFile->addWidget(groupBox);
    }
    fileBox->setLayout(vboxFile);
    return fileBox;
}