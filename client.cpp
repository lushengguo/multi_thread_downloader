#include "downloader.h"
#include "log.h"
#include "md5.h"
#include <chrono>
#include <cstddef>
#include <curl/curl.h>
#include <string>
#include <unordered_map>
struct CurlGlobalResourceGuard
{
    CurlGlobalResourceGuard() {
        curl_global_init(CURL_GLOBAL_ALL);
    }
    ~CurlGlobalResourceGuard() {
         curl_global_cleanup();
    }
};

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

using time_point = std::chrono::time_point<std::chrono::system_clock>;

std::string average_download_speed_illustrate(time_point begin, time_point end,
                                              size_t bytes)
{
    auto ms_begin =
        std::chrono::time_point_cast<std::chrono::milliseconds>(begin);
    auto ms_end = std::chrono::time_point_cast<std::chrono::milliseconds>(end);
    auto ms_duration =
        ms_end.time_since_epoch().count() - ms_begin.time_since_epoch().count();
    auto ms_duration2 = ms_duration;

    std::string r;
    size_t hours = ms_duration / (1000 * 60 * 60);
    ms_duration %= (1000 * 60 * 60);
    if (hours > 0)
         r.append(std::to_string(hours)).append("h");

    size_t minutes = ms_duration / (1000 * 60);
    ms_duration %= (1000 * 60);
    if (minutes > 0 || not r.empty())
         r.append(std::to_string(minutes)).append("m");

    size_t seconds = ms_duration / 1000;
    if (seconds > 0 || not r.empty())
         r.append(std::to_string(seconds)).append("s");

    ms_duration %= 1000;
    r.append(std::to_string(ms_duration)).append("ms");

    return fmt::format(
        "cost {}, average speed={}", r,
        beautify_speed(double(bytes) / (double(ms_duration2) / 1000)));
}

// 默认内存够大，如果不够大还要手动实现一份md5分段计算
int main()
{
    CurlGlobalResourceGuard guard;
    auto resource = resource_list["10MB"];
    std::string resource_path = resource.second;
    std::string md5 = resource.first;
    std::vector<uint8_t> data;
    time_point start = std::chrono::system_clock::now();

    Downloader downloader(SupportProtocol::Http);
    bool r = downloader.download(resource_path.c_str(), data);

    if (r)
    {
        bool success = md5::verify(data, md5);
        if (success)
        {
            Logger("download {} bytes success {} :)", data.size(),
                   average_download_speed_illustrate(
                       start, std::chrono::system_clock::now(), data.size()));
            return 0;
        }
    }

    Logger("download {} bytes failed :(", data.size());
    return -1;
}