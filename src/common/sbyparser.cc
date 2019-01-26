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

#include "common.h"
#include "log.h"
#include "sbyparser.h"
#include <iostream>

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

SBYParser::SBYParser()
{    
}

bool SBYParser::parse(std::istream &f)
{
    try {
        if (!f)
            log_error("Failed to open SBY file.\n");

        return true;
    } catch (log_execution_error_exception) {
        return false;
    }
}