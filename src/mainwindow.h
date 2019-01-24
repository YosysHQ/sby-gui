#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QGroupBox>

Q_DECLARE_METATYPE(std::string)

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

  protected:
    void createMenusAndBars();
    QGroupBox *createFirstExclusiveGroup();

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
};

#endif // MAINWINDOW_H
