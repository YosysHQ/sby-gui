#ifndef SBYITEM_H
#define SBYITEM_H

#include <QSet>
#include <QString>
#include <string>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include "sbyparser.h"

class SBYItem {
public:
    SBYItem(boost::filesystem::path path, std::string name);
    virtual ~SBYItem() { }
    std::string getName() { return name; }
    std::string getFileName() { return path.filename().string(); }
    std::string getFullPath() { return path.string(); }
    std::string getWorkFolder() { return path.parent_path().string(); }    
    int getStatusColor() { return statusColor; }
    std::string getStatus() { return status; }
    int getPercentage() { return percentage; }
    boost::optional<int> getTimeSpent() { return timeSpent; }
    boost::optional<std::string> getPreviousLog() { return previousLog; }

    void updateFromXML(boost::filesystem::path path);
    virtual void update() = 0;
    virtual bool isTop() = 0;
    virtual std::string getTaskName() = 0;
    virtual std::string getContents() = 0;
    virtual std::vector<std::string> &getFiles() = 0;
protected:
    std::string name;
    boost::filesystem::path path;
    int statusColor;
    std::string status;
    int percentage;
    boost::optional<int> timeSpent;
    boost::optional<std::string> previousLog;
};

class SBYFile;

class SBYTask : public SBYItem {
public:
    SBYTask(boost::filesystem::path path, std::string name, std::string content, std::vector<std::string> files, SBYFile* parent);
    void update() override;
    void updateTask();
    bool isTop() override { return false; }
    std::string getTaskName() override { return name; }
    std::string getContents() override { return content; };
    std::vector<std::string> &getFiles() override { return files; }
private:
    std::string content;    
    SBYFile *parent;
    std::vector<std::string> files;
};

class SBYFile : public SBYItem  {
public:
    SBYFile(boost::filesystem::path path);
    void parse();
    bool haveTasks();
    void refresh();
    void update() override;
    bool isTop() override { return true; }
    std::string getTaskName() override { return ""; }
    std::string getContents() override { return ""; };
    std::vector<std::string> &getFiles() override { return files; }
    std::vector<std::unique_ptr<SBYTask>> &getTasks() { return tasks; }
    SBYTask *getTask(std::string name);
    QSet<QString> &getTasksList() { return tasksList; }
private:
    SBYParser parser;
    std::vector<std::unique_ptr<SBYTask>> tasks;
    QSet<QString> tasksList;
    std::vector<std::string> files;
};
#endif // SBYITEM_H
