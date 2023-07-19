#pragma once
#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP

#include "protocol/http_downloader.h"
#include <atomic>
#include <memory>
#include <vector>

enum class SupportProtocol
{
    Http
};

struct ProgressPrinter
{
  public:
    ProgressPrinter(size_t file_size,
                    std::shared_ptr<std::atomic_size_t> progress,
                    std::shared_ptr<std::atomic_size_t> progress_old,
                    std::shared_ptr<std::atomic<time_t>> timestamp)
        : total_(file_size), progress_(progress), progress_old_(progress_old),
          timestamp_(timestamp)
    {
    }

    void feed(size_t bytes);

  private:
    const size_t total_;
    std::shared_ptr<std::atomic_size_t> progress_;
    std::shared_ptr<std::atomic_size_t> progress_old_;
    std::shared_ptr<std::atomic<time_t>> timestamp_;
};

class Downloader
{
  public:
    Downloader(SupportProtocol protocol);

    bool download(const char *resource_path, std::vector<uint8_t> &result,
                  size_t thread_num = 0);

  private:
    template <typename... Args>
    std::unique_ptr<DownloaderInterface> create_base_downloader(Args... args)
    {
        switch (protocol_)
        {
        case SupportProtocol::Http:
            return std::unique_ptr<DownloaderInterface>(
                new HttpDownloader(std::forward<Args>(args)...));
        default:
            return nullptr;
        }
    }

    struct DownloadTaskArg
    {
        size_t download_begin_offset_ = 0;
        size_t download_end_offset_ = 0;
        uint8_t *buffer_ = nullptr;

        ProgressPrinter *printer_ = nullptr;

        // 极限情况下，每个线程分到的下载数量都很大，做一个单次下载限制
        // 主要作用在于如果一部分文件一直失败的话，整个下载应该及时停止
        size_t file_size_ = 0;
        std::string resource_path_;
        std::shared_ptr<std::atomic_bool> error_;

        static constexpr size_t fixed_block_size_ = 10 * 1024 * 1024;
        static constexpr size_t retry_time_ = 3;
    };

    void download_task(DownloadTaskArg arg);

  private:
    SupportProtocol protocol_;
};

std::string beautify_speed(size_t bytes_per_second);

#endif