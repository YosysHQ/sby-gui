#include "qsbyitem.h"
#include <QHBoxLayout>
#include <QToolBar>
#include <QGraphicsColorizeEffect>

QSBYItem::QSBYItem(const QString & title, SBYItem *item, QWidget *parent) : QGroupBox(title, parent), item(item), process(nullptr)
{
    if (item->isTop()) {
        QString style = "QGroupBox { border: 3px solid gray; border-radius: 3px; margin-top: 0.5em; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }";
        setStyleSheet(style);     
        setMinimumWidth(370);
        setMaximumWidth(370);
    } else {
        QString style = "QGroupBox { border: 1px solid gray; border-radius: 3px; margin-top: 0.5em; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }";    
        setStyleSheet(style);     
    }

    vboxFile = new QVBoxLayout(this);
    QWidget *dummyFile = new QWidget();
    QHBoxLayout *hboxFile = new QHBoxLayout(dummyFile);

    progressBar = new QProgressBar(this);
    refreshView();

    QToolBar *toolBarFile = new QToolBar(this);    
    
    actionPlay = new QAction("Play", this);
    actionPlay->setIcon(QIcon(":/icons/resources/media-playback-start.png")); 
    toolBarFile->addAction(actionPlay);
    actionStop = new QAction("Stop", this);
    actionStop->setIcon(QIcon(":/icons/resources/media-playback-stop.png"));    
    actionStop->setEnabled(false);
    toolBarFile->addAction(actionStop);
    actionEdit = new QAction("Edit", this);
    actionEdit->setIcon(QIcon(":/icons/resources/text-x-generic.png"));    
    toolBarFile->addAction(actionEdit);
    
    connect(actionPlay, &QAction::triggered, [=]() { runSBYTask(); });   
    connect(actionStop, &QAction::triggered, [=]() { process->terminate(); });
    if (item->isTop()) {    
        connect(actionEdit, &QAction::triggered, [=]() { Q_EMIT editOpen(item->getFullPath(), item->getFileName()); });  
    } else {
        connect(actionEdit, &QAction::triggered, [=]() { Q_EMIT previewOpen(item->getContents(), item->getFileName(), item->getName()); });
    }

    hboxFile->addWidget(progressBar);
    hboxFile->addWidget(toolBarFile);

    vboxFile->addWidget(dummyFile);
}

void QSBYItem::printOutput()
{
    QString data = QString(process->readAllStandardOutput());
    Q_EMIT appendLog(data);
}

void QSBYItem::refreshView()
{
    QGraphicsColorizeEffect *effectFile = new QGraphicsColorizeEffect;
    switch(item->getStatus()) {
        case 1 : effectFile->setColor(QColor(0, 255, 0, 127)); break;
        case 2 : effectFile->setColor(QColor(255, 0, 0, 127)); break;
        default : effectFile->setColor(QColor(255, 255, 0, 127)); break;
    }
    progressBar->setGraphicsEffect(effectFile); 
    progressBar->setValue(item->getPercentage());
}
void QSBYItem::runSBYTask()
{
    if (process!=nullptr) {
        Q_EMIT appendLog(QString("---TASK IN PROGRESS---\n"));
        return;
    }
    //taskTimer->restart();
    //log->clear();
    QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
    effect->setColor(QColor(0, 0, 255, 127));
    progressBar->setGraphicsEffect(effect);    
    progressBar->setValue(50);    

    process = new QProcess;
    QStringList args;
    args << "-f";
    args << item->getFileName().c_str();
    args << item->getTaskName().c_str();
    process->setProgram("sby");
    process->setArguments(args);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("YOSYS_NOVERIFIC","1");
    env.insert("PYTHONUNBUFFERED","1");
    process->setProcessEnvironment(env);
    process->setWorkingDirectory(item->getWorkFolder().c_str());
    process->setProcessChannelMode(QProcess::MergedChannels);  
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(printOutput()));
    connect(process, &QProcess::started, [=]() { 
        actionPlay->setEnabled(false); 
        actionStop->setEnabled(true); 
    });
    connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        actionPlay->setEnabled(true); 
        actionStop->setEnabled(false); 
        item->update();
        refreshView(); 
        if (exitCode!=0) Q_EMIT appendLog(QString("---TASK STOPPED---\n")); 
        delete process; 
        process = nullptr; 
    });
    process->start();
}