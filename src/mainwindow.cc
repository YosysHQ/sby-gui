/*
 *  sby-gui -- SBY GUI
 *
 *  Copyright (C) 2019  Miodrag Milanovic <micko@yosyshq.com>
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
#include <QFile>
#include <QFileDialog>
#include <QGraphicsColorizeEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QMessageBox>
#include <QProgressBar>
#include <QRadioButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTabBar>
#include <QTextCursor>
#include <QtXml>
#include "Qsci/qscilexerverilog.h"
#include "Qsci/qscilexervhdl.h"
#include "Qsci/qsciscintilla.h"
#include "SciLexer.h"
#include "lexers/LexSBY.h"
#include "qscilexersby.h"
// clang-format off
#include "Catalogue.h"
// clang-format on

static void initBasenameResource() { Q_INIT_RESOURCE(base); }

void MainWindow::removeLayoutItems(QLayout *layout)
{
    while (layout->count() != 0) {
        QLayoutItem *child = layout->takeAt(0);
        if (child->layout() != 0) {
            removeLayoutItems(child->layout());
        } else if (child->widget() != 0) {
            delete child->widget();
        }

        delete child;
    }
}

QStringList MainWindow::getFileList(QDir path)
{
    QDir folder(path);
    folder.setNameFilters(QStringList() << "*.sby");
    return folder.entryList();
}

void MainWindow::openLocation(QFileInfo path)
{
    QFileInfoList fileList;
    refreshLocation = path;
    if (path.exists()) {
        if (path.isDir()) {
            currentFolder.setPath(path.absoluteFilePath());
            currentFolder.setNameFilters(QStringList() << "*.sby");
            fileList = currentFolder.entryInfoList();
            currentFileList = currentFolder.entryList();
        } else {
            QMessageBox::critical(this, "SBY Gui", "Invalid file location", QMessageBox::Ok);
            return;
        }
        fileWatcher->addPath(currentFolder.canonicalPath());
        for (auto name : fileList) {
            fileWatcher->addPath(name.absoluteFilePath());
        }
    }

    items.clear();
    removeLayoutItems(grid);
    fileMap.clear();
    files.clear();
    taskList.clear();

    // create new widgets
    int cnt = 0;
    for (auto file : fileList) {
        std::unique_ptr<SBYFile> f = std::make_unique<SBYFile>(file);
        f->parse();
        f->update();
        files.push_back(std::move(f));
        fileMap.insert(file.absoluteFilePath(), files.back().get());
    }

    for (const auto &file : files) {

        grid->addWidget(generateFileBox(file.get()), cnt++, 0);
    }
    grid->setRowStretch(cnt++, 1);
}
MainWindow::MainWindow(QString path, QWidget *parent) : QMainWindow(parent)
{
    initBasenameResource();
    Scintilla::Catalogue::AddLexerModule(&lmSBY);

    process = nullptr;

    setObjectName(QStringLiteral("MainWindow"));
    resize(1024, 768);

    setWindowIcon(QIcon(":/icons/resources/yosyshq.png"));

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
    splitter_h->setCollapsible(0, false);
    splitter_h->setCollapsible(1, false);
    splitter_h->setStretchFactor(0, 1);

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

    openLocation(QFileInfo(path));

    taskTimer = new QElapsedTimer();
    taskTimer->start();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::showTime);
    timer->start(1000);
}

void MainWindow::directoryChanged(const QString &path)
{
    QStringList newFileList = getFileList(currentFolder);

    QSet<QString> newDirSet(newFileList.begin(), newFileList.end());
    QSet<QString> currentDirSet(currentFileList.begin(), currentFileList.end());
    QSet<QString> newFiles = newDirSet - currentDirSet;
    QSet<QString> deletedFiles = currentDirSet - newDirSet;

    QStringList newList(newFiles.begin(), newFiles.end());
    QStringList deleteList(deletedFiles.begin(), deletedFiles.end());

    if (!newList.isEmpty()) {
        for (auto name : newList) {
            QString filename = QDir(currentFolder).filePath(name);
            fileWatcher->addPath(filename);
            std::unique_ptr<SBYFile> f = std::make_unique<SBYFile>(QFileInfo(filename));
            f->parse();
            f->update();
            files.push_back(std::move(f));
            fileMap.insert(filename, files.back().get());

            grid->addWidget(generateFileBox(files.back().get()), (int)files.size(), 0);
        }
    }
    if (!deleteList.isEmpty()) {
        for (auto name : deleteList) {
            QString filename = QDir(currentFolder).filePath(name);
            fileWatcher->removePath(filename);
            SBYFile *file = fileMap[filename];
            for (auto const &task : file->getTasks()) {
                QString name = file->getName() + "#" + task->getName();
                auto it = items.find(name);
                if (it != items.end()) {
                    items.erase(it);
                }
            }
            auto it = items.find(file->getName());
            if (it != items.end()) {
                items.erase(it);
            }
            auto itFile = files.begin();
            while (itFile != files.end()) {
                if (itFile->get()->getFullPath() == filename) {
                    files.erase(itFile);
                } else
                    ++itFile;
            }
        }
    }
    currentFileList = newFileList;
}

void MainWindow::fileChanged(const QString &filename)
{
    SBYFile *file = fileMap[filename];
    std::unique_ptr<SBYFile> f = std::make_unique<SBYFile>(QFileInfo(filename));
    f->parse();
    f->update();
    QSet<QString> newTaskSet = f->getTasksSet();
    QSet<QString> currTaskSet = file->getTasksSet();
    QSet<QString> newTasks = newTaskSet - currTaskSet;
    QSet<QString> deletedTasks = currTaskSet - newTaskSet;

    if (!deletedTasks.isEmpty()) {
        for (auto name : deletedTasks) {
            QString n = file->getFileName() + "#" + name;
            auto it = items.begin();
            while (it != items.end()) {
                if (it->second->getName() == n) {
                    items.erase(it);
                    break;
                } else
                    ++it;
            }
        }
    }

    file->refresh();

    if (!newTasks.isEmpty()) {
        for (auto name : newTasks) {
            QSBYItem *fileBox = items[file->getFileName()].get();
            std::unique_ptr<QSBYItem> groupBox = std::make_unique<QSBYItem>(name, file->getTask(name), fileBox, this);
            QString newname = groupBox->getName();

            connect(groupBox.get(), &QSBYItem::appendLog, this, &MainWindow::appendLog);
            connect(groupBox.get(), &QSBYItem::editOpen, this, &MainWindow::editOpen);
            connect(groupBox.get(), &QSBYItem::previewOpen, this, &MainWindow::previewOpen);
            connect(groupBox.get(), &QSBYItem::previewLog, this, &MainWindow::previewLog);
            connect(groupBox.get(), &QSBYItem::taskExecuted, this, &MainWindow::taskExecuted);
            connect(groupBox.get(), &QSBYItem::startTask, this, &MainWindow::startTask);
            connect(groupBox.get(), &QSBYItem::previewSource, this, &MainWindow::previewSource);
            connect(groupBox.get(), &QSBYItem::previewVCD, this, &MainWindow::previewVCD);

            fileBox->layout()->addWidget(groupBox.get());
            items.emplace(std::make_pair(newname, std::move(groupBox)));
        }
    }
    items[file->getFileName()]->refreshView();
}

void MainWindow::showTime()
{
    if (process == nullptr) {
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
    QAction *actionAbout = new QAction("&About", this);
    actionAbout->setStatusTip("Show the application's about box");
    connect(actionAbout, &QAction::triggered, this, &MainWindow::about);

    menuBar = new QMenuBar();
    QMenu *menu_File = new QMenu("&File", menuBar);
    QMenu *menu_Help = new QMenu("&Help", menuBar);

    menuBar->addAction(menu_File->menuAction());
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

    menu_Help->addAction(actionAbout);

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
        for (auto &item : files) {
            if (item->haveTasks()) {
                for (const auto &task : item->getTasks()) {
                    if (task->getStatusColor() != 1)
                        Q_EMIT startTask(item->getFileName() + "#" + task->getTaskName());
                }
            } else {
                if (item->getStatusColor() != 1)
                    Q_EMIT startTask(item->getFileName());
            }
        }
    });
    connect(actionStop, &QAction::triggered, [=]() {
        if (taskList.size() > 0) {
            QString name = taskList.front();
            taskList.clear();
            taskList.push_back(name); // Put back one to finish
            items[name]->stopProcess();
        }
    });
}

void MainWindow::save_sby(int index)
{
    QWidget *current = centralTabWidget->widget(index);
    if (current != nullptr) {
        if (QString(current->metaObject()->className()) == "QsciScintilla") {
            QsciScintilla *editor = (QsciScintilla *)current;
            if (editor->isModified()) {
                QString name = centralTabWidget->tabText(index);
                QFileInfo filepath(currentFolder, name);
                QFile file(filepath.absoluteFilePath());
                if (file.open(QIODevice::WriteOnly)) {
                    QByteArray contents = editor->text().toUtf8();
                    file.write(contents);
                    file.close();
                    editor->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
                    centralTabWidget->tabBar()->setTabTextColor(index, Qt::black);
                    if (filepath.completeSuffix() == "sby")
                        centralTabWidget->setTabIcon(index, QIcon(":/icons/resources/script_edit.png"));
                    else
                        centralTabWidget->setTabIcon(index, QIcon(":/icons/resources/page_code.png"));
                }
            }
        }
    }
}

void MainWindow::save_all()
{
    if (actionStop->isEnabled()) {
        QMessageBox::information(this, "SBY Gui", "The document can not be saved until tasks are done",
                                 QMessageBox::Ok);
    } else {
        for (int i = 0; i < centralTabWidget->count(); i++) {
            save_sby(i);
        }
    }
}

void MainWindow::save_file()
{
    if (actionStop->isEnabled()) {
        QMessageBox::information(this, "SBY Gui", "The document can not be saved until tasks are done",
                                 QMessageBox::Ok);
    } else {
        save_sby(centralTabWidget->currentIndex());
    }
}

void MainWindow::open_folder()
{
    QString folderName = QFileDialog::getExistingDirectory(
            this, tr("Open Directory"), QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!folderName.isEmpty()) {
        openLocation(QFileInfo(folderName));
    }
}

void MainWindow::close_editor() { closeTab(centralTabWidget->currentIndex(), false); }

void MainWindow::close_all()
{
    bool forceSave = false;
    for (int i = centralTabWidget->count() - 1; i >= 0; i--) {
        forceSave = closeTab(i, forceSave);
    }
}

bool MainWindow::closeTab(int index, bool forceSave)
{
    bool force = forceSave;
    QWidget *current = centralTabWidget->widget(index);
    if (current != nullptr) {
        if (QString(current->metaObject()->className()) == "QsciScintilla") {
            QsciScintilla *editor = (QsciScintilla *)current;
            if (editor->isModified()) {
                bool save = false;
                if (!force) {
                    int result = QMessageBox(QMessageBox::Information, "SBY Gui", "Do you wish to save file?",
                                             QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No)
                                         .exec();
                    if (result == QMessageBox::Yes || result == QMessageBox::YesToAll)
                        save = true;
                    if (result == QMessageBox::YesToAll)
                        force = true;
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
    if (QMessageBox::Yes == QMessageBox(QMessageBox::Information, "SBY Gui", "Are you sure you want to quit?",
                                        QMessageBox::Yes | QMessageBox::No)
                                    .exec()) {
        close_all();
        event->accept();
    }
}

void MainWindow::taskExecuted()
{
    taskList.pop_front();
    if (taskList.size() > 0) {
        QString name = taskList.front();
        items[name]->runSBYTask();
    } else {
        actionPlay->setEnabled(true);
        actionStop->setEnabled(false);
    }
}

void MainWindow::startTask(QString name)
{
    actionPlay->setEnabled(false);
    actionStop->setEnabled(true);
    if (std::find(taskList.begin(), taskList.end(), name) == taskList.end()) {
        taskList.push_back(name);
        if (taskList.size() == 1) {
            items[name]->runSBYTask();
        }
    }
}

QGroupBox *MainWindow::generateFileBox(SBYFile *file)
{
    std::unique_ptr<QSBYItem> fileBox = std::make_unique<QSBYItem>(file->getName(), file, nullptr, this);
    connect(fileBox.get(), &QSBYItem::appendLog, this, &MainWindow::appendLog);
    connect(fileBox.get(), &QSBYItem::editOpen, this, &MainWindow::editOpen);
    connect(fileBox.get(), &QSBYItem::previewOpen, this, &MainWindow::previewOpen);
    connect(fileBox.get(), &QSBYItem::previewLog, this, &MainWindow::previewLog);
    connect(fileBox.get(), &QSBYItem::taskExecuted, this, &MainWindow::taskExecuted);
    connect(fileBox.get(), &QSBYItem::startTask, this, &MainWindow::startTask);
    connect(fileBox.get(), &QSBYItem::previewSource, this, &MainWindow::previewSource);
    connect(fileBox.get(), &QSBYItem::previewVCD, this, &MainWindow::previewVCD);

    for (auto const &task : file->getTasks()) {
        std::unique_ptr<QSBYItem> groupBox =
                std::make_unique<QSBYItem>(task->getName(), task.get(), fileBox.get(), this);
        QString name = groupBox->getName();
        connect(groupBox.get(), &QSBYItem::appendLog, this, &MainWindow::appendLog);
        connect(groupBox.get(), &QSBYItem::editOpen, this, &MainWindow::editOpen);
        connect(groupBox.get(), &QSBYItem::previewOpen, this, &MainWindow::previewOpen);
        connect(groupBox.get(), &QSBYItem::previewLog, this, &MainWindow::previewLog);
        connect(groupBox.get(), &QSBYItem::taskExecuted, this, &MainWindow::taskExecuted);
        connect(groupBox.get(), &QSBYItem::startTask, this, &MainWindow::startTask);
        connect(groupBox.get(), &QSBYItem::previewSource, this, &MainWindow::previewSource);
        connect(groupBox.get(), &QSBYItem::previewVCD, this, &MainWindow::previewVCD);
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

QsciScintilla *MainWindow::openEditor(int lexer)
{
    QsciScintilla *editor = new QsciScintilla();
    QFont monospaceFont(MonospaceFont());
    monospaceFont.setPointSize(10);
    editor->setFont(monospaceFont);
    editor->setScrollWidth(200);
    editor->setScrollWidthTracking(1);
    editor->SendScintilla(QsciScintilla::SCI_SETUNDOCOLLECTION, false);
    editor->setProperty("fold", "1");
    editor->setProperty("fold.compact", "0");
    editor->setProperty("fold.comment", "1");
    editor->setProperty("fold.preprocessor", "1");
    // editor->setWrapMode(true);
    editor->setMarginWidth(0, 40); // Line number
    // editor->setMarginWidth(1, 20); // Foldemargin
    // editor->setMarginMarkerMask(1, QsciScintilla::SC_MASK_FOLDERS);
    // editor->setMarginType(1, QsciScintilla::MarginType::SymbolMargin);
    // editor->setMarginSensitivity(1, true);
    // editor->markerDefine(QsciScintilla::SC_MARKNUM_FOLDER, QsciScintilla::SC_MARK_BOXPLUS);
    // editor->markerDefine(QsciScintilla::SC_MARKNUM_FOLDEROPEN, QsciScintilla::SC_MARK_BOXMINUS);
    // editor->markerDefine(QsciScintilla::SC_MARKNUM_FOLDERSUB, QsciScintilla::SC_MARK_VLINE);
    // editor->markerDefine(QsciScintilla::SC_MARKNUM_FOLDERTAIL, QsciScintilla::SC_MARK_LCORNER);
    // editor->markerDefine(QsciScintilla::SC_MARKNUM_FOLDEREND, QsciScintilla::SC_MARK_BOXPLUSCONNECTED);
    // editor->markerDefine(QsciScintilla::SC_MARKNUM_FOLDEROPENMID, QsciScintilla::SC_MARK_BOXMINUSCONNECTED);
    // editor->markerDefine(QsciScintilla::SC_MARKNUM_FOLDERMIDTAIL, QsciScintilla::SC_MARK_TCORNER);
    // editor->setFolding(QsciScintilla::BoxedTreeFoldStyle);

    if (lexer == SCLEX_SBY) {
        QsciLexerSBY *sbyLexer = new QsciLexerSBY(this);
        sbyLexer->setDefaultFont(monospaceFont);
        for (int style = 0; style <= sbyLexer->styleBitsNeeded(); ++style) {
            sbyLexer->setFont(monospaceFont, style);
            sbyLexer->setColor(sbyLexer->defaultColor(style), style);
        }
        editor->setLexer(sbyLexer);
    } else if (lexer == SCLEX_VERILOG) {
        QsciLexerVerilog *verilogLexer = new QsciLexerVerilog(this);
        verilogLexer->setDefaultFont(monospaceFont);
        for (int style = 0; style <= verilogLexer->styleBitsNeeded(); ++style) {
            verilogLexer->setFont(monospaceFont, style);
            verilogLexer->setColor(verilogLexer->defaultColor(style), style);
        }
        editor->setLexer(verilogLexer);
    } else if (lexer == SCLEX_VHDL) {
        QsciLexerVHDL *vhdlLexer = new QsciLexerVHDL(this);
        vhdlLexer->setDefaultFont(monospaceFont);
        for (int style = 0; style <= vhdlLexer->styleBitsNeeded(); ++style) {
            vhdlLexer->setFont(monospaceFont, style);
            vhdlLexer->setColor(vhdlLexer->defaultColor(style), style);
        }
        editor->setLexer(vhdlLexer);
    }
    connect(editor, &QsciScintilla::marginClicked, this, &MainWindow::marginClicked);
    return editor;
}

void MainWindow::marginClicked(int margin, int position, Qt::KeyboardModifiers modifiers)
{
    QWidget *current = centralTabWidget->widget(centralTabWidget->currentIndex());
    if (current != nullptr) {
        if (QString(current->metaObject()->className()) == "QsciScintilla") {
            QsciScintilla *editor = (QsciScintilla *)current;

            if (margin == 1) {
                int line = 0;
                int index = 0;
                editor->lineIndexFromPosition(position, &line, &index);
                editor->foldLine(line);
            }
        }
    }
}

QsciScintilla *MainWindow::openEditorFile(QString fullpath)
{
    QsciScintilla *editor = openEditor(SCLEX_SBY);
    QFile file(fullpath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray contents = file.readAll();
        editor->setText(contents.constData());
        editor->SendScintilla(QsciScintilla::SCI_SETUNDOCOLLECTION, true);
        editor->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
        editor->setCursorPosition(0, 0);
    }
    return editor;
}

QsciScintilla *MainWindow::openEditorText(QString text, int lexer)
{
    QsciScintilla *editor = openEditor(lexer);
    editor->setText(text.toLatin1().data());
    editor->SendScintilla(QsciScintilla::SCI_SETUNDOCOLLECTION, true);
    editor->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
    editor->setCursorPosition(0, 0);
    return editor;
}

void MainWindow::previewOpen(QString content, QString fileName, QString taskName, bool reloadOnly)
{
    QString name = fileName + "#" + taskName;

    for (int i = 0; i < centralTabWidget->count(); i++) {
        if (centralTabWidget->tabText(i) == name) {
            centralTabWidget->setCurrentIndex(i);
            if (reloadOnly) {
                QWidget *current = centralTabWidget->widget(i);
                if (current != nullptr) {
                    if (QString(current->metaObject()->className()) == "QsciScintilla") {
                        QsciScintilla *editor = (QsciScintilla *)current;
                        editor->setReadOnly(false);
                        editor->setText(content.toLatin1().data());
                        editor->SendScintilla(QsciScintilla::SCI_SETUNDOCOLLECTION, true);
                        editor->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
                        editor->setCursorPosition(0, 0);
                        editor->setReadOnly(true);
                    }
                }
            }
            return;
        }
    }
    if (reloadOnly)
        return;

    QsciScintilla *editor = openEditorText(content, SCLEX_SBY);
    editor->setReadOnly(true);

    centralTabWidget->addTab(editor, QIcon(":/icons/resources/script.png"), name);
    centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);
}

void MainWindow::previewLog(QString content, QString fileName, QString taskName, bool reloadOnly)
{
    QString name = fileName;
    if (!taskName.isEmpty())
        name += "#" + taskName;
    name += ".log";

    for (int i = 0; i < centralTabWidget->count(); i++) {
        if (centralTabWidget->tabText(i) == name) {
            centralTabWidget->setCurrentIndex(i);
            if (reloadOnly) {
                QWidget *current = centralTabWidget->widget(i);
                if (current != nullptr) {
                    if (QString(current->metaObject()->className()) == "QsciScintilla") {
                        QsciScintilla *editor = (QsciScintilla *)current;
                        editor->setReadOnly(false);
                        editor->setText(content.toLatin1().data());
                        editor->SendScintilla(QsciScintilla::SCI_SETUNDOCOLLECTION, true);
                        editor->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
                        editor->setCursorPosition(0, 0);
                        editor->setReadOnly(true);
                    }
                }
            }
            return;
        }
    }
    if (reloadOnly)
        return;

    QsciScintilla *editor = openEditorText(content, 0);
    editor->setReadOnly(true);

    centralTabWidget->addTab(editor, QIcon(":/icons/resources/book.png"), name);
    centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);
}

void MainWindow::previewSource(QString fileName, bool reloadOnly)
{
    for (int i = 0; i < centralTabWidget->count(); i++) {
        if (centralTabWidget->tabText(i) == fileName) {
            centralTabWidget->setCurrentIndex(i);
            return;
        }
    }
    if (reloadOnly)
        return;

    QFileInfo fullpath(currentFolder, fileName);
    QFile file(fullpath.canonicalFilePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray contents = file.readAll();
        int lexer = 0;
        if (fullpath.completeSuffix() == "v")
            lexer = SCLEX_VERILOG;
        if (fullpath.completeSuffix() == "sv")
            lexer = SCLEX_VERILOG;
        if (fullpath.completeSuffix() == "vh")
            lexer = SCLEX_VERILOG;
        if (fullpath.completeSuffix() == "svh")
            lexer = SCLEX_VERILOG;
        if (fullpath.completeSuffix() == "vhd")
            lexer = SCLEX_VHDL;
        if (fullpath.completeSuffix() == "vhdl")
            lexer = SCLEX_VHDL;
        QsciScintilla *editor = openEditorText(contents.constData(), lexer);

        connect(editor, &QsciScintilla::modificationChanged, [=](bool modified) {
            for (int i = 0; i < centralTabWidget->count(); i++) {
                if (centralTabWidget->tabText(i) == fileName) {
                    if (modified) {
                        centralTabWidget->tabBar()->setTabTextColor(i, Qt::red);
                        centralTabWidget->setTabIcon(i, QIcon(":/icons/resources/disk.png"));
                    } else {
                        centralTabWidget->tabBar()->setTabTextColor(i, Qt::black);
                        centralTabWidget->setTabIcon(i, QIcon(":/icons/resources/page_code.png"));
                    }
                    return;
                }
            }
        });

        centralTabWidget->addTab(editor, QIcon(":/icons/resources/page_code.png"), fileName);
        centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);
    }
}

void MainWindow::previewVCD(QString fileName) { QProcess::startDetached("gtkwave", QStringList() << fileName); }

void MainWindow::editOpen(QString path, QString fileName, bool reloadOnly)
{
    QString name = fileName;
    for (int i = 0; i < centralTabWidget->count(); i++) {
        if (centralTabWidget->tabText(i) == name) {
            centralTabWidget->setCurrentIndex(i);
            if (reloadOnly) {
                QWidget *current = centralTabWidget->widget(i);
                if (current != nullptr) {
                    if (QString(current->metaObject()->className()) == "QsciScintilla") {
                        QsciScintilla *editor = (QsciScintilla *)current;
                        if (editor->isModified()) {
                            int result = QMessageBox(QMessageBox::Information, "SBY Gui", "Do you wish to reload file?",
                                                     QMessageBox::Yes | QMessageBox::No)
                                                 .exec();
                            if (result == QMessageBox::No)
                                return;
                        }
                        QFile file(path);
                        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            QByteArray contents = file.readAll();
                            editor->SendScintilla(QsciScintilla::SCI_SETUNDOCOLLECTION, false);
                            editor->setText(contents.constData());
                            editor->SendScintilla(QsciScintilla::SCI_SETUNDOCOLLECTION, true);
                            editor->SendScintilla(QsciScintilla::SCI_SETSAVEPOINT);
                            editor->setCursorPosition(0, 0);
                        }
                    }
                }
            }
            return;
        }
    }
    if (reloadOnly)
        return;

    QsciScintilla *editor = openEditorFile(path);
    connect(editor, &QsciScintilla::modificationChanged, [=](bool modified) {
        for (int i = 0; i < centralTabWidget->count(); i++) {
            if (centralTabWidget->tabText(i) == name) {
                if (modified) {
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

    centralTabWidget->addTab(editor, QIcon(":/icons/resources/script_edit.png"), name);
    centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);
}

void MainWindow::appendLog(QString logline)
{
    log->moveCursor(QTextCursor::End);
    log->insertPlainText(logline);
    log->moveCursor(QTextCursor::End);
}

void MainWindow::refreshView()
{
    openLocation(refreshLocation);
    appendLog("All processes killed\n");
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("SBY Gui"),
                       tr("The <b>SBY GUI</b> is part of "
                          "<br/><b>YosysHQ</b> solution for formal verification."));
}
