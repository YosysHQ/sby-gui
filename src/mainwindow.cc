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
#include <QTextCursor>
#include <QtXml>
#include <QFile>
#include <QGraphicsColorizeEffect>
#include "SciLexer.h"
#include "ScintillaEdit.h"
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
    std::vector<boost::filesystem::path> fileList;
    refreshLocation = path;
    if(boost::filesystem::exists(path.toStdString())) {
        if (boost::filesystem::is_directory(path.toStdString())) {
            currentFolder = path;
            fileList = getFilesInDir(currentFolder.toStdString().c_str(), ".sby");
        } else {
            boost::filesystem::path filepath(path.toStdString());
            currentFolder = filepath.parent_path().string().c_str();
            fileList.push_back(filepath);
        }
    } 
    removeLayoutItems(grid);
    files.clear();

    // create new widgets
    int cnt = 0;
    for (auto file : fileList)
    {
        std::unique_ptr<SBYFile> f = std::make_unique<SBYFile>(file);
        f->parse();
        f->update();
        files.push_back(std::move(f));
    }

    for(const auto & file : files) {
        
        grid->addWidget(generateFileBox(file.get()), cnt++, 0);
    }

}
MainWindow::MainWindow(QString path, QWidget *parent) : QMainWindow(parent)
{
    initBasenameResource();
    qRegisterMetaType<std::string>();

    process = nullptr;

    setObjectName(QStringLiteral("MainWindow"));
    resize(1024, 768);

    setWindowIcon(QIcon(":/icons/resources/symbiotic.png"));

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
    tabWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    tabWidget->setMinimumHeight(200);
    tabWidget->setMaximumHeight(200);

    QFont f("unexistent");
    f.setStyleHint(QFont::Monospace);

    log = new QPlainTextEdit();
    log->setReadOnly(true);
    log->setFont(f);
    tabWidget->addTab(log, "Log");    

    centralTabWidget = new QTabWidget();
    centralTabWidget->setTabsClosable(true);
    centralTabWidget->setMovable(true);
    connect(centralTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

    splitter_v->addWidget(centralTabWidget);
    splitter_v->addWidget(tabWidget);

    openLocation(path);

    taskTimer = new QTime;
    taskTimer->start();
    
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::showTime);
    timer->start(1000);
}

