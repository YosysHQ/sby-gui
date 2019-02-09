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

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>

Q_DECLARE_METATYPE(std::string)

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QString path, QWidget *parent = 0);
    virtual ~MainWindow();

  protected:
    void createMenusAndBars();
    QGroupBox *generateFileBox(boost::filesystem::path path);

  protected Q_SLOTS:
    void closeTab(int index);

    void new_proj();
    void open_proj();
    bool save_proj();
    void new_doc();
    void execute_doc();
    void run_doc();
    void open_doc();
    void save_doc();
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

    QString currentFolder;
};

#endif // MAINWINDOW_H
