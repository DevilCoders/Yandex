#pragma once

#include <library/cpp/deprecated/fgood/fgood.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/stream/file.h>

class TTextLineGenerator {
    struct TGenRec {
        const char* Str;
        size_t Len;
        unsigned Match;
        TGenRec()
            : Str(nullptr)
            , Len(0)
            , Match(0)
        {
        }
    };

public:
    template <class Table>
    TTextLineGenerator(const char* mask, const Table& nameTable)
        : Pattern(mask)
        , Generator(MakeGenerator(Pattern.data(), nameTable))
    {
    }

    bool CanGenerateByPcreMatch(unsigned nmatches, int* matches) const {
        for (const auto& it : Generator) {
            if (it.Match) {
                if (it.Match >= nmatches || matches[2 * it.Match] < 0)
                    return false;
            }
        }
        return true;
    }

    TString GenerateByPcreMatch(const char* string, unsigned nmatches, int* matches) const {
        TString ret;
        for (const auto& it : Generator) {
            ret += TString(it.Str, it.Len);
            if (it.Match) {
                if (it.Match >= nmatches || matches[2 * it.Match] < 0)
                    ythrow yexception() << "undefined variable $" << it.Match << " in generated line";
                ret += TString(string, matches[2 * it.Match], matches[2 * it.Match + 1] - matches[2 * it.Match]);
            }
        }
        return ret;
    }

private:
    template <class Table>
    static TVector<TGenRec> MakeGenerator(const char* mask, const Table& nameTable) {
        TVector<TGenRec> ret;
        const char* p = mask;
        while (*p) {
            TGenRec r;
            r.Str = p;
            const char* e = strchr(p, '$');
            if (!e) {
                r.Len = strlen(p);
                r.Match = 0;
                e = p + r.Len;
            } else {
                r.Len = e - p;
                ++e;
                switch (*e) {
                    case '$':
                        ++e;
                        ++r.Len;
                        r.Match = 0;
                        break;
                    case '(':
                        p = e + 1;
                        e = strchr(e, ')');
                        if (!e)
                            ythrow yexception() << "bracket not closed";
                        r.Match = LookupNameTable(nameTable, p, e - p);
                        ++e;
                        break;
                    default:
                        ythrow yexception() << "mask entry should be either $$ or $(name) '" << mask << "'";
                }
            }
            p = e;
            ret.push_back(r);
        }
        return ret;
    }

    template <class Table>
    static unsigned LookupNameTable(Table& nameTable, const char* name, const size_t len) {
        TString buf(name, len);
        typename Table::const_iterator it = nameTable.find(buf.data());
        if (it == nameTable.end())
            return (unsigned)(-1);
        return it->second;
    }

private:
    TString Pattern;
    TVector<TGenRec> Generator;
};

class TTextMultiLineGenerator {
public:
    template <class Table>
    TTextMultiLineGenerator(const char* filename, const Table& nameTable)
        : Generators(MakeGenerators(filename, nameTable))
    {
    }

    template <class Table>
    TTextMultiLineGenerator(IInputStream& in, const Table& nameTable)
        : Generators(MakeGenerators(in, nameTable))
    {
    }

    template <class Table>
    TTextMultiLineGenerator(const TString& genTemplate, const Table& nameTable) {
        Generators.push_back(TTextLineGenerator(genTemplate.data(), nameTable));
    }

    template <class Callback>
    void GenerateByPcreMatch(Callback& callback, const char* string, unsigned nmatches, int* matches) const {
        for (const auto& generator : Generators) {
            if (generator.CanGenerateByPcreMatch(nmatches, matches)) {
                callback(generator.GenerateByPcreMatch(string, nmatches, matches));
            }
        }
    }

private:
    template <class Table>
    static TVector<TTextLineGenerator> MakeGenerators(const char* filename, const Table& nameTable) {
        TFileInput fileIn(filename);
        return MakeGenerators(fileIn, nameTable);
    }

    template <class Table>
    static TVector<TTextLineGenerator> MakeGenerators(IInputStream& in, const Table& nameTable) {
        TVector<TTextLineGenerator> ret;
        TString s;
        while (in.ReadLine(s)) {
            if (s.size()) {
                TTextLineGenerator g(s.data(), nameTable);
                ret.push_back(g);
            }
        }
        return ret;
    }

private:
    TVector<TTextLineGenerator> Generators;
};
