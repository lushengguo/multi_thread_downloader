#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <cryptopp/hex.h>
#include <cryptopp/md5.h>

namespace md5
{
bool verify(const std::vector<uint8_t> &message, const std::string &hash)
{
    CryptoPP::Weak::MD5 md5;
    std::string calculatedHash;
    CryptoPP::StringSource calc(
        message.data(), message.size(), true,
        new CryptoPP::HashFilter(
            md5, new CryptoPP::HexEncoder(
                     new CryptoPP::StringSink(calculatedHash))));

    return strcasecmp(calculatedHash.c_str(), hash.c_str()) == 0;
}
} // namespace md5