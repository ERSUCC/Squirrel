#include "../include/base64.h"

std::string Base64::encode(const std::string str)
{
    const unsigned int truncated = str.size() - (str.size() % 3);

    std::string result;

    for (unsigned int i = 0; i < truncated; i += 3)
    {
        result += ALPHABET[str[i] >> 2];
        result += ALPHABET[((str[i] & 0b00000011) << 4) | (str[i + 1] >> 4)];
        result += ALPHABET[((str[i + 1] & 0b00001111) << 2) | (str[i + 2] >> 6)];
        result += ALPHABET[str[i + 2] & 0b00111111];
    }

    if (str.size() - truncated == 1)
    {
        result += ALPHABET[str[truncated] >> 2];
        result += ALPHABET[(str[truncated] & 0b00000011) << 4];
    }

    else if (str.size() - truncated == 2)
    {
        result += ALPHABET[str[truncated] >> 2];
        result += ALPHABET[((str[truncated] & 0b00000011) << 4) | (str[truncated + 1] >> 4)];
        result += ALPHABET[(str[truncated + 1] & 0b00001111) << 2];
    }

    return result;
}

std::string Base64::decode(const std::string str)
{
    const unsigned int truncated = str.size() - (str.size() % 4);

    std::string result;

    for (unsigned int i = 0; i < truncated; i += 4)
    {
        result += (base64ToAscii(str[i]) << 2) | (base64ToAscii(str[i + 1]) >> 4);
        result += (base64ToAscii(str[i + 1]) << 4) | (base64ToAscii(str[i + 2]) >> 2);
        result += (base64ToAscii(str[i + 2]) << 6) | base64ToAscii(str[i + 3]);
    }

    if (str.size() - truncated == 2)
    {
        result += base64ToAscii(str[truncated]) << 2;
    }

    else if (str.size() - truncated == 3)
    {
        result += (base64ToAscii(str[truncated]) << 2) | (base64ToAscii(str[truncated + 1]) >> 4);
        result += base64ToAscii(str[truncated + 1]) << 4;
    }

    return result;
}

char Base64::base64ToAscii(const char c)
{
    if (c == '+')
    {
        return 62;
    }

    if (c == '/')
    {
        return 63;
    }

    if (c < 58)
    {
        return c + 4;
    }

    if (c < 91)
    {
        return c - 65;
    }

    return c - 71;
}
