#include "sbyitem.h"
#include <boost/lexical_cast.hpp>
#include <QDomDocument>
#include <QFile>

SBYItem::SBYItem(boost::filesystem::path path, std::string name) : path(path), name(name), timeSpent(boost::none), previousLog(boost::none)
{

}

void SBYItem::updateFromXML(boost::filesystem::path xmlFile)
{
    xmlFile.replace_extension("xml");
    timeSpent = boost::none;
    previousLog = boost::none;
    int errors = 0;
    int failures = 0;
    statusColor = 0;
    if (boost::filesystem::exists(xmlFile)) {
        QDomDocument xml;
        QFile f(xmlFile.string().c_str());
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
                status = testcase.attribute("status").toStdString();
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
            previousLog = systemOut.text().toStdString();
        }
    } 
}

SBYTask::SBYTask(boost::filesystem::path path, std::string name, std::string content, SBYFile* parent) : SBYItem(path, name), content(content), parent(parent)
{
}

void SBYTask::updateTask()
{    
    statusColor = 0;
    percentage = 0;
    boost::filesystem::path dir = path.parent_path().string();
    dir /= (path.stem().string() + "_" + name);
    if (boost::filesystem::is_directory(dir) && boost::filesystem::exists(dir)) {
        updateFromXML(dir / (path.stem().string() + "_" + name));
    }
}

void SBYTask::update()
{
    parent->update();    
}

SBYFile::SBYFile(boost::filesystem::path path) : SBYItem(path, path.filename().string())
{
}

void SBYFile::parse()
{
    parser.parse(path.string());
    for (auto task : parser.get_tasks())
    {
        tasks.push_back(std::make_unique<SBYTask>(path,task, parser.get_config_content(task),this));
        tasksList.insert(QString(task.c_str()));
    }
}

void SBYFile::refresh()
{
    std::unique_ptr<SBYFile> f = std::make_unique<SBYFile>(path);
    f->parse();
    f->update();

    QSet<QString> newTaskSet = f->getTasksList(); 
    QSet<QString> currTaskSet = getTasksList(); 
    QSet<QString> newTasks = newTaskSet - currTaskSet;
    QSet<QString> deletedTasks = currTaskSet - newTaskSet;

    parser.parse(path.string());

    if(!newTasks.isEmpty())
    {
        for(auto name : newTasks) {
            tasks.push_back(std::make_unique<SBYTask>(path,name.toStdString(), parser.get_config_content(name.toStdString()),this));
            tasksList.insert(name);
        }
    }
    if(!deletedTasks.isEmpty())
    {
        for(auto name : deletedTasks) {
            auto it = tasks.begin();
            while(it != tasks.end()) {
                if (it->get()->getTaskName() == name.toStdString()) {
                    tasks.erase(it);
                }
                else ++it;
            }            
            tasksList.remove(name);
        }
    }
    update();
}

bool SBYFile::haveTasks()
{
    return parser.get_tasks().size() != 0;
}

void SBYFile::update()
{
    statusColor = 0;
    percentage = 0;
    if (!haveTasks()) {
        boost::filesystem::path dir = path.parent_path().string();
        dir /= path.stem();
        if (boost::filesystem::is_directory(dir) && boost::filesystem::exists(dir)) {
            updateFromXML(dir / path.stem());
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

SBYTask *SBYFile::getTask(std::string name)
{
    for(auto& task : tasks) {
        if (task->getTaskName() == name) 
            return task.get();
    }
    return nullptr;
}