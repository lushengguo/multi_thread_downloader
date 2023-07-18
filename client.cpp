#include "downloader.h"
#include "log.h"
#include <atomic>
#include <cryptopp/hex.h>
#include <cryptopp/md5.h>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <mutex>
#include <string_view>
#include <strings.h>
#include <thread>
#include <unordered_map>

bool verify_md5(const std::vector<uint8_t> &message, const std::string &hash)
{
    CryptoPP::MD5 md5;
    std::string calculatedHash;
    CryptoPP::StringSource calc(
        message.data(), message.size(), true,
        new CryptoPP::HashFilter(
            md5, new CryptoPP::HexEncoder(
                     new CryptoPP::StringSink(calculatedHash))));

    return strcasecmp(calculatedHash.c_str(), hash.c_str()) == 0;
}

struct DownloadTaskArg
{
    size_t download_begin_offset;
    size_t download_end_offset;
    uint8_t *buffer;

    static constexpr size_t retry_time = 3;

    static size_t fixed_block_size;
    static size_t file_size;
    static std::string resource_path;
    static std::atomic_bool error;
    static std::atomic_size_t progress;
};

// 极限情况下，每个线程分到的下载数量都很大，做一个单次下载限制
// 主要作用在于如果一部分文件一直失败的话，整个下载应该及时停止
size_t DownloadTaskArg::fixed_block_size = 10 * 1024 * 1024;
size_t DownloadTaskArg::file_size = 0;
std::string DownloadTaskArg::resource_path("");
std::atomic_bool DownloadTaskArg::error(false);
std::atomic_size_t DownloadTaskArg::progress(0);

void download_task(DownloadTaskArg arg)
{
    HttpDownloader downloader(arg.resource_path.c_str());
    size_t begin = arg.download_begin_offset;
    size_t end = arg.download_end_offset;
    uint8_t *buffer = arg.buffer;

    for (;;)
    {
        for (size_t retry = arg.retry_time; retry > 0; retry--)
        {
            if (arg.error.load(std::memory_order_acquire))
                return;

            size_t download_bytes = std::min(arg.fixed_block_size, end - begin);
            int downloaded_bytes =
                downloader.download(begin, download_bytes, buffer);

            if (downloaded_bytes == download_bytes)
            {
                begin += downloaded_bytes;
                buffer += downloaded_bytes;
                size_t progress_old = arg.progress.fetch_add(
                    downloaded_bytes, std::memory_order_release);

                Logger("download progress {:2}%",
                       ((double)(progress_old + downloaded_bytes) /
                        arg.file_size) *
                           100);

                if (begin == end)
                    return;

                break;
            }
            else if (retry == 0)
            {
                arg.error.store(true, std::memory_order_release);
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

std::unordered_map<std::string, std::pair<std::string, std::string>>
    resource_list = {
        {"100KB",
         {"4c6426ac7ef186464ecbb0d81cbfcb1e",
          "http://speedtest.tele2.net/100KB.zip"}},
        {"100MB",
         {"2f282b84e7e608d5852449ed940bfc51",
          "http://speedtest.tele2.net/100MB.zip"}},
        {"10MB",
         {"f1c9645dbc14efddc7d8a322685f26eb",
          "http://speedtest.tele2.net/10MB.zip"}},
        {"1KB",
         {"0f343b0931126a20f133d67c2b018a3b",
          "http://speedtest.tele2.net/1KB.zip"}},
        {"1MB",
         {"b6d81b360a5672d80c27430f39153e2c",
          "http://speedtest.tele2.net/1MB.zip"}},
        {"200MB",
         {"3566de3a97906edb98d004d6b947ae9b",
          "http://speedtest.tele2.net/200MB.zip"}},
        {"20MB",
         {"8f4e33f3dc3e414ff94e5fb6905cba8c",
          "http://speedtest.tele2.net/20MB.zip"}},
        {"2MB",
         {"b2d1236c286a3c0704224fe4105eca49",
          "http://speedtest.tele2.net/2MB.zip"}},
        {"3MB",
         {"d1dd210d6b1312cb342b56d02bd5e651",
          "http://speedtest.tele2.net/3MB.zip"}},
        {"500MB",
         {"d8b61b2c0025919d5321461045c8226f",
          "http://speedtest.tele2.net/500MB.zip"}},
        {"50MB",
         {"25e317773f308e446cc84c503a6d1f85",
          "http://speedtest.tele2.net/50MB.zip"}},
        {"512KB",
         {"59071590099d21dd439896592338bf95",
          "http://speedtest.tele2.net/512KB.zip"}},
        {"5MB",
         {"5f363e0e58a95f06cbe9bbc662c5dfb6",
          "http://speedtest.tele2.net/5MB.zip"}},
        {"1GB",
         {"cd573cfaace07e7949bc0c46028904ff",
          "http://speedtest.tele2.net/1GB.zip"}},
        {"10GB",
         {"2dd26c4d4799ebd29fa31e48d49e8e53",
          "http://speedtest.tele2.net/10GB.zip"}},
        {"50GB",
         {"e7f4706922e1edfdb43cd89eb1af606d",
          "http://speedtest.tele2.net/50GB.zip"}},
        {"100GB",
         {"09cd755eb35bc534487a5796d781a856",
          "http://speedtest.tele2.net/100GB.zip"}},
        {"1000GB",
         {"2c9a0f21395470f88f1ded4194979af8",
          "http://speedtest.tele2.net/1000GB.zip"}},
};

// 默认内存够大，如果不够大还要手动实现一份md5分段计算
int main()
{
    std::string resource_path = resource_list["1MB"].second;
    std::string md5 = resource_list["1MB"].first;

    HttpDownloader downloader(resource_path.c_str());
    const int file_size = downloader.get_file_size();
    if (file_size <= 0)
    {
        Logger("unexpected file size {} quit now ... ", file_size);
        return -1;
    }

    size_t thread_num = std::min<size_t>(
        std::thread::hardware_concurrency(),
        std::max<size_t>(1, file_size / DownloadTaskArg::fixed_block_size));
    thread_num = 2;
    Logger("file size is {}, use {} threads to accerlate download", file_size,
           thread_num);

    const size_t average_task_load = file_size / thread_num;
    std::vector<uint8_t> file(file_size, 0);
    Logger("vector size={}, vector end={}", file.size(),
           (size_t)file.data() + file.size());

    DownloadTaskArg::file_size = file_size;
    DownloadTaskArg::resource_path = resource_path;

    std::vector<std::thread> thread_pool;
    for (size_t i = 0; i < thread_num; i++)
    {
        size_t begin = i * average_task_load;
        size_t end = std::min<size_t>(begin + average_task_load, file_size);
        uint8_t *buffer = file.data() + begin;
        thread_pool.emplace_back(download_task,
                                 DownloadTaskArg{begin, end, buffer});
    }

    for (auto &&thread : thread_pool)
        if (thread.joinable())
            thread.join();

    if (not DownloadTaskArg::error.load(std::memory_order_relaxed))
    {
        bool success = verify_md5(file, md5);
        if (success)
        {
            Logger("download {} bytes success :)", file_size);
            return 0;
        }
    }

    Logger("download {} bytes failed :(", file_size);
    return -1;
}