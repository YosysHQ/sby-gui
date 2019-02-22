#include "sbyitem.h"
#include <fstream>

SBYItem::SBYItem(boost::filesystem::path path, std::string name) : path(path), name(name)
{

}

SBYTask::SBYTask(boost::filesystem::path path, std::string name, std::string content) : SBYItem(path, name), content(content)
{
}

void SBYTask::update()
{
    status = 0;
    percentage = 0;
    boost::filesystem::path dir = path.parent_path().string();
    dir /= (path.stem().string() + "_" + name);
    if (boost::filesystem::is_directory(dir) && boost::filesystem::exists(dir)) {
        if (boost::filesystem::exists(dir / "PASS")) {
            status = 1; 
            percentage = 100;
        }
        if (boost::filesystem::exists(dir / "ERROR")) {
            status = 2; 
            percentage = 100;
        }
    }
}

SBYFile::SBYFile(boost::filesystem::path path) : SBYItem(path, path.filename().string())
{
}

void SBYFile::parse()
{
    std::fstream fs;
    fs.open(path.string().c_str(), std::fstream::in);
    parser.parse(fs);
    fs.close();
    for (auto task : parser.get_tasks())
    {
        tasks.push_back(std::make_unique<SBYTask>(path,task, parser.get_config_content(task)));
    }
}

bool SBYFile::haveTasks()
{
    return parser.get_tasks().size() != 0;
}

void SBYFile::update()
{
    status = 0;
    percentage = 0;
    if (!haveTasks()) {
        boost::filesystem::path dir = path.parent_path().string();
        dir /= path.stem();
        if (boost::filesystem::is_directory(dir) && boost::filesystem::exists(dir)) {
            if (boost::filesystem::exists(dir / "PASS")) {
                status = 1;
                percentage = 100;
            }
            if (boost::filesystem::exists(dir / "ERROR")) {
                status = 2;
                percentage = 100;
            }
/*            
            boost::filesystem::path xmlFile = dir / path.stem();
            xmlFile.replace_extension("xml");
            if (boost::filesystem::exists(xmlFile)) {
                QDomDocument xml;
                QFile f(xmlFile.string().c_str());
                f.open(QIODevice::ReadOnly);
                xml.setContent(&f);
                f.close();
            }*/
        }        
    }
    else // have tasks
    {
        int counter = 0;
        int valid = 0;
        for (auto& task : tasks)
        {
            task->update();
            if (task->getStatus()!=0) counter++;
            if (task->getStatus()==1) valid++;
        }
        percentage = counter * 100 / tasks.size();
        if (tasks.size()==counter) {
            if (valid==counter) 
                status = 1;
            else if (valid == 0)
                status = 2;
            else
                status = 0;
        } else {
            status = 0;
        }
    }
}
