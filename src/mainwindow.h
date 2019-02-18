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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGroupBox>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QGridLayout>
#include <QProcess>
#include <QPlainTextEdit>
#include <QProgressBar>

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>

Q_DECLARE_METATYPE(std::string)

class ScintillaEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QString path, QWidget *parent = 0);
    virtual ~MainWindow();

  protected:
    void createMenusAndBars();
    QGroupBox *generateFileBox(boost::filesystem::path path);

    void openLocation(QString path);
    void removeLayoutItems(QLayout* layout);
    void previewOpen(boost::filesystem::path path, std::string task);
    void editOpen(boost::filesystem::path path);
    ScintillaEdit *openEditor();
    ScintillaEdit *openEditorFile(std::string fullpath);
    ScintillaEdit *openEditorText(std::string text);
    void runSBYFile(boost::filesystem::path path, QAction* playAction, QAction* stopAction, QProgressBar *progressBar);
    void runSBYTask(boost::filesystem::path path, std::string task, QAction* playAction, QAction* stopAction, QProgressBar *progressBar);
    void refreshView();
  protected Q_SLOTS:
    void closeTab(int index);

    void open_sby();
    void open_folder();
    void printOutput();
  Q_SIGNALS:
    void updateTreeView();
    void executePython(QString content);
    void runPythonScript(QString content);

  protected:
    QTabWidget *tabWidget;
    QTabWidget *centralTabWidget;

    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    QGridLayout *grid;

    QString currentFolder;

    QAction *actionNew;
    QAction *actionOpen;
    QAction *actionOpenFolder;
    QAction *actionSave;
    QAction *actionSaveAs;
    QAction *actionRefresh;
    QAction *actionExit;

    QAction *actionCut;
    QAction *actionCopy;
    QAction *actionPaste;
    QAction *actionUndo;
    QAction *actionRedo;
    QProcess *process;
    QPlainTextEdit *log;

    QString refreshLocation;
};

#endif // MAINWINDOW_H
