#pragma once

#include "downloader.h"
#include <atomic>
#include <memory>

struct Progress
{
    const size_t total;
    std::shared_ptr<std::atomic_size_t> cur;
    double prev_downloaded_percentage;
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