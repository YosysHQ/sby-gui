#ifndef SBYITEM_H
#define SBYITEM_H

#include <string>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
#include "sbyparser.h"

class SBYItem {
public:
    SBYItem(boost::filesystem::path path, std::string name);
    virtual ~SBYItem() { }
    std::string getName() { return name; }
    std::string getFileName() { return path.filename().string(); }
    std::string getFullPath() { return path.string(); }
    std::string getWorkFolder() { return path.parent_path().string(); }    
    int getStatus() { return status; }
    int getPercentage() { return percentage; }

    virtual void update() = 0;
    virtual bool isTop() = 0;
    virtual std::string getTaskName() = 0;
    virtual std::string getContents() = 0;
protected:
    std::string name;
    boost::filesystem::path path;
    int status;
    int percentage;
};

class SBYTask : public SBYItem {
public:
    SBYTask(boost::filesystem::path path, std::string name, std::string content);
    void update() override;
    bool isTop() override { return false; }
    std::string getTaskName() override { return name; }
    std::string getContents() override { return content; };
private:
    std::string content;    
};

class SBYFile : public SBYItem  {
public:
    SBYFile(boost::filesystem::path path);
    void parse();
    bool haveTasks();
    void update() override;
    bool isTop() override { return true; }
    std::string getTaskName() override { return ""; }
    std::string getContents() override { return ""; };
    std::vector<std::unique_ptr<SBYTask>> &getTasks() { return tasks; }
private:
    SBYParser parser;
    std::vector<std::unique_ptr<SBYTask>> tasks;
};
#endif // SBYITEM_H
