#pragma once
#ifndef LOG_HPP
#define LOG_HPP

#include <fmt/format.h>
#include <iostream>
#include <mutex>

extern std::mutex log_mutex;

template <typename... Args, bool newline = true>
void Logger(fmt::format_string<Args...> &&fmt, Args &&...args)
{
    std::unique_lock<std::mutex> lock(log_mutex);
    // std::cout << fmt::format(std::move(fmt), std::forward<Args>(args)...) <<
    // std::endl;
    fmt::print(std::move(fmt), std::forward<Args>(args)...);
    if (newline)
        std::cout << std::endl;
}

#endif