void MainWindow::showTime()
{
    if (process==nullptr)  {
        timeDisplay->setText("");
        return;
    }
    QTime time = QTime::fromMSecsSinceStartOfDay(taskTimer->elapsed());
    QString text = time.toString("hh:mm:ss");
    if ((time.second() % 2) == 0) {
        text[2] = ' ';
        text[5] = ' ';
    }
    timeDisplay->setText(text);
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

    setStyleSheet("QStatusBar::item { border: 0px solid black }; ");
    timeDisplay = new QLabel();
    timeDisplay->setContentsMargins(0, 0, 0, 0);
    statusBar = new QStatusBar();
    statusBar->addPermanentWidget(timeDisplay);
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
    actionRefresh = new QAction("Refresh", this);
    actionRefresh->setIcon(QIcon(":/icons/resources/view-refresh.png"));
    connect(actionRefresh, &QAction::triggered, [=]() { refreshView(); });
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
    //actionUndo->setShortcuts(QKeySequence::Undo);
    menu_Edit->addAction(actionUndo);
    actionRedo = new QAction("Redo", this);
    actionRedo->setIcon(QIcon(":/icons/resources/edit-redo.png"));
    //actionRedo->setShortcuts(QKeySequence::Redo);
    menu_Edit->addAction(actionRedo);
    menu_Edit->addSeparator();
    actionCut = new QAction("Cut", this);
    actionCut->setIcon(QIcon(":/icons/resources/edit-cut.png"));
    //actionCut->setShortcuts(QKeySequence::Cut);
    menu_Edit->addAction(actionCut);
    actionCopy = new QAction("Copy", this);
    actionCopy->setIcon(QIcon(":/icons/resources/edit-copy.png"));
    //actionCopy->setShortcuts(QKeySequence::Copy);
    menu_Edit->addAction(actionCopy);
    actionPaste = new QAction("Paste", this);
    actionPaste->setIcon(QIcon(":/icons/resources/edit-paste.png"));
    //actionPaste->setShortcuts(QKeySequence::Paste);
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
    mainToolBar->addAction(actionRefresh);
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

QGroupBox *MainWindow::generateFileBox(SBYFile *file)
{
    QSBYItem *fileBox = new QSBYItem(file->getName().c_str(), file, this);  
    connect(fileBox, &QSBYItem::appendLog, this, &MainWindow::appendLog);
    connect(fileBox, &QSBYItem::editOpen, this, &MainWindow::editOpen);
    connect(fileBox, &QSBYItem::previewOpen, this, &MainWindow::previewOpen);
  
    for (auto const & task : file->getTasks())
    {
        QSBYItem *groupBox = new QSBYItem(task->getName().c_str(), task.get(), this);       
        connect(groupBox, &QSBYItem::appendLog, this, &MainWindow::appendLog);
        connect(groupBox, &QSBYItem::editOpen, this, &MainWindow::editOpen);
        connect(groupBox, &QSBYItem::previewOpen, this, &MainWindow::previewOpen);
        fileBox->layout()->addWidget(groupBox);
    }
    return fileBox;
}

ScintillaEdit *MainWindow::openEditor()
{
    ScintillaEdit *editor = new ScintillaEdit();
    editor->styleClearAll();
    editor->setMarginWidthN(0, 35);
    editor->setScrollWidth(200);
    editor->setScrollWidthTracking(1);
    return editor;
}

ScintillaEdit *MainWindow::openEditorFile(std::string fullpath)
{
    ScintillaEdit *editor = openEditor();
    QFile file(fullpath.c_str());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray contents = file.readAll();
        editor->get_doc()->insert_string(0, contents);
    }
    return editor;
}

ScintillaEdit *MainWindow::openEditorText(std::string text)
{
    ScintillaEdit *editor = openEditor();
    QByteArray contents;
    contents.append(text.c_str());
    editor->get_doc()->insert_string(0, contents);
    return editor;
}

void MainWindow::previewOpen(std::string content, std::string fileName, std::string taskName)
{
    std::string name = fileName + "#" + taskName;
 
    for(int i=0;i<centralTabWidget->count();i++) {
        if(centralTabWidget->tabText(i) == QString(name.c_str())) { 
            centralTabWidget->setCurrentIndex(i); 
            return; 
        } 
    }

    ScintillaEdit *editor = openEditorText(content);
    editor->setReadOnly(true);

    centralTabWidget->addTab(editor, name.c_str());
    centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);
}

void MainWindow::editOpen(std::string path, std::string fileName)
{
    std::string name = fileName;
    for(int i=0;i<centralTabWidget->count();i++) {
        if(centralTabWidget->tabText(i) == QString(name.c_str())) { 
            centralTabWidget->setCurrentIndex(i); 
            return; 
        } 
    }
    ScintillaEdit *editor = openEditorFile(path);
    connect(editor, &ScintillaEdit::modified, [=]() { 
        for(int i=0;i<centralTabWidget->count();i++) {
            if(centralTabWidget->tabText(i) == QString(name.c_str())) { 
                centralTabWidget->tabBar()->setTabTextColor(i, Qt::red);
                centralTabWidget->setTabIcon(i, QIcon(":/icons/resources/disk.png"));
                return; 
            } 
        }
    });

    centralTabWidget->addTab(editor, QIcon(":/icons/resources/script.png"), name.c_str());
    centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);
}

void MainWindow::appendLog(QString logline)
{
    log->moveCursor(QTextCursor::End);
    log->insertPlainText (logline);
    log->moveCursor(QTextCursor::End);
}

void MainWindow::refreshView()
{
    openLocation(refreshLocation);
    appendLog("All processes killed\n");
}