#include "http_downloader.h"
#include "log.h"
#include <curl/curl.h>
#include <iostream>
#include <mutex>
#include <regex>
#include <string.h>

HttpDownloader::HttpDownloader(const char *request_path)
    : request_path_(request_path)
{
}

struct WriteData
{
    uint8_t *buffer;
    size_t downloaded_byte = 0;
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     WriteData *output)
{
    size_t downloaded_byte = size * nmemb;
    uint8_t *buffer = static_cast<uint8_t *>(contents);
    Logger("in thread {} already downloaded {} bytes and commit {} to {}",
    gettid(),
           output->downloaded_byte, downloaded_byte, (size_t)output->buffer);
    memcpy(output->buffer, buffer, downloaded_byte);
    output->downloaded_byte += downloaded_byte;
    output->buffer += downloaded_byte;
    return downloaded_byte;
}

int HttpDownloader::download(size_t bytes_from, size_t bytes_download,
                             uint8_t *buffer)
{
    curl_global_init(CURL_GLOBAL_ALL);

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        Logger("Failed to initialize libcurl.");
        return 1;
    }

    WriteData write_data{buffer, 0};
    curl_easy_setopt(curl, CURLOPT_URL, request_path_);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_data);

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

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return write_data.downloaded_byte;
}

size_t HeaderCallback(void *contents, size_t size, size_t nmemb,
                      std::string *output)
{
    size_t totalSize = size * nmemb;
    output->append(static_cast<char *>(contents), totalSize);
    return totalSize;
}

size_t HttpDownloader::get_file_size()
{
    size_t file_size = 0;

    curl_global_init(CURL_GLOBAL_ALL);

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
    curl_global_cleanup();

    return file_size;
}