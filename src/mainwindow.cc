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

MainWindow::MainWindow(QString path, QWidget *parent) : QMainWindow(parent)
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

    QVBoxLayout *gridMain = new QVBoxLayout;
    QGridLayout *grid = new QGridLayout;
    
    int cnt = 0;
    for(auto file : files) {
        QFrame *line = new QFrame(this);
        line->setFrameShape(QFrame::HLine); // Horizontal line
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(2);

        grid->addWidget(generateFileBox(file), cnt++, 0);
        grid->addWidget(line, cnt++, 0);

    }
    QWidget *container = new QWidget();
    container->setLayout(grid);

    QScrollArea *scrollArea = new QScrollArea();
    // scrollArea->setBackgroundRole(QPalette::Mid);
    scrollArea->setWidget(container);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setLayout(gridMain);
    scrollArea->setMinimumWidth(400);
    scrollArea->setMaximumWidth(400);

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

    for(auto f : files) {
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
    }
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

    menu_File->addAction(new QAction("New", this));
    menu_File->addAction(new QAction("Open...", this));
    menu_File->addAction(new QAction("Save", this));
    menu_File->addAction(new QAction("Save As...", this));
    menu_File->addAction(new QAction("Save a Copy As...", this));
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
    menu_File->addAction(new QAction("Exit", this));

    menu_Edit->addAction(new QAction("Undo", this));
    menu_Edit->addAction(new QAction("Redo", this));
    menu_Edit->addSeparator();
    menu_Edit->addAction(new QAction("Cut", this));
    menu_Edit->addAction(new QAction("Copy", this));
    menu_Edit->addAction(new QAction("Paste", this));
    menu_Edit->addAction(new QAction("Delete", this));
    menu_Edit->addAction(new QAction("Select All", this));

    menu_Search->addAction(new QAction("Find...", this));
    menu_Search->addAction(new QAction("Find Next", this));
    menu_Search->addAction(new QAction("Find Previous", this));
    menu_Search->addAction(new QAction("Replace...", this));
    menu_Search->addAction(new QAction("Go to...", this));

    menu_Help->addAction(new QAction("About", this));
}

void MainWindow::new_doc()
{
    ScintillaEdit *editor = new ScintillaEdit();
    editor->styleClearAll();
    editor->setMarginWidthN(0, 35);
    editor->setScrollWidth(200);
    editor->setScrollWidthTracking(1);
    centralTabWidget->addTab(editor, "New");
    centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);
}

void MainWindow::new_proj() {}

void MainWindow::open_proj() {}

bool MainWindow::save_proj() { return false; }

void MainWindow::execute_doc() {}

void MainWindow::run_doc() {}

void MainWindow::open_doc()
{
    QString fileName =
            QFileDialog::getOpenFileName(this, QString("Open document"), QString(),
                                         QString("Python files (*.py);;Verilog files (*.v);;Text files (*.txt)"));
    if (!fileName.isEmpty()) {
        ScintillaEdit *editor = new ScintillaEdit();
        editor->styleClearAll();
        editor->setMarginWidthN(0, 35);
        editor->setScrollWidth(200);
        editor->setScrollWidthTracking(1);

        QFileInfo finfo(fileName);
        if (finfo.suffix() == "py")
            editor->setLexer(SCLEX_PYTHON);

        centralTabWidget->addTab(editor, finfo.fileName());
        centralTabWidget->setCurrentIndex(centralTabWidget->count() - 1);

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray contents = file.readAll();
            editor->get_doc()->insert_string(0, contents);
        }
    }
}

void MainWindow::save_doc() {}

QGroupBox *MainWindow::generateFileBox(boost::filesystem::path path)
{
    QGroupBox *fileBox = new QGroupBox(path.filename().c_str());
    fileBox->setMinimumWidth(370);
    fileBox->setMaximumWidth(370);

    SBYParser parser;
    std::fstream fs;
    fs.open(path.string().c_str(), std::fstream::in);
    parser.parse(fs);
    fs.close();
    
    QVBoxLayout *vboxFile = new QVBoxLayout;
    for (auto task : parser.get_tasks())
    {
        QGroupBox *groupBox = new QGroupBox(task.c_str());
        vboxFile->addWidget(groupBox);
    }
    fileBox->setLayout(vboxFile);
    return fileBox;
}