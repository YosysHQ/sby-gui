#include "qsbyitem.h"
#include <QHBoxLayout>
#include <QToolBar>
#include <QGraphicsColorizeEffect>
#include <QInputDialog>

QSBYItem::QSBYItem(const QString & title, SBYItem *item, QSBYItem *top, QWidget *parent) : QGroupBox(title, parent), item(item), process(nullptr), shutdown(false), top(top)
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

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setSpacing(0); 
    QWidget *dummyItem = new QWidget(this);
    QHBoxLayout *hbox = new QHBoxLayout(dummyItem);
    hbox->setSpacing(0); 
    hbox->setMargin(0);
    QWidget *dummyItem2 = new QWidget(this);
    QHBoxLayout *hbox2 = new QHBoxLayout(dummyItem2);
    hbox2->setSpacing(0); 
    hbox2->setMargin(0);

    progressBar = new QProgressBar(this);

    QToolBar *toolBar = new QToolBar(this);    
    QToolBar *toolBar2 = new QToolBar(this);   
    bool statusVisible = false;

    actionPlay = new QAction("Play", this);
    actionPlay->setIcon(QIcon(":/icons/resources/media-playback-start.png")); 
    toolBar->addAction(actionPlay);
    actionStop = new QAction("Stop", this);
    actionStop->setIcon(QIcon(":/icons/resources/media-playback-stop.png"));    
    actionStop->setEnabled(false);
    toolBar->addAction(actionStop);
    actionLog = nullptr;
    actionFiles = nullptr;
    actionWave = nullptr;
    if (item->isTop()) {
        actionEdit = new QAction("Edit", this);
        actionEdit->setIcon(QIcon(":/icons/resources/script_edit.png"));    
        SBYFile* file = static_cast<SBYFile*>(item);
        if (!file->haveTasks())  {
           actionLog = new QAction("Log", this);
           actionLog->setIcon(QIcon(":/icons/resources/book.png"));
           actionLog->setEnabled(false);
           actionFiles = new QAction("Files", this);
           actionFiles->setIcon(QIcon(":/icons/resources/page_code.png"));
           actionFiles->setEnabled(false);
           actionWave = new QAction("Wave", this);
           actionWave->setIcon(QIcon(":/icons/resources/gtkwave.png"));
           actionWave->setEnabled(false);
           toolBar2->addAction(actionWave);
           toolBar2->addAction(actionFiles);
           toolBar2->addAction(actionLog);
           toolBar2->addAction(actionEdit);
           statusVisible = true;
        } else {
           toolBar->addAction(actionEdit);
        }
    } else {
        actionEdit = new QAction("View", this);
        actionEdit->setIcon(QIcon(":/icons/resources/script.png"));
        actionLog = new QAction("Log", this);
        actionLog->setIcon(QIcon(":/icons/resources/book.png"));
        actionLog->setEnabled(false);        
        actionFiles = new QAction("Files", this);
        actionFiles->setIcon(QIcon(":/icons/resources/page_code.png"));
        actionFiles->setEnabled(false);
        actionWave = new QAction("Wave", this);
        actionWave->setIcon(QIcon(":/icons/resources/gtkwave.png"));
        actionWave->setEnabled(false);
        toolBar2->addAction(actionWave);
        toolBar2->addAction(actionFiles);
        toolBar2->addAction(actionLog);
        toolBar2->addAction(actionEdit);
        statusVisible = true;
    }
    
    label = new QLabel(this);
    label->setFrameStyle(QFrame::NoFrame);
    if (statusVisible)
        label->setText("Last run: ??? sec");
    else 
        label->setVisible(false);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    connect(actionPlay, &QAction::triggered, [=]() { 
        if (item->isTop()) {
         SBYFile* file = static_cast<SBYFile*>(item);
         if (file->haveTasks())  {
            for(const auto & task : file->getTasks())
                Q_EMIT startTask(item->getFileName() + "#" + task->getTaskName()); 
         } else {
            Q_EMIT startTask(getName()); 
         }
        } else {
            Q_EMIT startTask(getName()); 
        }
    });   
    connect(actionStop, &QAction::triggered, [=]() { process->terminate(); });
    if (item->isTop()) {    
        connect(actionEdit, &QAction::triggered, [=]() { Q_EMIT editOpen(item->getFullPath(), item->getFileName()); });  
    } else {
        connect(actionEdit, &QAction::triggered, [=]() { Q_EMIT previewOpen(item->getContents(), item->getFileName(), item->getName()); });
    }

    actionStatus = nullptr;
    if (statusVisible) {
        QToolBar *statusToolBar = new QToolBar(this);    
        actionStatus = new QAction("Unknown", this);
        actionStatus->setIcon(QIcon(":/icons/resources/question.png")); 
        statusToolBar->addAction(actionStatus);

        hbox->addWidget(statusToolBar);
    }
    hbox->addWidget(progressBar);
    hbox->addWidget(toolBar);
    hbox2->addWidget(label);
    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
    hbox2->addItem(spacer);
    hbox2->addWidget(toolBar2);

    vbox->addWidget(dummyItem);
    vbox->addWidget(dummyItem2);

    refreshView();

}

