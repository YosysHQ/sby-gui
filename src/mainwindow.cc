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
#include <QMessageBox>
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

QStringList MainWindow::getFileList(QString path)
{
    QDir folder(currentFolder);
    folder.setNameFilters(QStringList()<<"*.sby");
    return folder.entryList();
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
            QMessageBox::critical(this, "SBY Gui",
                                "Invalid file location",
                                QMessageBox::Ok);
            return;
        }

        currentFileList = getFileList(currentFolder);
        fileWatcher->addPath(currentFolder);
        for(auto name : currentFileList) {
            fileWatcher->addPath(QDir(currentFolder).filePath(name));
        }        
    } 

    items.clear();
    removeLayoutItems(grid);
    fileMap.clear();
    files.clear();
    taskList.clear();

    // create new widgets
    int cnt = 0;
    for (auto file : fileList)
    {
        std::unique_ptr<SBYFile> f = std::make_unique<SBYFile>(file);
        f->parse();
        f->update();
        files.push_back(std::move(f));
        fileMap.emplace(std::make_pair(file.string(), files.back().get()));
    }

    for(const auto & file : files) {
        
        grid->addWidget(generateFileBox(file.get()), cnt++, 0);
    }
    grid->setRowStretch(cnt++,1);
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
    connect(centralTabWidget, &QTabWidget::tabCloseRequested, [=](int index) { closeTab(index, false); });

    splitter_v->addWidget(centralTabWidget);
    splitter_v->addWidget(tabWidget);

    fileWatcher = new QFileSystemWatcher(this);
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::fileChanged);
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::directoryChanged);

    openLocation(path);

    taskTimer = new QTime;
    taskTimer->start();
    
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::showTime);
    timer->start(1000);
}

void MainWindow::directoryChanged(const QString & path)
{
    QStringList newFileList = getFileList(currentFolder);

    QSet<QString> newDirSet = QSet<QString>::fromList(newFileList); 
    QSet<QString> currentDirSet = QSet<QString>::fromList(currentFileList);
    QSet<QString> newFiles = newDirSet - currentDirSet;
    QSet<QString> deletedFiles = currentDirSet - newDirSet;


    QStringList newList = newFiles.toList();
    QStringList deleteList = deletedFiles.toList();

    if(!newList.isEmpty())
    {
        for(auto name : newList) {
            QString filename = QDir(currentFolder).filePath(name);
            fileWatcher->addPath(filename);
            std::unique_ptr<SBYFile> f = std::make_unique<SBYFile>(boost::filesystem::path(filename.toStdString().c_str()));
            f->parse();
            f->update();
            files.push_back(std::move(f));
            fileMap.emplace(std::make_pair(filename.toStdString(), files.back().get()));

            grid->addWidget(generateFileBox(files.back().get()), (int)files.size(), 0);
        }
    }
    if(!deleteList.isEmpty())
    {
        for(auto name : deleteList) {
            QString filename = QDir(currentFolder).filePath(name);
            fileWatcher->removePath(filename);
            SBYFile *file = fileMap[filename.toStdString()];
            for (auto const & task : file->getTasks())
            {
                std::string name = file->getName() + "#" + task->getName();
                auto it = items.find(name);
                if (it!=items.end()) {
                    items.erase(it);
                }
            }
            auto it = items.find(file->getName());
            if (it!=items.end()) {
                items.erase(it);
            }
            auto itFile = files.begin();
            while(itFile != files.end()) {
                if (itFile->get()->getFullPath() == filename.toStdString()) {
                    files.erase(itFile);
                }
                else ++itFile;
            }
        }
    }    
    currentFileList = newFileList;
}

