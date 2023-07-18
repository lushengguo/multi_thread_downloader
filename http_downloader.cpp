#include "http_downloader.h"
#include "log.h"
#include <curl/curl.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <regex>
#include <string.h>

HttpDownloader::HttpDownloader(const char *request_path, Progress *progress)
    : request_path_(request_path), progress_(progress)
{
}

struct WriteData
{
    uint8_t *buffer;
    size_t downloaded_byte = 0;
    std::function<void(size_t)> progress_printer;
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     WriteData *writer)
{
    size_t downloaded_byte = size * nmemb;
    uint8_t *buffer = static_cast<uint8_t *>(contents);
    memcpy(writer->buffer, buffer, downloaded_byte);
    if (writer->progress_printer)
        writer->progress_printer(downloaded_byte);
    writer->downloaded_byte += downloaded_byte;
    writer->buffer += downloaded_byte;
    return downloaded_byte;
}

int HttpDownloader::download(size_t bytes_from, size_t bytes_download,
                             uint8_t *buffer)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        Logger("Failed to initialize libcurl.");
        return 1;
    }

    WriteData write_data{buffer, 0};
    if (progress_)
    {
        write_data.progress_printer =
            std::bind(&Progress::feed, progress_, std::placeholders::_1);
    }
    curl_easy_setopt(curl, CURLOPT_URL, request_path_);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_data);

    //指定下载的范围
    /*
    HTTP协议中的Range头字段指定了客户端请求的资源的一个或多个子范围。这使得客户端可以请求部分资源而不是整个资源。Range头字段的语法如下：
    Range: bytes=start-end
    */
    struct curl_slist *headers = nullptr;
    std::string range_header = fmt::format("Range: bytes={}-{}", bytes_from,
                                           bytes_from + bytes_download - 1);
    headers = curl_slist_append(headers, range_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        Logger("Failed to perform request: {}", curl_easy_strerror(res));
    }

    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return write_data.downloaded_byte;
}

size_t HeaderCallback(void *contents, size_t size, size_t nmemb,
                      std::string *headers)
{
    size_t totalSize = size * nmemb;
    headers->append(static_cast<char *>(contents), totalSize);
    return totalSize;
}

size_t HttpDownloader::get_file_size()
{
    Logger("HttpDownloader::get_file_size！！");
    size_t file_size = 0;

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        Logger("Failed to initialize libcurl.");
        return file_size;
    }

    std::string headerData;
    curl_easy_setopt(curl, CURLOPT_URL, request_path_);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerData);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        Logger("Failed to perform request: {}", curl_easy_strerror(res));
    }
    else
    {
        std::regex pattern("Content-Length\\s*:\\s*(\\d+)\\s*\r\n");
        std::smatch result;
        if (std::regex_search(headerData, result, pattern))
            file_size = std::stoull(result[1].str());
    }

    curl_easy_cleanup(curl);

    return file_size;
}