#pragma once

#include <kernel/search_types/search_types.h>

#include <util/system/defaults.h>
#include <util/system/yassert.h>
#include <util/generic/yexception.h>
#include <util/generic/noncopyable.h>

class TKeyConvertException: public yexception {
};


class TFormToKeyConvertor : private TNonCopyable {
private:
    enum { DefaultSize = MAXKEY_BUF * 4 + 1 };

    char* const Buffer;
    const size_t BufSize;
    const bool DeleteBuffer;

public:
    TFormToKeyConvertor()
        : Buffer(new char[DefaultSize])
        , BufSize(DefaultSize)
        , DeleteBuffer(true)
    {
    }
    TFormToKeyConvertor(char* buffer, size_t bufsize)
        : Buffer(buffer)
        , BufSize(bufsize)
        , DeleteBuffer(false)
    {
        Y_ASSERT(buffer);
    }
    ~TFormToKeyConvertor() {
        if (DeleteBuffer)
            delete [] Buffer;
    }
    //! @note the function always terminates the result with '\0'
    const char* Convert(const wchar16* token, size_t leng);
    const char* Convert(const wchar16* token, size_t leng, size_t& written);
    const char* ConvertUTF8(const char* token, size_t leng); // leng in bytes, not characters

    //! @param written      the number of characters copied into the buffer not including null terminator
    const char* ConvertAttrValue(const wchar16* value, size_t len, size_t& written);
    const char* ConvertAttrValue(const wchar16* value, size_t len) {
        size_t written = 0;
        return ConvertAttrValue(value, len, written);
    }
};

inline const char* TFormToKeyConvertor::Convert(const wchar16* token, size_t leng) {
    size_t written = 0;
    return Convert(token, leng, written);
}

bool HasExtSymbols(const wchar16* token, size_t leng);

bool TryToConvertUTF8ToYandex(char* buffer, size_t buflen, const char* token, size_t toklen); // toklen in bytes, not characters

void ConvertFromYandexToUTF8(const TStringBuf& src, TString& dst);

char* ConvertUnicodeToYandex(char* buffer, size_t buflen, const wchar16* text, size_t textlen, size_t& written);