void MainWindow::fileChanged(const QString & filename)
{
    SBYFile *file = fileMap[filename.toStdString()];
    std::unique_ptr<SBYFile> f = std::make_unique<SBYFile>(boost::filesystem::path(filename.toStdString().c_str()));
    f->parse();
    f->update();
    QSet<QString> newTaskSet = f->getTasksList(); 
    QSet<QString> currTaskSet = file->getTasksList(); 
    QSet<QString> newTasks = newTaskSet - currTaskSet;
    QSet<QString> deletedTasks = currTaskSet - newTaskSet;

    if(!deletedTasks.isEmpty())
    {
        for(auto name : deletedTasks) {
            std::string n = file->getFileName() + "#" + name.toStdString();
            auto it = items.begin();
            while(it != items.end()) {
                if (it->second->getName() == n) {
                    items.erase(it);
                    break;                    
                }
                else ++it;
            }
        }
    } 
    
    file->refresh();

    if(!newTasks.isEmpty())
    {
        for(auto name : newTasks) {
            QSBYItem *fileBox = items[file->getFileName()].get();
            std::unique_ptr<QSBYItem> groupBox = std::make_unique<QSBYItem>(name.toStdString().c_str(), file->getTask(name.toStdString()), fileBox, this);
            std::string newname = groupBox->getName();
            connect(groupBox.get(), &QSBYItem::appendLog, this, &MainWindow::appendLog);
            connect(groupBox.get(), &QSBYItem::editOpen, this, &MainWindow::editOpen);
            connect(groupBox.get(), &QSBYItem::previewOpen, this, &MainWindow::previewOpen);
            connect(groupBox.get(), &QSBYItem::previewLog, this, &MainWindow::previewLog);
            connect(groupBox.get(), &QSBYItem::taskExecuted, this, &MainWindow::taskExecuted);
            connect(groupBox.get(), &QSBYItem::startTask, this, &MainWindow::startTask);
            fileBox->layout()->addWidget(groupBox.get());            
            items.emplace(std::make_pair(newname, std::move(groupBox)));
        }
    }
    items[file->getFileName()]->refreshView();
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
    actionOpenFolder = new QAction("Open Folder...", this);
    actionOpenFolder->setIcon(QIcon(":/icons/resources/folder-open.png"));
    actionOpenFolder->setStatusTip("Open folder with SBY file(s)");
    connect(actionOpenFolder, &QAction::triggered, this, &MainWindow::open_folder);
    menu_File->addAction(actionOpenFolder);
    actionSave = new QAction("Save", this);
    actionSave->setIcon(QIcon(":/icons/resources/document-save.png"));
    actionSave->setShortcuts(QKeySequence::Save);
    actionSave->setStatusTip("Save SBY file");
    connect(actionSave, &QAction::triggered, this, &MainWindow::save_file);
    menu_File->addAction(actionSave);
    actionSaveAs = new QAction("Save As...", this);
    actionSaveAs->setIcon(QIcon(":/icons/resources/document-save-as.png"));
    actionSaveAs->setStatusTip("Save SBY under different name");
    menu_File->addAction(actionSaveAs);
    actionRefresh = new QAction("Refresh", this);
    actionRefresh->setIcon(QIcon(":/icons/resources/view-refresh.png"));
    connect(actionRefresh, &QAction::triggered, [=]() { refreshView(); });
    menu_File->addAction(actionSaveAs);

    actionSaveAll = new QAction("Save All", this);
    actionSaveAll->setStatusTip("Save all SBY files");
    connect(actionSaveAll, &QAction::triggered, this, &MainWindow::save_all);
    menu_File->addAction(actionSaveAll);

    actionClose = new QAction("Close...", this);
    actionClose->setShortcuts(QKeySequence::Close);
    actionClose->setStatusTip("Close current editor");
    connect(actionClose, &QAction::triggered, this, &MainWindow::close_editor);
    menu_File->addAction(actionClose);

    actionCloseAll = new QAction("Close all", this);
    actionCloseAll->setStatusTip("Close all editors");
    connect(actionCloseAll, &QAction::triggered, this, &MainWindow::close_all);
    menu_File->addAction(actionCloseAll);
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
    mainToolBar->addAction(actionOpenFolder);
    mainToolBar->addAction(actionSave);
    mainToolBar->addAction(actionRefresh);

    actionPlay = new QAction("Play", this);
    actionPlay->setIcon(QIcon(":/icons/resources/media-playback-start.png")); 
    mainToolBar->addAction(actionPlay);
    actionStop = new QAction("Stop", this);
    actionStop->setIcon(QIcon(":/icons/resources/media-playback-stop.png"));    
    actionStop->setEnabled(false);
    mainToolBar->addAction(actionStop);
    connect(actionPlay, &QAction::triggered, [=]() { 
        for (auto & item : files)
        {
            if (item->haveTasks())  {
                for(const auto & task : item->getTasks())
                    Q_EMIT startTask(item->getFileName() + "#" + task->getTaskName()); 
            } else {
                Q_EMIT startTask(item->getFileName() ); 
            }
        }
    });   
    connect(actionStop, &QAction::triggered, [=]() { 
        if (taskList.size()>0)  {        
            std::string name = taskList.front();        
            taskList.clear();
            taskList.push_back(name); // Put back one to finish
            items[name]->stopProcess();
        }         
     });
}

