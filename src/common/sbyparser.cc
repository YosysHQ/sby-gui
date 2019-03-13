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
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include "common.h"
#include "log.h"

#include <QProcess>

SBYParser::SBYParser() {}


std::string SBYParser::dumpcfg(boost::filesystem::path path, std::string task)
{
    QProcess process;        
    QStringList args;
    args << "--dumpcfg";
    args << path.filename().c_str();
    args << task.c_str();
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
    return output.toStdString();
}

bool SBYParser::parse(boost::filesystem::path path)
{
    try {
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
            configs.emplace(task.toStdString(), dumpcfg(path, task.toStdString()));    
            tasklist.push_back(task.toStdString());            
        }
        if (tasks.isEmpty())
        {
            configs.emplace("", dumpcfg(path, ""));
        }
        return true;
    } catch (...) {
        return false;
    }
}