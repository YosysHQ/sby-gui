/*
 *  sby-gui -- SymbiYosys GUI
 *
 *  Copyright (C) 2019  Miodrag Milanovic <miodrag@symbioticeda.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "sbyparser.h"
#include <QProcess>

SBYParser::SBYParser() {}


QString SBYParser::dumpcfg(boost::filesystem::path path, QString task)
{
    QProcess process;        
    QStringList args;
    args << "--dumpcfg";
    args << path.filename().c_str();
    args << task;
    process.setProgram("sby");
    process.setArguments(args);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONUNBUFFERED","1");
    process.setProcessEnvironment(env);
    process.setWorkingDirectory(path.parent_path().string().c_str());
    process.setProcessChannelMode(QProcess::MergedChannels);       
    process.start();   
    process.waitForFinished();
    return process.readAllStandardOutput();
}

bool SBYParser::parse(boost::filesystem::path path)
{
    try {
        tasklist.clear();
        configs.clear();

        QProcess process;        
        QStringList args;
        args << "--dumptasks";
        args << path.filename().c_str();
        process.setProgram("sby");
        process.setArguments(args);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("PYTHONUNBUFFERED","1");
        process.setProcessEnvironment(env);
        process.setWorkingDirectory(path.parent_path().string().c_str());
        process.setProcessChannelMode(QProcess::MergedChannels);       
        process.start();   
        process.waitForFinished();
        QString output(process.readAllStandardOutput());
        QStringList tasks = output.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);

        for (auto task : tasks) {
            configs.insert(task, dumpcfg(path, task)); 
            tasklist << task;   
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

QStringList SBYParser::get_config_files(QString task)
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