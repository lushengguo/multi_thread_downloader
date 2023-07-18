#pragma once

#include <fmt/format.h>
#include <iostream>
#include <mutex>

extern std::mutex log_mutex;

template <typename... Args>
void Logger(fmt::format_string<Args...> &&fmt, Args &&...args)
{
    std::unique_lock<std::mutex> lock(log_mutex);
    std::cout << fmt::format(std::move(fmt), std::forward<Args>(args)...) << std::endl;
}