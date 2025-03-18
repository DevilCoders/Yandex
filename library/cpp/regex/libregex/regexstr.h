#pragma once

#include <library/cpp/deprecated/fgood/fgood.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/memory/tempbuf.h>
#include <util/system/byteorder.h>
#include <util/stream/input.h>
#include <util/stream/file.h>

#include <contrib/libs/pcre/pcre.h>

class TPcreFree {
public:
    template <class T>
    static inline void Destroy(T* t) noexcept {
        pcre_free(t);
    }
};

class TRegexStringParser {
    static const int DEFOPTIONS = PCRE_EXTENDED;

public:
    TRegexStringParser(const char* re, int options = DEFOPTIONS)
        : RE(ConstructPcre(re, options))
        , MaxCaptures(GetReProperty<int>(PCRE_INFO_CAPTURECOUNT) + 1)
        , RecursionLimit(-1)
    {
    }

    // Default copy constructor and assignment are Ok

    // @brief set pcre.match_limit_recursion option
    // limit <=0 means unlimited
    void SetRecursionLimit(int limit) {
        RecursionLimit = limit;
    }

    template <class F>
    bool Match(F& func, const char* string, size_t length, size_t startoffset = 0) const {
        TTempArray<int> captures_arr(MaxCaptures * 3);
        int* const captures = captures_arr.Data();
        unsigned ret = MatchRaw(string, length, startoffset, captures);
        if (!ret)
            return false;
        for (unsigned i = 1; i < ret; ++i) {
            if (captures[2 * i] >= 0) {
                const char* s = string + captures[2 * i];
                size_t l = captures[2 * i + 1] - captures[2 * i];
                func(i, s, l);
            }
        }
        return true;
    }

    template <class Generator>
    TString Substitute(const Generator& gen, const char* string, size_t length, size_t startoffset = 0) const {
        TTempArray<int> captures_arr(MaxCaptures * 3);
        int* const captures = captures_arr.Data();
        unsigned ret = MatchRaw(string, length, startoffset, captures);
        if (!ret)
            return "";
        return gen.GenerateByPcreMatch(string, ret, captures);
    }

    template <class Generator, class Callback>
    bool Generate(const Generator& gen, Callback& callback,
                  const char* string, size_t length, size_t startoffset = 0) const {
        TTempArray<int> captures_arr(MaxCaptures * 3);
        int* const captures = captures_arr.Data();
        unsigned ret = MatchRaw(string, length, startoffset, captures);
        if (!ret)
            return false;
        gen.GenerateByPcreMatch(callback, string, ret, captures);
        return true;
    }

    template <class F>
    void GetNameTable(F& func) const {
        const unsigned numRows = GetReProperty<int>(PCRE_INFO_NAMECOUNT);
        const unsigned rowLen = GetReProperty<int>(PCRE_INFO_NAMEENTRYSIZE);
        const uint8_t* table = GetReProperty<const uint8_t*>(PCRE_INFO_NAMETABLE);
        for (size_t i = 0; i < numRows; ++i, table += rowLen) {
            unsigned pos = (table[0] << 8) + table[1];
            const char* name = reinterpret_cast<const char*>(table + 2);
            func(pos, name);
        }
    }

protected:
    unsigned MatchRaw(const char* string, size_t length, size_t startoffset, int* captures) const {
        pcre_extra extra = {};

        if (RecursionLimit > 0) {
            extra.flags |= PCRE_EXTRA_MATCH_LIMIT_RECURSION;
            extra.match_limit_recursion = RecursionLimit;
        }

        int ret = pcre_exec(RE.Get(), &extra, string, length, startoffset, 0, captures, MaxCaptures * 3);
        if (ret < -1)
            ythrow yexception() << "PCRE match error " << ret << "";
        if (ret == -1)
            return 0;
        return ret ? (unsigned)ret : MaxCaptures;
    }

protected:
    template <typename T>
    inline T GetReProperty(int what) const {
        T ret = T{};
        int err = pcre_fullinfo(RE.Get(), nullptr, what, &ret);
        if (err)
            ythrow yexception() << "pcre_fullinfo failed (" << err << ") - memory corruption?";
        return ret;
    }

private:
    static pcre* ConstructPcre(const char* re, int options) {
        const char* errMsg = nullptr;
        int errCode = 0;
        pcre* ret = pcre_compile(re, options, &errMsg, &errCode, nullptr);
        if (!ret)
            ythrow yexception() << "pcre_compile failed on re '" << re << "': " << errMsg;
        return ret;
    }

protected:
    TSimpleSharedPtr<pcre, TPcreFree> RE;
    const size_t MaxCaptures;
    int RecursionLimit;
};

