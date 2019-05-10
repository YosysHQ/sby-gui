#ifndef SBYITEM_H
#define SBYITEM_H

#include <QSet>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QMap>
#include <memory>

class SBYItem {
public:
    SBYItem(QFileInfo path, QString name);
    virtual ~SBYItem() { }
    QString getName() { return name; }
    QString getFileName() { return path.fileName(); }
    QString getFullPath() { return path.absoluteFilePath(); }
    QString getWorkFolder() { return path.dir().absolutePath(); }    
    int getStatusColor() { return statusColor; }
    QString getStatus() { return status; }
    int getPercentage() { return percentage; }
    int &getTimeSpent() { return timeSpent; }
    QString &getPreviousLog() { return previousLog; }

    void updateFromXML(QFileInfo path);
    virtual void update() = 0;
    virtual bool isTop() = 0;
    virtual QString getTaskName() = 0;
    virtual QString getContents() = 0;
    virtual QStringList &getFiles() = 0;
    virtual QFileInfoList &getVCDFiles() = 0;
protected:
    QString name;
    QFileInfo path;
    int statusColor;
    QString status;
    int percentage;
    int timeSpent;
    QString previousLog;
};

class SBYFile;

class SBYTask : public SBYItem {
public:
    SBYTask(QFileInfo path, QString name, QString content, QStringList files, SBYFile* parent);
    void update() override;
    void updateTask();
    bool isTop() override { return false; }
    QString getTaskName() override { return name; }
    QString getContents() override { return content; };
    QStringList &getFiles() override { return files; }
    QFileInfoList &getVCDFiles() override { return vcdFiles; }
private:
    QString content;    
    SBYFile *parent;
    QStringList files;
    QFileInfoList vcdFiles;
};

class SBYFile : public SBYItem  {
public:
    SBYFile(QFileInfo path);
    void parse();
    bool haveTasks();
    void refresh();
    void update() override;
    bool isTop() override { return true; }
    QString getTaskName() override { return ""; }
    QString getContents() override { return ""; };
    QStringList &getFiles() override { return files; }
    QFileInfoList &getVCDFiles() override { return vcdFiles; }
    std::vector<std::unique_ptr<SBYTask>> &getTasks() { return tasks; }
    SBYTask *getTask(QString name);
    QSet<QString> &getTasksSet() { return tasksSet; }
private:
    bool parse(QFileInfo &path);
    QStringList get_config_files(QString task);
    QString dumpcfg(QFileInfo &path, QString task);

    QStringList taskList;
    QMap<QString, QString> configs;

    std::vector<std::unique_ptr<SBYTask>> tasks;
    QSet<QString> tasksSet;
    QStringList files;
    QFileInfoList vcdFiles;
};
#endif // SBYITEM_H
