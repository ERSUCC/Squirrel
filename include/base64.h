#pragma once

#include <string>

#define ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

struct Base64
{
    static std::string encode(const std::string str);
    static std::string decode(const std::string str);

private:
    static char base64ToAscii(const char c);

};
