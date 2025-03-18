#pragma once

#include "ip_map.h"

#include <library/cpp/charset/codepage.h>

#include <util/folder/path.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/system/byteorder.h>
#include <util/system/mutex.h>
#include <util/system/unaligned_mem.h>


namespace NAntiRobot {
    class TAddr;

    ui32 StrToIp(const TStringBuf& ip);
    TString IpToStr(ui32 ip);

    void Trace(const char* format, ...);

    inline bool StartsWith(const TStringBuf& str, const TStringBuf& prefix) noexcept {
        return str.StartsWith(prefix);
    }

    inline bool StartsWithOneOf(const TStringBuf& /*str*/) {
        return false;
    }

    template <typename T, typename... Args>
    bool StartsWithOneOf(const TStringBuf& str, const T& anotherStr, const Args&... args) {
        return str.StartsWith(anotherStr) || StartsWithOneOf(str, args...);
    }

    inline bool EndsWith(const TStringBuf& str, const TStringBuf& suffix) noexcept {
        return str.EndsWith(suffix);
    }

    inline bool EndsWithOneOf(const TStringBuf& /*str*/) {
        return false;
    }

    template <typename T, typename... Args>
    bool EndsWithOneOf(const TStringBuf& str, const T& anotherStr, const Args&... args) {
        return str.EndsWith(anotherStr) || EndsWithOneOf(str, args...);
    }

    inline TStringBuf ToLower(const TStringBuf& s, char* buf) {
        static const CodePage* cp = CodePageByCharset(CODES_ASCII);

        return TStringBuf(buf, cp->ToLower(s.begin(), s.end(), buf));
    }

    const TString& ShortHostName();

#if defined(_MSC_VER) && _MSC_VER < 1900
#   define _sb ""
#else
    constexpr TStringBuf operator "" _sb(const char* str, size_t size) {
        return TStringBuf(str, size);
    }
#endif

    template <typename T>
    void FlushToFileIfNotLocked(const T& data, const TFsPath& filename, const TMutex& mutex) {
        if (TTryGuard<TMutex> guard{mutex}) {
            TOFStream out(filename);
            out << data;
        }
    }

    template <typename TKey, typename TValue>
    void MergeHashMaps(THashMap<TKey, TValue>& destination, const THashMap<TKey, TValue>& source) {
        for (const auto& item : source) {
            if (destination.contains(item.first)) {
                destination.at(item.first) = item.second;
            } else {
                destination.emplace(item);
            }
        }
    }

    TString NiceAddrStr(const TString& addr);

    TIpRangeMap<size_t> ParseCustomHashingRules(const TString& customHashingRules);

    TString GetRequesterAddress(const TSocket& socket);

    template <typename TParam, typename T>
    using TParametrized = T;

    template <typename T>
    T ReadLittle(const char** ptr) {
        T x = LittleToHost(ReadUnaligned<T>(*ptr));
        *ptr += sizeof(T);
        return x;
    }

    void LowerHexEncode(TStringBuf src, char* dst);
    TString LowerHexEncode(TStringBuf src);
}
