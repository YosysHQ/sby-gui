#ifndef SBYITEM_H
#define SBYITEM_H

#include <QSet>
#include <QString>
#include "Maybe.h"
#include <string>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>

#include "sbyparser.h"

class SBYItem {
public:
    SBYItem(boost::filesystem::path path, QString name);
    virtual ~SBYItem() { }
    QString getName() { return name; }
    QString getFileName() { return path.filename().string().c_str(); }
    QString getFullPath() { return path.string().c_str(); }
    QString getWorkFolder() { return path.parent_path().string().c_str(); }    
    int getStatusColor() { return statusColor; }
    QString getStatus() { return status; }
    int getPercentage() { return percentage; }
    Maybe<int> &getTimeSpent() { return timeSpent; }
    Maybe<QString> &getPreviousLog() { return previousLog; }

    void updateFromXML(boost::filesystem::path path);
    virtual void update() = 0;
    virtual bool isTop() = 0;
    virtual QString getTaskName() = 0;
    virtual QString getContents() = 0;
    virtual QStringList &getFiles() = 0;
    virtual std::vector<boost::filesystem::path> &getVCDFiles() = 0;
protected:
    QString name;
    boost::filesystem::path path;
    int statusColor;
    QString status;
    int percentage;
    Maybe<int> timeSpent;
    Maybe<QString> previousLog;
};

class SBYFile;

class SBYTask : public SBYItem {
public:
    SBYTask(boost::filesystem::path path, QString name, QString content, QStringList files, SBYFile* parent);
    void update() override;
    void updateTask();
    bool isTop() override { return false; }
    QString getTaskName() override { return name; }
    QString getContents() override { return content; };
    QStringList &getFiles() override { return files; }
    std::vector<boost::filesystem::path> &getVCDFiles() override { return vcdFiles; }
private:
    QString content;    
    SBYFile *parent;
    QStringList files;
    std::vector<boost::filesystem::path> vcdFiles;
};

class SBYFile : public SBYItem  {
public:
    SBYFile(boost::filesystem::path path);
    void parse();
    bool haveTasks();
    void refresh();
    void update() override;
    bool isTop() override { return true; }
    QString getTaskName() override { return ""; }
    QString getContents() override { return ""; };
    QStringList &getFiles() override { return files; }
    std::vector<boost::filesystem::path> &getVCDFiles() override { return vcdFiles; }
    std::vector<std::unique_ptr<SBYTask>> &getTasks() { return tasks; }
    SBYTask *getTask(QString name);
    QSet<QString> &getTasksList() { return tasksList; }
private:
    SBYParser parser;
    std::vector<std::unique_ptr<SBYTask>> tasks;
    QSet<QString> tasksList;
    QStringList files;
    std::vector<boost::filesystem::path> vcdFiles;
};
#endif // SBYITEM_H