QSBYItem::~QSBYItem()
{
    if (actionLog) disconnect(actionLog);
    if (actionFiles) disconnect(actionFiles);
    if (actionWave) disconnect(actionWave);
    if (process) {
        shutdown = true;
        process->terminate();        
        process->waitForFinished();
        process->close();
    }
}
void QSBYItem::printOutput()
{
    QString data = QString(process->readAllStandardOutput());
    Q_EMIT appendLog(data);
}

void QSBYItem::refreshView()
{
    QGraphicsColorizeEffect *effectFile = new QGraphicsColorizeEffect;
    switch(item->getStatusColor()) {
        case 1 : effectFile->setColor(QColor(0, 255, 0, 127)); break;
        case 2 : effectFile->setColor(QColor(255, 0, 0, 127)); break;
        default : effectFile->setColor(QColor(255, 255, 0, 127)); break;
    }
    progressBar->setGraphicsEffect(effectFile); 
    progressBar->setValue(item->getPercentage());
    if (actionLog) {
        disconnect(actionLog);
        if (item->getPreviousLog()) {
            actionLog->setEnabled(true);
            connect(actionLog, &QAction::triggered, [=]() { Q_EMIT previewLog(item->getPreviousLog().get(), item->getFileName(), item->getTaskName()); });
        } else {
            actionLog->setEnabled(false);            
        }
    }
    if (actionWave) {
        disconnect(actionWave);
        if (item->getVCDFiles().size()>0) {            
            actionWave->setEnabled(true);
            connect(actionWave, &QAction::triggered, [=]() { 
                auto files = item->getVCDFiles();
                if (files.size()>1) {
                    QInputDialog qDialog;

                    QStringList items;
                    for (auto file : files)                    
                        items << QString(file.filename().string().c_str());

                    qDialog.setOptions(QInputDialog::UseListViewForComboBoxItems);
                    qDialog.setComboBoxItems(items);
                    qDialog.setWindowTitle("Choose VCD to open");
                    qDialog.setLabelText("Select file :");

                    connect(&qDialog, &QInputDialog::textValueSelected, 
                            [=](const QString &f) { 
                                for (auto file : files)                    
                                    if (file.filename().string()==f.toStdString())
                                        Q_EMIT previewVCD(file.string()); 
                    });
                    qDialog.exec();
                } else if (files.size()==1) {
                    Q_EMIT previewVCD(files[0].string());
                }
            });    
        } else {
            actionWave->setEnabled(false);            
        }
    }
    if (actionFiles) {
        disconnect(actionFiles);
        if (item->getFiles().size()>0) {            
            actionFiles->setEnabled(true);
            connect(actionFiles, &QAction::triggered, [=]() { 
                auto files = item->getFiles();
                if (files.size()>1) {
                    QInputDialog qDialog;

                    QStringList items;
                    for (auto file : files)                    
                        items << QString(file.c_str());

                    qDialog.setOptions(QInputDialog::UseListViewForComboBoxItems);
                    qDialog.setComboBoxItems(items);
                    qDialog.setWindowTitle("Choose file to open");
                    qDialog.setLabelText("Select file :");

                    connect(&qDialog, &QInputDialog::textValueSelected, 
                            [=](const QString &file) { Q_EMIT previewSource(file.toStdString()); });
                    qDialog.exec();
                } else if (files.size()==1) {
                    Q_EMIT previewSource(files[0]);
                }
            });    
        } else {
            actionFiles->setEnabled(false);            
        }
    }
    if (actionStatus!=nullptr)
    {
        std::string status = item->getStatus();
        if (status=="PASS") {
            actionStatus->setIcon(QIcon(":/icons/resources/accept.png")); 
            actionStatus->setText("Pass");
        } else if (status=="FAIL") {
            actionStatus->setIcon(QIcon(":/icons/resources/cancel.png")); 
            actionStatus->setText("Fail");
        } else if (status=="ERROR") {
            actionStatus->setIcon(QIcon(":/icons/resources/delete.png")); 
            actionStatus->setText("Error");
        } else if (status=="TIMEOUT") {
            actionStatus->setIcon(QIcon(":/icons/resources/time.png")); 
            actionStatus->setText("Timeout");
        } else {
            actionStatus->setIcon(QIcon(":/icons/resources/question.png")); 
            actionStatus->setText("Unknown");
        }
    }
    std::string time = "Last run: ";
    if (item->getTimeSpent())
        time += std::to_string(item->getTimeSpent().get()) + " sec";
    else
        time += "??? sec";    
    label->setText(time.c_str());
}
void QSBYItem::runSBYTask()
{
    QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
    effect->setColor(QColor(0, 0, 255, 127));
    progressBar->setGraphicsEffect(effect);    
    progressBar->setValue(50);    

    process = new QProcess;
    QStringList args;
    args << "-f";
    args << item->getFileName().c_str();
    if (!item->isTop()) { 
        args << item->getTaskName().c_str();
    }
    process->setProgram("sby");
    process->setArguments(args);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    //env.insert("YOSYS_NOVERIFIC","1");
    env.insert("PYTHONUNBUFFERED","1");
    process->setProcessEnvironment(env);
    process->setWorkingDirectory(item->getWorkFolder().c_str());
    process->setProcessChannelMode(QProcess::MergedChannels);  
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(printOutput()));
    connect(process, &QProcess::stateChanged, [=](QProcess::ProcessState newState) { 
        if (newState == QProcess::NotRunning && state == QProcess::Starting) {
            Q_EMIT appendLog(QString("Unable to start SBY\n")); 
            actionPlay->setEnabled(true); 
            actionStop->setEnabled(false); 
            item->update();
            if (top)
                top->refreshView();
            refreshView(); 
            Q_EMIT taskExecuted();
        }
        state = newState;
    });

    connect(process, &QProcess::started, [=]() { 
        actionPlay->setEnabled(false); 
        actionStop->setEnabled(true); 
    });
    connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {
        if (shutdown) return;
        actionPlay->setEnabled(true); 
        actionStop->setEnabled(false); 
        item->update();
        if (top)
            top->refreshView();
        refreshView(); 
        if (exitCode!=0) Q_EMIT appendLog(QString("---TASK STOPPED---\n")); 
        delete process; 
        process = nullptr; 
        Q_EMIT taskExecuted();
    });
    process->start();
}

void QSBYItem::stopProcess()
{
    if (process)
        process->terminate();
}

std::string QSBYItem::getName()
{
    if (item->isTop())
        return item->getFileName();
    else 
        return item->getFileName() + "#" + item->getName();
}