class TMultiRegexBuilder {
public:
    void Add(const TString& what) {
        if (Rex.size())
            Rex += "|";
        Rex += TString::Join("(?:", what + ")");
    }

    TString Get() const {
        return Rex;
    }

private:
    TString Rex;
};

class TMultiRegexStringParser: public TRegexStringParser {
public:
    template <class Src>
    explicit TMultiRegexStringParser(Src& src)
        : TRegexStringParser(LoadAndMakeRegEx(src).data())
    {
    }

    TMultiRegexStringParser(const TMultiRegexStringParser& f)
        : TRegexStringParser(f)
    {
    }

    TMultiRegexStringParser(const TString& regExpr)
        : TRegexStringParser(regExpr.data())
    {
    }

private:
    static TString LoadAndMakeRegEx(const char* filename) {
        TFileInput fileIn(filename);
        return LoadAndMakeRegEx(fileIn);
    }

    static TString LoadAndMakeRegEx(IInputStream& in) {
        TMultiRegexBuilder bld;
        TString s;
        while (in.ReadLine(s)) {
            if (! s.size() || s[0] == '#' || s[0] == ' ' || s[0] == '\t')
                continue;
            bld.Add(s);
        }
        return bld.Get();
    }

    static TString LoadAndMakeRegEx(const TVector<TString>& from) {
        TMultiRegexBuilder bld;
        for (const auto& it : from)
            bld.Add(it);
        return bld.Get();
    }

    static TString LoadAndMakeRegEx(const TMultiRegexBuilder& from) {
        return from.Get();
    }
};

template <class TRegex = TRegexStringParser>
class TRegexNamedParser: public TRegex {
    class TNameTable {
    public:
        template <class Callback>
        void GenerateByPcreMatch(Callback& callback,
                                 const char* string, unsigned nmatches, int* matches) const {
            for (unsigned i = 0; i < nmatches; ++i) {
                if (matches[2 * i] >= 0) {
                    const char* name = nullptr;
                    if (i) {
                        auto it = Names.find(i);
                        if (it != Names.end()) {
                            name = it->second;
                        }
                    }

                    callback(name,
                             string + matches[2 * i], matches[2 * i + 1] - matches[2 * i]);
                }
            }
        }

        void operator()(unsigned pos, const char* name) {
            Names[pos] = name;
        }

    private:
        TMap<unsigned, const char*> Names;
    };

public:
    template <class From>
    explicit TRegexNamedParser(const From& from)
        : TRegex(from)
    {
        TRegex::GetNameTable(NameTable);
    }

    template <class From1, class From2>
    explicit TRegexNamedParser(const From1& from1, const From2& from2)
        : TRegex(from1, from2)
    {
        TRegex::GetNameTable(NameTable);
    }

    TRegexNamedParser(const TRegexNamedParser& f)
        : TRegex(static_cast<const TRegex&>(f))
        , NameTable(f.NameTable)
    {
    }

    template <class Callback>
    bool Generate(Callback& callback,
                  const char* string, size_t length, size_t startoffset = 0) const {
        return TRegex::Generate(NameTable, callback, string, length, startoffset);
    }

private:
    TNameTable NameTable;
};
