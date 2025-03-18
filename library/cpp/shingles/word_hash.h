#pragma once

#include "hash_func.h"

#include <util/charset/unidata.h>
#include <library/cpp/charset/codepage.h>
#include <util/charset/utf8.h>

template <typename T>
struct THashedWordPos {
    T Hash;     // Hash value
    size_t Pos; // Start position of hashed part of string
    size_t Len; // Length in characters of hashed part of string
    size_t Num; // Length in symbols (e.g. for utf8) of hashed part of string

    THashedWordPos()
        : Hash(0)
        , Pos(0)
        , Len(0)
        , Num(0)
    {
    }

    THashedWordPos(T hash, size_t pos, size_t len, size_t num)
        : Hash(hash)
        , Pos(pos)
        , Len(len)
        , Num(num)
    {
    }

    operator T() const {
        return Hash;
    }
};

struct TIsAlNum {
    static bool Do(wchar16 c) {
        return IsAlnum(c);
    }
};

struct TFakeTransform {
    template <typename T>
    static T Do(T t) {
        return t;
    }
};

struct TLowerCase {
    static char Do(char c) {
        return csYandex.ToLower(c);
    }

    static wchar16 Do(wchar16 c) {
        return ToLower(c);
    }

    static wchar32 Do(wchar32 c) {
        return ToLower(c);
    }
};

template <typename HashFuncType>
class THashFirstWideWord {
public:
    typedef HashFuncType THashFunc;
    typedef typename THashFunc::TValue TValue;
    typedef THashedWordPos<TValue> TResult;

    template <typename TCheck, typename TTransform>
    static TResult Do(const wchar16* s, size_t len, TValue init = THashFunc::Init) {
        TValue hash = init;
        size_t i = 0, j = len;
        for (; i < len; ++i) {
            if (!TCheck::Do(s[i])) {
                if (j == len)
                    continue;
                else
                    break;
            }
            if (j == len)
                j = i;
            hash = THashFunc::Do(static_cast<TValue>(TTransform::Do(s[i])), hash);
        }
        size_t k = i > j ? i - j : 0;
        return TResult(hash, j, k, k);
    }

    static TResult Do(const wchar16* s, size_t len, bool caseSensitive, TValue init = NFNV::TInit<TValue>::Val) {
        if (caseSensitive)
            return Do<TIsAlNum, TFakeTransform>(s, len, init);
        else
            return Do<TIsAlNum, TLowerCase>(s, len, init);
    }
};

template <typename HashFuncType>
class THashFirstUTF8Word: public THashFirstWideWord<HashFuncType> {
public:
    typedef THashFirstWideWord<HashFuncType> TBase;
    typedef typename TBase::TValue TValue;
    typedef typename TBase::TResult TResult;
    using TBase::Do;

    template <typename TCheck, typename TTransform>
    static TResult Do(const char* s, size_t len, TValue init = NFNV::TInit<TValue>::Val) {
        const ui8* str = (const ui8*)s;
        TValue hash = init;
        size_t i = 0, j = len, k = 0;
        while (i < len) {
            wchar32 rune;
            size_t runeLen;
            bool recoded = SafeReadUTF8Char(rune, runeLen, str + i, str + len) == RECODE_OK;
            if (!recoded || !TCheck::Do(rune)) {
                if (j == len) {
                    i += recoded ? runeLen : 1;
                    continue;
                } else
                    break;
            }
            if (j == len)
                j = i;

            hash = HashFuncType::Do(static_cast<TValue>(TTransform::Do(rune)), hash);
            ++k;
            i += runeLen;
        }
        return TResult(hash, j, i > j ? i - j : 0, k);
    }

    static TResult Do(const char* s, size_t len, bool caseSensitive, TValue init = NFNV::TInit<TValue>::Val) {
        if (caseSensitive)
            return Do<TIsAlNum, TFakeTransform>(s, len, init);
        else
            return Do<TIsAlNum, TLowerCase>(s, len, init);
    }
};

// One-byte characters must be in csYandex and are converted to wide on the fly
template <typename HashFuncType>
class THashFirstYandWord: public THashFirstWideWord<HashFuncType> {
public:
    typedef THashFirstWideWord<HashFuncType> TBase;
    typedef typename TBase::TValue TValue;
    typedef typename TBase::TResult TResult;
    using TBase::Do;

    template <typename TCheck, typename TTransform>
    static TResult Do(const char* s, size_t len, TValue init = NFNV::TInit<TValue>::Val) {
        TValue hash = init;
        size_t i = 0, j = len;
        for (; i < len; ++i) {
            wchar16 c = static_cast<wchar16>(csYandex.unicode[(ui8)s[i]]);
            if (!TCheck::Do(c)) {
                if (j == len)
                    continue;
                else
                    break;
            }
            if (j == len)
                j = i;
            hash = HashFuncType::Do(static_cast<TValue>(TTransform::Do(c)), hash);
        }
        size_t k = i > j ? i - j : 0;
        return TResult(hash, j, k, k);
    }

    static TResult Do(const char* s, size_t len, bool caseSensitive, TValue init = NFNV::TInit<TValue>::Val) {
        if (caseSensitive)
            return Do<TIsAlNum, TFakeTransform>(s, len, init);
        else
            return Do<TIsAlNum, TLowerCase>(s, len, init);
    }
};
