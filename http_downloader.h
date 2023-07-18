#pragma once

#include "downloader.h"
#include <atomic>
#include <memory>

struct Progress
{
  public:
    void feed(size_t bytes)
    {
        size_t progress_old = cur->fetch_add(bytes, std::memory_order_seq_cst);
        size_t progress = progress_old + bytes;
    }

  private:
    const size_t total;

    std::shared_ptr<std::atomic_size_t> cur;
    std::shared_ptr<std::atomic_size_t> percentage;
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