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
    QString getName();
    void stopProcess();
    QSBYItem* getParent() { return top; }
  protected Q_SLOTS:
    void printOutput();
  Q_SIGNALS:
    void appendLog(QString content);
    void taskExecuted();
    void startTask(QString name);
    void editOpen(QString path, QString fileName, bool reloadOnly);
    void previewOpen(QString content, QString fileName, QString taskName, bool reloadOnly);
    void previewLog(QString content, QString fileName, QString taskName, bool reloadOnly);
    void previewSource(QString fileName, bool reloadOnly);
    void previewVCD(QString fileName);
  protected:    
    QProgressBar *progressBar;
    QAction *actionStatus;
    QAction *actionPlay;
    QAction *actionStop;
    QAction *actionEdit;
    QAction *actionLog;
    QAction *actionFiles;
    QAction *actionWave;

    SBYItem *item;
    QProcess *process;
    bool shutdown;
    QProcess::ProcessState state;
    QLabel *label;
    QSBYItem *top;
};

#endif // QSBYITEM_H