void MainWindow::save_sby(int index)
{
    QWidget *current = centralTabWidget->widget(index);
    if (current!=nullptr)
    {
        if (std::string(current->metaObject()->className()) == "ScintillaEdit")
        {
            ScintillaEdit *editor = (ScintillaEdit*)current;
            if (editor->modify()){
                QString name = centralTabWidget->tabText(index);
                boost::filesystem::path filepath(currentFolder.toStdString());
                filepath /= name.toStdString();
                QFile file(filepath.c_str());
                if (file.open(QIODevice::WriteOnly)) {
                    QByteArray contents = editor->get_doc()->get_char_range(0, editor->get_doc()->length());
                    file.write(contents);
                    file.close();
                    editor->setSavePoint();                    
                    centralTabWidget->tabBar()->setTabTextColor(index, Qt::black);
                    centralTabWidget->setTabIcon(index, QIcon(":/icons/resources/script_edit.png"));
                }
            }
        }
    }
}

void MainWindow::save_all()
{
    if (actionStop->isEnabled()) {
        QMessageBox::information(this, "SBY Gui",
                                "The document can not be saved until tasks are done",
                                QMessageBox::Ok);
    } else {
        for (int i=0;i<centralTabWidget->count();i++)
        {
            save_sby(i);
        }
    }
}

