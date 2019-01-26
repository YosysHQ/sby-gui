/*
 *  sby-gui -- SymbiYosys GUI
 *
 *  Copyright (C) 2018  Clifford Wolf <clifford@symbioticeda.com>
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

#include <stdexcept>

#ifndef COMMON_H
#define COMMON_H

#if defined(__GNUC__) || defined(__clang__)
#define SBY_ATTRIBUTE(...) __attribute__((__VA_ARGS__))
#define SBY_NORETURN __attribute__((noreturn))
#define SBY_DEPRECATED __attribute__((deprecated))
#define SBY_PACKED_STRUCT(...) __VA_ARGS__ __attribute__((packed))
#elif defined(_MSC_VER)
#define SBY_ATTRIBUTE(...)
#define SBY_NORETURN __declspec(noreturn)
#define SBY_DEPRECATED __declspec(deprecated)
#define SBY_PACKED_STRUCT(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#else
#define SBY_ATTRIBUTE(...)
#define SBY_NORETURN
#define SBY_DEPRECATED
#define SBY_PACKED_STRUCT(...) __VA_ARGS__
#endif


class assertion_failure : public std::runtime_error
{
  public:
    assertion_failure(std::string msg, std::string expr_str, std::string filename, int line);

    std::string msg;
    std::string expr_str;
    std::string filename;
    int line;
};

SBY_NORETURN
inline void assert_fail_impl(const char *message, const char *expr_str, const char *filename, int line)
{
    throw assertion_failure(message, expr_str, filename, line);
}

SBY_NORETURN
inline void assert_fail_impl_str(std::string message, const char *expr_str, const char *filename, int line)
{
    throw assertion_failure(message, expr_str, filename, line);
}

#define SBY_ASSERT(cond) (!(cond) ? assert_fail_impl(#cond, #cond, __FILE__, __LINE__) : (void)true)
#define SBY_ASSERT_MSG(cond, msg) (!(cond) ? assert_fail_impl(msg, #cond, __FILE__, __LINE__) : (void)true)
#define SBY_ASSERT_FALSE(msg) (assert_fail_impl(msg, "false", __FILE__, __LINE__))
#define SBY_ASSERT_FALSE_STR(msg) (assert_fail_impl_str(msg, "false", __FILE__, __LINE__))

#endif
