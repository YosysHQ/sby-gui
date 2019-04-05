#include "sbyitem.h"
#include <boost/lexical_cast.hpp>
#include <QDomDocument>
#include <QFile>

SBYItem::SBYItem(boost::filesystem::path path, QString name) : path(path), name(name), timeSpent(nothing()), previousLog(nothing())
{

}

void SBYItem::updateFromXML(boost::filesystem::path xmlFile)
{
    xmlFile.replace_extension("xml");
    timeSpent = nothing();
    previousLog = nothing();
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
                timeSpent = timeSpent.just(boost::lexical_cast<int>(testcase.attribute("time").toStdString()));
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
            previousLog = previousLog.just(systemOut.text());
        }
    } 
}

SBYTask::SBYTask(boost::filesystem::path path, QString name, QString content, QStringList files, SBYFile* parent) : SBYItem(path, name), content(content), parent(parent), files(files)
{
}

void SBYTask::updateTask()
{    
    statusColor = 0;
    percentage = 0;
    vcdFiles.clear();
    boost::filesystem::path dir = path.parent_path().string();
    dir /= (path.stem().string() + "_" + name.toStdString());
    if (boost::filesystem::is_directory(dir) && boost::filesystem::exists(dir)) {
        updateFromXML(dir / (path.stem().string() + "_" + name.toStdString()));
        boost::filesystem::path engine_0 = dir / "engine_0";
        if (boost::filesystem::is_directory(engine_0) && boost::filesystem::exists(engine_0)) {
            boost::filesystem::directory_iterator it(engine_0);
            boost::filesystem::directory_iterator endit;
            while (it != endit) {
                if(boost::filesystem::is_regular_file(*it) && it->path().extension() == ".vcd") {
                    vcdFiles.push_back(it->path());
                }
                ++it;
            }
        }
    }
}

void SBYTask::update()
{
    parent->update();    
}

SBYFile::SBYFile(boost::filesystem::path path) : SBYItem(path, path.filename().string().c_str())
{
}

void SBYFile::parse()
{
    parser.parse(path.string());
    for (auto task : parser.get_tasks())
    {
        tasks.push_back(std::make_unique<SBYTask>(path,task, parser.get_config_content(task), parser.get_config_files(task), this));
        tasksList.insert(task);
    }
    if (!haveTasks()) {
        files = parser.get_config_files("");
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
    if (!haveTasks()) {
        files = parser.get_config_files("");
    }

    if(!newTasks.isEmpty())
    {
        for(auto name : newTasks) {
            tasks.push_back(std::make_unique<SBYTask>(path,name, parser.get_config_content(name), parser.get_config_files(name), this));
            tasksList.insert(name);
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
    vcdFiles.clear();
    if (!haveTasks()) {
        boost::filesystem::path dir = path.parent_path().string();
        dir /= path.stem();
        if (boost::filesystem::is_directory(dir) && boost::filesystem::exists(dir)) {
            updateFromXML(dir / path.stem());
            
            boost::filesystem::path engine_0 = dir / "engine_0";
            if (boost::filesystem::is_directory(engine_0) && boost::filesystem::exists(engine_0)) {
                boost::filesystem::directory_iterator it(engine_0);
                boost::filesystem::directory_iterator endit;
                while (it != endit) {
                    if(boost::filesystem::is_regular_file(*it) && it->path().extension() == ".vcd") {
                        vcdFiles.push_back(it->path());
                    }
                    ++it;
                }
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