#pragma once
#ifndef PROTOCOL_HTTP_DOWNLOADER_HPP
#define PROTOCOL_HTTP_DOWNLOADER_HPP

#include "interface.h"
#include <functional>

class HttpDownloader : public DownloaderInterface
{
  public:
    HttpDownloader(const char *request_path,
                   std::function<void(size_t)> progress_printer);

    virtual ~HttpDownloader() = default;

    virtual size_t get_file_size() final;
    virtual int download(size_t bytes_from, size_t bytes_to,
                         uint8_t *buffer) final;

  private:
    const char *request_path_ = nullptr;
    std::function<void(size_t)> printer_;
};

#endif