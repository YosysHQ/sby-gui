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
#include <fstream>
#include "gtest/gtest.h"

TEST(SBYParser, empty)
{
    std::stringstream ss("");
    SBYParser parser;
    ASSERT_EQ(parser.parse(ss), true);
}

TEST(SBYParser, invalid_file)
{
    std::ifstream is;
    is.open("not_existing_file.sby");
    SBYParser parser;
    ASSERT_EQ(parser.parse(is), false);
}

TEST(SBYParser, demo_parse)
{
    std::stringstream ss("");
    ss << "[options]" << std::endl;
    ss << "mode bmc" << std::endl;
    ss << "depth 100" << std::endl;
    ss << std::endl;
    ss << "[engines]" << std::endl;
    ss << "smtbmc" << std::endl;
    ss << std::endl;
    ss << "[script]" << std::endl;
    ss << "read -formal demo.sv" << std::endl;
    ss << "prep -top demo" << std::endl;
    ss << std::endl;
    ss << "[files]" << std::endl;
    ss << "demo.sv" << std::endl;

    SBYParser parser;
    ASSERT_EQ(parser.parse(ss), true);
    std::vector<std::string> tasks = parser.get_tasks();
    ASSERT_EQ(tasks.empty(), true);
}

TEST(SBYParser, tasks_parse)
{
    std::stringstream ss("");
    ss << "[tasks]" << std::endl;
    ss << "prf" << std::endl;
    ss << "cvr" << std::endl;
    ss << std::endl;
    ss << "[options]" << std::endl;
    ss << "prf: mode prove" << std::endl;
    ss << "prf: depth  5" << std::endl;
    ss << "cvr: mode cover" << std::endl;
    ss << "cvr: depth 60" << std::endl;
    ss << std::endl;
    ss << "[engines]" << std::endl;
    ss << "smtbmc" << std::endl;
    ss << std::endl;
    ss << "[script]" << std::endl;
    ss << "read -formal -DPFCACHE fwb_master.v" << std::endl;
    ss << "read -formal -DPFCACHE pfcache.v" << std::endl;
    ss << "prep -top pfcache" << std::endl;
    ss << std::endl;
    ss << "[files]" << std::endl;
    ss << "../../rtl/core/pfcache.v" << std::endl;
    ss << "../../rtl/ex/fwb_master.v" << std::endl;
    SBYParser parser;
    ASSERT_EQ(parser.parse(ss), true);
    std::vector<std::string> tasks = parser.get_tasks();
    ASSERT_EQ(tasks.empty(), false);
    ASSERT_EQ(tasks.size(), 2);
    ASSERT_EQ(parser.get_all_tags().size(), 2);
    for (auto task : tasks) {
        ASSERT_EQ(parser.get_tags(task).size(), 1);
    }
    ASSERT_EQ(parser.get_tags("prf")[0], "prf");
    ASSERT_EQ(parser.get_tags("cvr")[0], "cvr");
}

TEST(SBYParser, multi_tag)
{
    std::stringstream ss("");
    ss << "[tasks]" << std::endl;
    ss << "full		system  lock    dualread" << std::endl;
    ss << "full_single	system  lock" << std::endl;
    ss << "bare		nolocal lock    dualread" << std::endl;
    ss << "nolock_nolocal	nolocal nolock  dualread" << std::endl;
    ss << "nolock_system	system  nolock  dualread" << std::endl;
    ss << "piped		system	lock    dualread" << std::endl;
    ss << "cover           system lock     dualread" << std::endl;
    ss << "cover_pipe      system lock     dualread cover piped" << std::endl;
    ss << std::endl;
    ss << "[options]" << std::endl;
    ss << "~cover: mode prove" << std::endl;
    ss << "cover:  mode cover" << std::endl;
    ss << "depth 16" << std::endl;
    ss << "full:           depth 10" << std::endl;
    ss << "bare:           depth 11" << std::endl;
    ss << "nolock_nolocal: depth 11" << std::endl;
    ss << "nolock_system:  depth 11" << std::endl;
    ss << "piped:          depth 11" << std::endl;
    ss << "cover:          depth 24" << std::endl;
    ss << "cover_pipe:     depth 45" << std::endl;
    ss << std::endl;
    ss << "[engines]" << std::endl;
    ss << "# smtbmc yices" << std::endl;
    ss << "smtbmc --nopresat boolector" << std::endl;
    ss << "# abc pdr" << std::endl;
    ss << "# aiger suprove" << std::endl;
    ss << "# aiger avy" << std::endl;
    ss << std::endl;
    ss << "[script]" << std::endl;
    ss << "read -formal -D DCACHE fwb_master.v" << std::endl;
    ss << "read -formal -D DCACHE iscachable.v" << std::endl;
    ss << "read -formal -D DCACHE dcache.v" << std::endl;
    ss << "piped:		chparam -set OPT_PIPE      1 dcache" << std::endl;
    ss << "~piped:		chparam -set OPT_PIPE      0 dcache" << std::endl;
    ss << "dualread:	chparam -set OPT_DUAL_READ_PORT	1 dcache" << std::endl;
    ss << "~dualread:	chparam -set OPT_DUAL_READ_PORT	0 dcache" << std::endl;
    ss << "lock:		chparam -set OPT_LOCK      1 dcache" << std::endl;
    ss << "system: 	chparam -set OPT_LOCAL_BUS 1 dcache" << std::endl;
    ss << "nolock:		chparam -set OPT_LOCK      0 dcache" << std::endl;
    ss << "nolocal:	chparam -set OPT_LOCAL_BUS 0 dcache" << std::endl;
    ss << "prep -top dcache" << std::endl;
    ss << std::endl;
    ss << "[files]" << std::endl;
    ss << "../../rtl/core/dcache.v" << std::endl;
    ss << "../../rtl/core/iscachable.v" << std::endl;
    ss << "../../rtl/ex/fwb_master.v" << std::endl;
    ss << "#" << std::endl;
    SBYParser parser;
    ASSERT_EQ(parser.parse(ss), true);
    std::vector<std::string> tasks = parser.get_tasks();
    ASSERT_EQ(tasks.empty(), false);
    ASSERT_EQ(tasks.size(), 8);
    ASSERT_EQ(parser.get_all_tags().size(), 33);
    ASSERT_EQ(parser.get_tags("full").size(), 4);
    ASSERT_EQ(parser.get_tags("full_single").size(), 3);
    ASSERT_EQ(parser.get_tags("cover_pipe").size(), 6);
}