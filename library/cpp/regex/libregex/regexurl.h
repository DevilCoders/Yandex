#pragma once

#include <library/cpp/charset/ci_string.h>
#include <library/cpp/deprecated/fgood/fgood.h>
#include <util/generic/hash.h>
#include <util/string/vector.h>
#include <util/stream/input.h>

#include "maskgen.h"
#include "regexstr.h"

class TNameTableMaker {
    // there is no need to copy strings, so just alias pointers
    typedef THashMap<const char*, unsigned, ci_hash, ci_equal_to> NameTable;

    class TNameTablePusher {
    public:
        TNameTablePusher(NameTable& nt)
            : NT(nt)
        {
        }
        void operator()(unsigned value, const char* name) {
            NT[name] = value;
        }

    private:
        NameTable& NT;
    };

protected:
    template <class TRe>
    static NameTable MakeNameTable(const TRe& re) {
        NameTable nt;
        TNameTablePusher p(nt);
        re.GetNameTable(p);
        return nt;
    }
};

class TStrAddCallback {
public:
    TStrAddCallback(TString& str)
        : Str(str)
    {
    }
    void operator()(const TString& s) {
        Str.append(s);
    }

private:
    TString& Str;
};

class TUrlRegexpConverter: public TNameTableMaker {
public:
    TUrlRegexpConverter(const char* refilename, const char* genfilename)
        : RE(refilename)
        , Gen(genfilename, MakeNameTable(RE))
    {
    }

    TUrlRegexpConverter(IInputStream& refInput, IInputStream& genInput)
        : RE(refInput)
        , Gen(genInput, MakeNameTable(RE))
    {
    }

    TUrlRegexpConverter(const TString& regExpr, const TString& genTemplate)
        : RE(regExpr)
        , Gen(genTemplate, MakeNameTable(RE))
    {
    }

    template <class Callback>
    bool Generate(Callback& callback, const char* string, size_t len) const {
        return RE.Generate(Gen, callback, string, len);
    }

    template <class Callback>
    bool Generate(Callback& callback, const char* string) const {
        return Generate(callback, string, strlen(string));
    }

    template <class Callback>
    bool Generate(Callback& callback, const TString& string) const {
        return Generate(callback, string.c_str(), string.size());
    }

    TString Convert(const TString& what) const {
        TString s;
        TStrAddCallback c(s);
        if (Generate(c, what))
            return s;
        return what;
    }

private:
    TMultiRegexStringParser RE;
    TTextMultiLineGenerator Gen;
};

class TListUrlRegexpConverter {
public:
    TListUrlRegexpConverter(const char* filename)
        : Converters(Load(filename))
    {
    }

    TListUrlRegexpConverter(IInputStream& in)
        : Converters(Load(in, "input-stream"))
    {
    }

    TString Convert(const TString& what) const {
        TString s;
        TStrAddCallback c(s);
        for (const auto& converter : Converters) {
            if (converter.Generate(c, what))
                return s;
        }
        return what;
    }

    template <class Callback>
    bool Generate(Callback& callback, const TString& string) const {
        callback(Convert(string));
        return true;
    }

private:
    static TVector<TUrlRegexpConverter> Load(const char* filename) {
        TFileInput fileIn(filename);
        return Load(fileIn, filename);
    }

    static TVector<TUrlRegexpConverter> Load(IInputStream& in, const TString& filename) {
        TVector<TUrlRegexpConverter> ret;
        TString s;
        while (in.ReadLine(s)) {
            if (! s.size() || s[0] == '#' || s[0] == ' ' || s[0] == '\t')
                continue;
            TVector<TString> vs = SplitString(s.data(), "\t", 4);
            if (vs.size() != 2)
                ythrow yexception() << "bad line in file " << filename << ": " << s.data();
            ret.push_back(TUrlRegexpConverter(vs[0], vs[1]));
        }
        return ret;
    }

private:
    TVector<TUrlRegexpConverter> Converters;
};