void MainWindow::save_file()
{
    if (actionStop->isEnabled()) {
        QMessageBox::information(this, "SBY Gui",
                                "The document can not be saved until tasks are done",
                                QMessageBox::Ok);
    } else {
        save_sby(centralTabWidget->currentIndex());
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

void MainWindow::close_editor()
{
    closeTab(centralTabWidget->currentIndex(), false);
}

void MainWindow::close_all()
{
    bool forceSave = false;
    for (int i=centralTabWidget->count()-1;i>=0;i--)
    {
        forceSave = closeTab(i, forceSave);
    }
}

bool MainWindow::closeTab(int index, bool forceSave) { 
    bool force = forceSave;
    QWidget *current = centralTabWidget->widget(index);
    if (current!=nullptr)
    {
        if (std::string(current->metaObject()->className()) == "ScintillaEdit")
        {
            ScintillaEdit *editor = (ScintillaEdit*)current;
            if (editor->modify()) {
                bool save = false; 
                if (!force) {
                    int result = QMessageBox(QMessageBox::Information, "SBY Gui", "Do you wish to save file?", QMessageBox::Yes|QMessageBox::YesToAll|QMessageBox::No).exec();
                    if (result == QMessageBox::Yes || result == QMessageBox::YesToAll) save = true;
                    if (result == QMessageBox::YesToAll) force = true;
                }
                if (save)
                    save_sby(index);
            }
        }
    }

    centralTabWidget->removeTab(index);
    return force;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    if (QMessageBox::Yes == QMessageBox(QMessageBox::Information, "SBY Gui", "Are you sure you want to quit?", QMessageBox::Yes|QMessageBox::No).exec()) 
    {
        close_all();
        event->accept();    
    }    
}

void MainWindow::taskExecuted()
{   
    taskList.pop_front();
    if (taskList.size()>0)  {        
        std::string name = taskList.front();        
        items[name]->runSBYTask();
    } else {
        actionPlay->setEnabled(true); 
        actionStop->setEnabled(false); 
    }
}

void MainWindow::startTask(std::string name)
{   
    actionPlay->setEnabled(false); 
    actionStop->setEnabled(true);
    if (std::find(taskList.begin(),taskList.end(),name) == taskList.end()) 
    {
        taskList.push_back(name);
        if (taskList.size()==1)         
        {
            items[name]->runSBYTask();
        }
    }
}

QGroupBox *MainWindow::generateFileBox(SBYFile *file)
{
    std::unique_ptr<QSBYItem> fileBox = std::make_unique<QSBYItem>(file->getName().c_str(), file, nullptr, this);
    connect(fileBox.get(), &QSBYItem::appendLog, this, &MainWindow::appendLog);
    connect(fileBox.get(), &QSBYItem::editOpen, this, &MainWindow::editOpen);
    connect(fileBox.get(), &QSBYItem::previewOpen, this, &MainWindow::previewOpen);
    connect(fileBox.get(), &QSBYItem::previewLog, this, &MainWindow::previewLog);
    connect(fileBox.get(), &QSBYItem::taskExecuted, this, &MainWindow::taskExecuted);
    connect(fileBox.get(), &QSBYItem::startTask, this, &MainWindow::startTask);
  
    for (auto const & task : file->getTasks())
    {
        std::unique_ptr<QSBYItem> groupBox = std::make_unique<QSBYItem>(task->getName().c_str(), task.get(), fileBox.get(), this);
        std::string name = groupBox->getName();
        connect(groupBox.get(), &QSBYItem::appendLog, this, &MainWindow::appendLog);
        connect(groupBox.get(), &QSBYItem::editOpen, this, &MainWindow::editOpen);
        connect(groupBox.get(), &QSBYItem::previewOpen, this, &MainWindow::previewOpen);
        connect(groupBox.get(), &QSBYItem::previewLog, this, &MainWindow::previewLog);
        connect(groupBox.get(), &QSBYItem::taskExecuted, this, &MainWindow::taskExecuted);
        connect(groupBox.get(), &QSBYItem::startTask, this, &MainWindow::startTask);
        fileBox->layout()->addWidget(groupBox.get());
        items.emplace(std::make_pair(name, std::move(groupBox)));
    }
    items.emplace(std::make_pair(file->getName(), std::move(fileBox)));
    return items[file->getName()].get();
}

const char *MonospaceFont()
{
	static char fontNameDefault[200] = "";
	if (!fontNameDefault[0]) {
        const QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
		strcpy(fontNameDefault, font.family().toUtf8());
	}
	return fontNameDefault;
}

ScintillaEdit *MainWindow::openEditor()
{
    ScintillaEdit *editor = new ScintillaEdit();
    editor->styleSetFont(STYLE_DEFAULT, MonospaceFont());    
    editor->setMarginWidthN(0, 35);
    editor->setScrollWidth(200);
    editor->setScrollWidthTracking(1);
    editor->setUndoCollection(false);
    return editor;
}

ScintillaEdit *MainWindow::openEditorFile(std::string fullpath)
{
    ScintillaEdit *editor = openEditor();
    QFile file(fullpath.c_str());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray contents = file.readAll();
        editor->setText(contents.constData());
        editor->setUndoCollection(true);
        editor->setSavePoint();
        editor->gotoPos(0);
    }
    return editor;
}

ScintillaEdit *MainWindow::openEditorText(std::string text)
{
    ScintillaEdit *editor = openEditor();
    editor->setText(text.c_str());
    editor->setUndoCollection(true);
    editor->setSavePoint();
    editor->gotoPos(0);
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

    centralTabWidget->addTab(editor, QIcon(":/icons/resources/script.png"), name.c_str());
    centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);
}

void MainWindow::previewLog(std::string content, std::string fileName, std::string taskName)
{
    std::string name = fileName;
    if (!taskName.empty()) name+= "#" + taskName;
    name += ".log";
 
    for(int i=0;i<centralTabWidget->count();i++) {
        if(centralTabWidget->tabText(i) == QString(name.c_str())) { 
            centralTabWidget->setCurrentIndex(i); 
            return; 
        } 
    }

    ScintillaEdit *editor = openEditorText(content);
    editor->setReadOnly(true);

    centralTabWidget->addTab(editor, QIcon(":/icons/resources/book.png"), name.c_str());
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
                if(editor->modify()) {
                    centralTabWidget->tabBar()->setTabTextColor(i, Qt::red);
                    centralTabWidget->setTabIcon(i, QIcon(":/icons/resources/disk.png"));
                } else {
                    centralTabWidget->tabBar()->setTabTextColor(i, Qt::black);
                    centralTabWidget->setTabIcon(i, QIcon(":/icons/resources/script_edit.png"));
                }
                return; 
            } 
        }
    });

    centralTabWidget->addTab(editor, QIcon(":/icons/resources/script_edit.png"), name.c_str());
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
