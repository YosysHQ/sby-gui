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

#ifndef SBYPARSER_H
#define SBYPARSER_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

class SBYParser
{
  public:
    SBYParser();

    bool parse(std::istream &f);
    std::vector<std::string> &get_tasks() { return tasklist; }
    std::vector<std::string> &get_all_tags() { return task_tags_all; }
    std::vector<std::string> &get_tags(std::string task) { return task_tags[task]; }
  private:
    void read_sbyconfig(std::istream &f, std::string taskname);
    void extract_tasks(std::istream &f);

    std::vector<std::string> tasklist;
    std::vector<std::string> task_tags_all;
    std::map<std::string, std::vector<std::string>> task_tags;
};

#endif // SBYPARSER_H