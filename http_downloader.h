#pragma once

#include "downloader.h"
#include <atomic>
#include <fmt/format.h>
#include <memory>
#include <time.h>

struct Progress
{
  public:
    Progress(size_t file_size, std::shared_ptr<std::atomic_size_t> progress,
             std::shared_ptr<std::atomic_size_t> progress_old,
             std::shared_ptr<std::atomic<time_t>> timestamp)
        : total_(file_size), progress_(progress), progress_old_(progress_old),
          timestamp_(timestamp)
    {
    }

    void feed(size_t bytes)
    {
        size_t p = progress_->fetch_add(bytes);

        time_t now = time(nullptr);
        time_t prev_second = now - 1;
        if (timestamp_->compare_exchange_strong(prev_second, now))
        {
            p += bytes;
            size_t po = progress_old_->load();
            fmt::print("downloaded {:.2f}%, speed {} b/s\n",
                       double(p) / total_ * 100, p - po);

            progress_old_->store(p);
        }
    }

  private:
    const size_t total_;

    std::shared_ptr<std::atomic_size_t> progress_;
    std::shared_ptr<std::atomic_size_t> progress_old_;
    std::shared_ptr<std::atomic<time_t>> timestamp_;
};

class HttpDownloader : Downloader
{
  public:
    HttpDownloader(const char *request_path, Progress* progress);

    size_t get_file_size() override;
    int download(size_t bytes_from, size_t bytes_to, uint8_t *buffer) override;

  private:
    const char *request_path_ = nullptr;
    Progress* progress_;
};