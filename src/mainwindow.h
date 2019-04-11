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
#include <QLabel>
#include <QTime>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QDir>
#include <map>
#include <deque>
#include "qsbyitem.h"

class ScintillaEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QString path, QWidget *parent = 0);
    virtual ~MainWindow();

  protected:
    void createMenusAndBars();
    QGroupBox *generateFileBox(SBYFile *file);

    void openLocation(QFileInfo path);
    void removeLayoutItems(QLayout* layout);
    void editOpen(QString path, QString fileName);
    void previewOpen(QString content, QString fileName, QString taskName, bool reloadOnly);
    void previewLog(QString content, QString fileName, QString taskName, bool reloadOnly);
    void previewSource(QString fileName, bool reloadOnly);
    void previewVCD(QString fileName);
    ScintillaEdit *openEditor(int lexer);
    ScintillaEdit *openEditorFile(QString fullpath);
    ScintillaEdit *openEditorText(QString text, int lexer);
    void refreshView();
    void appendLog(QString logline);
    void showTime();
    virtual void closeEvent(QCloseEvent * event);
    void save_sby(int index);
    bool closeTab(int index, bool forceSave);
    QStringList getFileList(QDir path);
  protected Q_SLOTS:
    void taskExecuted();
    void startTask(QString name);

    void open_folder();
    void save_file();
    void save_all();
    void close_editor();
    void close_all();

    void directoryChanged(const QString & path);
    void fileChanged(const QString & path);
    void marginClicked(int position, int modifiers, int margin);
  protected:
    QTabWidget *tabWidget;
    QTabWidget *centralTabWidget;

    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    QGridLayout *grid;

    QDir currentFolder;

    QAction *actionNew;
    QAction *actionOpenFolder;
    QAction *actionSave;
    QAction *actionSaveAll;
    QAction *actionSaveAs;
    QAction *actionRefresh;
    QAction *actionClose;
    QAction *actionCloseAll;
    QAction *actionExit;

    QAction *actionCut;
    QAction *actionCopy;
    QAction *actionPaste;
    QAction *actionUndo;
    QAction *actionRedo;
    QAction *actionPlay;
    QAction *actionStop;

    QProcess *process;
    QPlainTextEdit *log;
    QLabel *timeDisplay;
    QFileInfo refreshLocation;

    QTime *taskTimer;

    QFileSystemWatcher *fileWatcher;

    QStringList currentFileList;
    std::vector<std::unique_ptr<SBYFile>> files;
    std::map<QString, SBYFile*> fileMap;
    std::map<QString, std::unique_ptr<QSBYItem>> items;
    std::deque<QString> taskList;
};

#endif // MAINWINDOW_H
