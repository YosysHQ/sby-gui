#ifndef QSBYITEM_H
#define QSBYITEM_H

#include <QGroupBox>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QAction>
#include <QProcess>
#include <QLabel>
#include "sbyitem.h"

class QSBYItem : public QGroupBox
{
    Q_OBJECT

  public:
    QSBYItem(const QString & title, SBYItem *item, QSBYItem* top, QWidget *parent = 0);
    virtual ~QSBYItem();
    void runSBYTask();
    void refreshView();
    std::string getName();
    void stopProcess();
    QSBYItem* getParent() { return top; }
  protected Q_SLOTS:
    void printOutput();
  Q_SIGNALS:
    void appendLog(QString content);
    void taskExecuted();
    void startTask(std::string name);
    void editOpen(std::string path, std::string fileName);
    void previewOpen(std::string content, std::string fileName, std::string taskName);
    void previewLog(std::string content, std::string fileName, std::string taskName);
  protected:    
    QProgressBar *progressBar;
    QAction *actionStatus;
    QAction *actionPlay;
    QAction *actionStop;
    QAction *actionEdit;
    QAction *actionLog;
    QAction *actionFiles;

    SBYItem *item;
    QProcess *process;
    bool shutdown;
    QProcess::ProcessState state;
    QLabel *label;
    QSBYItem *top;
};

#endif // QSBYITEM_H
