#include "sbyitem.h"
#include <boost/lexical_cast.hpp>
#include <QDomDocument>
#include <QFile>
#include <QProcess>
#include <QDir>

SBYItem::SBYItem(QFileInfo path, QString name) : path(path), name(name), timeSpent(-1), previousLog()
{

}

void SBYItem::updateFromXML(QFileInfo inputFile)
{    
    QFileInfo xmlFile(inputFile.path() + "/" + inputFile.completeBaseName() + ".xml");
    timeSpent = -1;
    previousLog = "";
    int errors = 0;
    int failures = 0;
    statusColor = 0;
    if (xmlFile.exists()) {
        QDomDocument xml;
        QFile f(xmlFile.absoluteFilePath());
        f.open(QIODevice::ReadOnly);
        xml.setContent(&f);
        f.close();
        QDomNodeList testsuiteList = xml.elementsByTagName("testsuite");
        if (!testsuiteList.isEmpty()) {
            QDomElement testsuite = testsuiteList.at(0).toElement();
            if (testsuite.hasAttribute("errors"))
            {
                errors = boost::lexical_cast<int>(testsuite.attribute("errors").toStdString());
            }
            if (testsuite.hasAttribute("failures"))
            {
                failures = boost::lexical_cast<int>(testsuite.attribute("failures").toStdString());
            }
        }
        QDomNodeList testcaseList = xml.elementsByTagName("testcase");
        if (!testcaseList.isEmpty()) {
            QDomElement testcase = testcaseList.at(0).toElement();
            if (testcase.hasAttribute("time"))
            {
                timeSpent = boost::lexical_cast<int>(testcase.attribute("time").toStdString());
            }
            if (testcase.hasAttribute("status"))
            {
                percentage = 100;                
                status = testcase.attribute("status");
                if (errors==0 && failures==0) {
                    statusColor = 1;                     
                }
                else {
                    statusColor = 2; 
                }
            }
        }
        QDomNodeList systemOutList = xml.elementsByTagName("system-out");
        if (!systemOutList.isEmpty()) {
            QDomElement systemOut = systemOutList.at(0).toElement();
            previousLog = systemOut.text();
        }
    } 
}

SBYTask::SBYTask(QFileInfo path, QString name, QString content, QStringList files, SBYFile* parent) : SBYItem(path, name), content(content), parent(parent), files(files)
{
}

void SBYTask::updateTask()
{    
    statusColor = 0;
    percentage = 0;
    vcdFiles.clear();
    QFileInfo dir(path.path() + "/" + path.completeBaseName() + "_" + name);
    if (dir.exists() && dir.isDir()) {
        QFileInfo indir(path.path() + "/" + path.completeBaseName() + "_" + name  + "/" + path.completeBaseName() + "_" + name);
        updateFromXML(indir);
        QFileInfo engine_0 = QFileInfo(indir.path() + "/" +  "engine_0");
        if (engine_0.exists() && engine_0.isDir()) {
            QDir vcdDir = QDir(engine_0.absoluteFilePath());
            vcdDir.setNameFilters(QStringList()<<"*.vcd");            
            vcdFiles = vcdDir.entryInfoList();
        }
    }
}

void SBYTask::update()
{
    parent->update();    
}

SBYFile::SBYFile(QFileInfo path) : SBYItem(path, path.fileName())
{
}

QString SBYFile::dumpcfg(QFileInfo &path, QString task)
{
    QProcess process;        
    QStringList args;
    args << "--dumpcfg";
    args << path.fileName();
    args << task;
    process.setProgram("sby");
    process.setArguments(args);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONUNBUFFERED","1");
    process.setProcessEnvironment(env);    
    process.setWorkingDirectory(path.dir().canonicalPath());
    process.setProcessChannelMode(QProcess::MergedChannels);       
    process.start();   
    process.waitForFinished();
    return process.readAllStandardOutput();
}

