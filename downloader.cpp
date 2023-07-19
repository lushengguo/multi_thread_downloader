#include "downloader.h"
#include "log.h"
#include "protocol/http_downloader.h"
#include <atomic>
#include <memory>
#include <thread>
#include <time.h>

Downloader::Downloader(SupportProtocol protocol) : protocol_(protocol)
{
}

void Downloader::download_task(DownloadTaskArg arg)
{
    auto downloader = create_base_downloader(
        arg.resource_path_.c_str(),
        std::bind(&ProgressPrinter::feed, arg.printer_, std::placeholders::_1));
    if (not downloader)
        return;

    size_t begin = arg.download_begin_offset_;
    size_t end = arg.download_end_offset_;
    uint8_t *buffer = arg.buffer_;

    for (;;)
    {
        for (size_t retry = arg.retry_time_; retry > 0; retry--)
        {
            if (arg.error_->load(std::memory_order_acquire))
                return;

            size_t download_bytes =
                std::min(arg.fixed_block_size_, end - begin);
            int downloaded_bytes =
                downloader->download(begin, download_bytes, buffer);

            if (size_t(downloaded_bytes) == download_bytes)
            {
                begin += downloaded_bytes;
                buffer += downloaded_bytes;
                if (begin == end)
                    return;

                break;
            }
            else if (retry == 0)
            {
                arg.error_->store(true, std::memory_order_release);
                Logger("download failed, quit now ... ");
                return;
            }
            else // retry download
            {
                continue;
            }
        }
    }
}

bool Downloader::download(const char *resource_path,
                          std::vector<uint8_t> &result, size_t thread_num)
{
    auto downloader = create_base_downloader(resource_path, nullptr);
    if (not downloader)
        return false;

    const int file_size = downloader->get_file_size();
    if (file_size <= 0)
    {
        Logger("unexpected file size {} quit now ... ", file_size);
        return false;
    }

    if (thread_num != 0)
        thread_num = std::min<size_t>(
            std::thread::hardware_concurrency(),
            std::max<size_t>(1,
                             file_size / DownloadTaskArg::fixed_block_size_));

    Logger("file size is {}, use {} threads to accerlate download", file_size,
           thread_num);

    const size_t average_task_load = file_size / thread_num;
    result.resize(file_size);

    ProgressPrinter global_progress_printer(
        file_size, std::make_shared<std::atomic_size_t>(0),
        std::make_shared<std::atomic_size_t>(0),
        std::make_shared<std::atomic<time_t>>(time(nullptr)));

    std::shared_ptr<std::atomic_bool> error =
        std::make_shared<std::atomic_bool>(false);
    std::vector<std::jthread> thread_pool;
    for (size_t i = 0; i < thread_num; i++)
    {
        DownloadTaskArg arg;
        arg.download_begin_offset_ = i * average_task_load;
        arg.download_end_offset_ = std::min<size_t>(
            arg.download_begin_offset_ + average_task_load, file_size);
        arg.buffer_ = result.data() + arg.download_begin_offset_;
        arg.printer_ = &global_progress_printer;
        arg.file_size_ = file_size;
        arg.resource_path_ = resource_path;
        arg.error_ = error;

        thread_pool.emplace_back(&Downloader::download_task, this,
                                 std::move(arg));
    }

    for (auto &&thread : thread_pool)
        if (thread.joinable())
            thread.join();

    return !error->load(std::memory_order_release);
}

static std::string beautify_speed(size_t bytes_per_second)
{
    static std::vector<std::pair<size_t, std::string>> rules{
        {10, "kb/s"}, {20, "mb/s"}, {30, "gb/s"}};

    if (bytes_per_second == 0)
        return "0b/s";

    auto iter = std::find_if(rules.rbegin(), rules.rend(), [&](auto &&pair) {
        return bytes_per_second >> pair.first;
    });

    return fmt::format("{:.2f}{}",
                       double(bytes_per_second) / std::pow(2, iter->first),
                       iter->second);
}

void ProgressPrinter::feed(size_t bytes)
{
    size_t p = progress_->fetch_add(bytes);

    time_t now = time(nullptr);
    time_t prev_second = now - 1;
    if (timestamp_->compare_exchange_strong(prev_second, now) ||
        (p + bytes) == total_)
    {
        p += bytes;
        size_t po = progress_old_->load();
        fmt::print("downloaded {:.2f}%, speed {}\n", double(p) / total_ * 100,
                   beautify_speed(p - po));

        progress_old_->store(p);
    }
}