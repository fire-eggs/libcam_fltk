//
// Created by kevin on 2/25/22.
//
#include <stdio.h>
#include <stdarg.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

void dolog(const char *fmt, ...)
{
    char logbuff[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logbuff,1024,fmt, ap);
    spdlog::info(logbuff);
    va_end(ap);
}

void initlog()
{
    auto max_size = 1048576 * 5;
    auto max_files = 3;
    auto logger = spdlog::rotating_logger_mt(":", "logs/libcam_fltk.log", max_size, max_files);
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(3));
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug

    spdlog::info("{:=^50}","*"); // split between starts
    spdlog::info("app start");
}