bool SBYFile::parse(QFileInfo &path)
{
    try {
        taskList.clear();
        configs.clear();

        QProcess process;        
        QStringList args;
        args << "--dumptasks";
        args << path.fileName();
        process.setProgram("sby");
        process.setArguments(args);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("PYTHONUNBUFFERED","1");
        process.setProcessEnvironment(env);
        process.setWorkingDirectory(path.dir().canonicalPath());
        process.setProcessChannelMode(QProcess::MergedChannels);       
        process.start();   
        process.waitForFinished();
        QString output(process.readAllStandardOutput());
        QStringList tasks = output.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);

        for (auto task : tasks) {
            configs.insert(task, dumpcfg(path, task)); 
            taskList << task;   
        }
        if (tasks.isEmpty())
        {
            configs.insert("", dumpcfg(path, ""));
        }
        return true;
    } catch (...) {
        return false;
    }
}

QStringList SBYFile::get_config_files(QString task)
{
    QStringList files;
    QStringList lines = configs[task].split(QRegExp("\n|\r\n|\r"));
    bool filesSection = false;
    for(auto line : lines)
    {
        line = line.trimmed();

        if (line == "--") filesSection = false;
        if (line.startsWith("[")) filesSection = false;
        if (filesSection && !line.isEmpty()) {
            files << line;
        }
        if (line == "[files]") filesSection = true;        
    }
    return files;
}

void SBYFile::parse()
{
    parse(path);
    for (auto task : taskList)
    {
        tasks.push_back(std::make_unique<SBYTask>(path,task, configs[task], get_config_files(task), this));
        tasksSet.insert(task);
    }
    if (!haveTasks()) {
        files = get_config_files("");
    }
}

void SBYFile::refresh()
{
    std::unique_ptr<SBYFile> f = std::make_unique<SBYFile>(path);
    f->parse();
    f->update();

    QSet<QString> newTaskSet = f->getTasksSet(); 
    QSet<QString> currTaskSet = getTasksSet(); 
    QSet<QString> newTasks = newTaskSet - currTaskSet;
    QSet<QString> deletedTasks = currTaskSet - newTaskSet;

    parse(path);
    if (!haveTasks()) {
        files = get_config_files("");
    }

    if(!newTasks.isEmpty())
    {
        for(auto name : newTasks) {
            tasks.push_back(std::make_unique<SBYTask>(path,name, configs[name], get_config_files(name), this));
            tasksSet.insert(name);
        }
    }
    if(!deletedTasks.isEmpty())
    {
        for(auto name : deletedTasks) {
            auto it = tasks.begin();
            while(it != tasks.end()) {
                if (it->get()->getTaskName() == name) {
                    tasks.erase(it);
                }
                else ++it;
            }            
            tasksSet.remove(name);
        }
    }
    update();
}

bool SBYFile::haveTasks()
{
    return taskList.size() != 0;
}

void SBYFile::update()
{
    statusColor = 0;
    percentage = 0;
    vcdFiles.clear();
    if (!haveTasks()) {
        QFileInfo dir(path.path() + "/" + path.completeBaseName());
        if (dir.exists() && dir.isDir()) {
            QFileInfo indir(path.path() + "/" + path.completeBaseName() + "/" + path.completeBaseName());
            updateFromXML(indir);
            QFileInfo engine_0 = QFileInfo(indir.path() + "/" +  "engine_0");
            if (engine_0.exists() && engine_0.isDir()) {
                QDir vcdDir = QDir(engine_0.absoluteFilePath());
                vcdDir.setNameFilters(QStringList()<<"*.vcd");            
                vcdFiles = vcdDir.entryInfoList();
            }            
        }        
    }
    else // have tasks
    {
        int counter = 0;
        int valid = 0;
        for (auto& task : tasks)
        {
            task->updateTask();
            if (task->getStatusColor()!=0) counter++;
            if (task->getStatusColor()==1) valid++;
        }
        percentage = counter * 100 / tasks.size();
        if (tasks.size()==counter) {
            if (valid==counter) 
                statusColor = 1;
            else if (valid == 0)
                statusColor = 2;
            else
                statusColor = 0;
        } else {
            statusColor = 0;
        }
    }
}

SBYTask *SBYFile::getTask(QString name)
{
    for(auto& task : tasks) {
        if (task->getTaskName() == name) 
            return task.get();
    }
    return nullptr;
}