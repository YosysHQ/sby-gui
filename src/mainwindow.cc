#include "mainwindow.h"
#include <QAction>
#include <QFileDialog>
#include <QGridLayout>
#include <QIcon>
#include <QSplitter>
#include <QTabBar>
#include "SciLexer.h"
#include "ScintillaEdit.h"

static void initBasenameResource() { Q_INIT_RESOURCE(base); }

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
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
    splitter_h->addWidget(splitter_v);

    gridLayout->addWidget(splitter_h, 0, 0, 1, 1);

    setCentralWidget(centralWidget);

    tabWidget = new QTabWidget();

    centralTabWidget = new QTabWidget();
    centralTabWidget->setTabsClosable(true);
    connect(centralTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

    splitter_v->addWidget(centralTabWidget);
    splitter_v->addWidget(tabWidget);
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
    editor->setLexer(SCLEX_PYTHON);
    editor->styleClearAll();
    editor->setMarginWidthN(0, 35);
    editor->setScrollWidth(200);
    editor->setScrollWidthTracking(1);

    editor->styleSetFore(SCE_P_DEFAULT, 0x000000);
    editor->styleSetFore(SCE_P_COMMENTLINE, 0x008000);
    editor->styleSetFore(SCE_P_NUMBER, 0xFF0000);
    editor->styleSetFore(SCE_P_STRING, 0x808080);
    editor->styleSetFore(SCE_P_CHARACTER, 0x808080);
    editor->styleSetFore(SCE_P_WORD, 0x0000FF);
    editor->styleSetFore(SCE_P_TRIPLE, 0xFF8000);
    editor->styleSetFore(SCE_P_TRIPLEDOUBLE, 0xFF8000);
    editor->styleSetFore(SCE_P_CLASSNAME, 0x000000);
    editor->styleSetFore(SCE_P_DEFNAME, 0xFF00FF);
    editor->styleSetFore(SCE_P_OPERATOR, 0x000080);
    editor->styleSetFore(SCE_P_IDENTIFIER, 0x000000);
    editor->styleSetFore(SCE_P_COMMENTBLOCK, 0x008000);
    editor->styleSetFore(SCE_P_DECORATOR, 0xFF8000);

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

        editor->styleSetFore(SCE_P_DEFAULT, 0x000000);
        editor->styleSetFore(SCE_P_COMMENTLINE, 0x008000);
        editor->styleSetFore(SCE_P_NUMBER, 0xFF0000);
        editor->styleSetFore(SCE_P_STRING, 0x808080);
        editor->styleSetFore(SCE_P_CHARACTER, 0x808080);
        editor->styleSetFore(SCE_P_WORD, 0x0000FF);
        editor->styleSetFore(SCE_P_TRIPLE, 0xFF8000);
        editor->styleSetFore(SCE_P_TRIPLEDOUBLE, 0xFF8000);
        editor->styleSetFore(SCE_P_CLASSNAME, 0x000000);
        editor->styleSetFore(SCE_P_DEFNAME, 0xFF00FF);
        editor->styleSetFore(SCE_P_OPERATOR, 0x000080);
        editor->styleSetFore(SCE_P_IDENTIFIER, 0x000000);
        editor->styleSetFore(SCE_P_COMMENTBLOCK, 0x008000);
        editor->styleSetFore(SCE_P_DECORATOR, 0xFF8000);

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
