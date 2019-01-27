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
#include "common.h"
#include "log.h"

//// SymbiYosys parser of SBY config files in Python
//// Here for reference
////
//// def read_sbyconfig(sbydata, taskname):
////     cfgdata = list()
////     tasklist = list()
////
////     pycode = None
////     tasks_section = False
////     task_tags_active = set()
////     task_tags_all = set()
////     task_skip_block = False
////     task_skiping_blocks = False
////
////     for line in sbydata:
////         line = line.rstrip("\n")
////         line = line.rstrip("\r")
////
////         if tasks_section and line.startswith("["):
////             tasks_section = False
////
////         if task_skiping_blocks:
////             if line == "--":
////                 task_skip_block = False
////                 task_skiping_blocks = False
////                 continue
////
////         found_task_tag = False
////         task_skip_line = False
////
////         for t in task_tags_all:
////             if line.startswith(t+":"):
////                 line = line[len(t)+1:].lstrip()
////                 match = t in task_tags_active
////             elif line.startswith("~"+t+":"):
////                 line = line[len(t)+2:].lstrip()
////                 match = t not in task_tags_active
////             else:
////                 continue
////
////             if line == "":
////                 task_skiping_blocks = True
////                 task_skip_block = not match
////                 task_skip_line = True
////             else:
////                 task_skip_line = not match
////
////             found_task_tag = True
////             break
////
////         if len(task_tags_all) and not found_task_tag:
////             tokens = line.split()
////             if len(tokens) > 0 and tokens[0][0] == line[0] and tokens[0].endswith(":"):
////                 print("ERROR: Invalid task specifier \"%s\"." % tokens[0], file=sys.stderr)
////                 sys.exit(1)
////
////         if task_skip_line or task_skip_block:
////             continue
////
////         if tasks_section:
////             if line.startswith("#"):
////                 continue
////             line = line.split()
////             if len(line) > 0:
////                 tasklist.append(line[0])
////             for t in line:
////                 if taskname == line[0]:
////                     task_tags_active.add(t)
////                 task_tags_all.add(t)
////
////         elif line == "[tasks]":
////             tasks_section = True
////
////         elif line == "--pycode-begin--":
////             pycode = ""
////
////         elif line == "--pycode-end--":
////             gdict = globals().copy()
////             gdict["cfgdata"] = cfgdata
////             gdict["taskname"] = taskname
////             exec("def output(line):\n  cfgdata.append(line)\n" + pycode, gdict)
////             pycode = None
////
////         else:
////             if pycode is None:
////                 cfgdata.append(line)
////             else:
////                 pycode += line + "\n"
////
////     return cfgdata, tasklist

SBYParser::SBYParser() {}

void SBYParser::read_sbyconfig(std::istream &f, std::string taskname)
{
    f.seekg(0, f.beg);

    std::vector<std::string> cfgdata;

    std::string line;
    bool tasks_section = false;
    std::set<std::string> task_tags_active;
    bool task_skip_block = false;
    bool task_skiping_blocks = false;
    bool pycode_section = false;

    for (auto tag : task_tags[taskname])
        task_tags_active.emplace(tag);
    while (getline(f, line)) {
        boost::trim_right(line);
        if (tasks_section && boost::starts_with(line, "["))
            tasks_section = false;

        if (task_skiping_blocks) {
            if (line == "--") {
                task_skip_block = false;
                task_skiping_blocks = false;
                continue;
            }
        }

        bool found_task_tag = false;
        bool task_skip_line = false;

        for (const auto &t : task_tags_all) {
            bool match = false;
            if (boost::starts_with(line, t + ":")) {
                line = line.substr(t.size() + 1);
                match = task_tags_active.find(t) != task_tags_active.end(); // contain
            } else if (boost::starts_with(line, "~" + t + ":")) {
                line = line.substr(t.size() + 2);
                match = task_tags_active.find(t) == task_tags_active.end(); // does not contain
            } else
                continue;

            if (line == "") {
                task_skiping_blocks = true;
                task_skip_block = !match;
                task_skip_line = true;
            } else {
                task_skip_line = !match;
            }

            found_task_tag = true;
            break;
        }

        if (!task_tags_all.empty() && !found_task_tag) {
            std::vector<std::string> tokens;
            boost::split(tokens, line, boost::is_any_of("\t "), boost::token_compress_on);

            if (tokens.size() > 0 && tokens[0][0] == line[0] && boost::ends_with(tokens[0], ":")) {
                printf("ERROR: Invalid task specifier \"%s\".", tokens[0].c_str());
                return;
            }
        }

        if (task_skip_line || task_skip_block)
            continue;

        if (line == "[tasks]") {
            tasks_section = true;
        } else if (line == "--pycode-begin--") {
            pycode_section = true;
        } else if (line == "--pycode-end--") {
            pycode_section = false;
        } else {
            if (!pycode_section)
                cfgdata.push_back(line);
        }
    }
}

void SBYParser::extract_tasks(std::istream &f)
{
    f.seekg(0, f.beg);

    std::string line;
    bool tasks_section = false;

    while (getline(f, line)) {
        boost::trim_right(line);
        if (tasks_section && boost::starts_with(line, "["))
            tasks_section = false;

        if (tasks_section) {
            if (boost::starts_with(line, "#"))
                continue;
            std::vector<std::string> val;
            boost::split(val, line, boost::is_any_of("\t "), boost::token_compress_on);
            if (val.size() > 0 && !val[0].empty()) {
                tasklist.push_back(val[0]);
                task_tags.emplace(val[0], val);

                task_tags_all.insert(task_tags_all.end(), val.begin(), val.end());
            }
        } else if (line == "[tasks]") {
            tasks_section = true;
        }
    }
}

bool SBYParser::parse(std::istream &f)
{
    try {
        if (!f)
            log_error("Failed to open SBY file.\n");

        extract_tasks(f);
        read_sbyconfig(f, "");
        return true;
    } catch (log_execution_error_exception) {
        return false;
    }
}