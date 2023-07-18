#pragma once

#include <stddef.h>
#include <vector>
#include <stdint.h>

class Downloader
{
  public:
    virtual size_t get_file_size() = 0;
    virtual int download(size_t bytes_from, size_t bytes_to, uint8_t *buffer) = 0;
};

class HttpDownloader : Downloader
{
  public:
    HttpDownloader(const char *request_path);

    size_t get_file_size() override;
    int download(size_t bytes_from, size_t bytes_to, uint8_t *buffer) override;

  private:
    const char *request_path_ = nullptr;
};