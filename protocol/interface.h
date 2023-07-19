#pragma once
#ifndef PROTOCOL_INTERFACE_HPP
#define PROTOCOL_INTERFACE_HPP

#include <stddef.h>
#include <stdint.h>

class DownloaderInterface
{
  public:
    virtual size_t get_file_size() = 0;
    virtual int download(size_t bytes_from, size_t bytes_to,
                         uint8_t *buffer) = 0;
};

#endif