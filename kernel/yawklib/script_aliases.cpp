#include <library/cpp/charset/wide.h>
#include <util/generic/hash_set.h>
#include <util/stream/file.h>
#include <util/folder/dirut.h>
#include <util/string/strip.h>
#include <algorithm>
#include "script_aliases.h"
#include "wtrutil.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
class TScriptAliases
{
    struct TAlias {
        TUtf16String Name;
        TUtf16String Val;
        bool operator< (const TAlias& rhs) const {
            return Name.size() > rhs.Name.size();
        }
    };
    typedef TVector<TAlias> TData;
    TData Data;
    bool Sorted;
public:
    TScriptAliases()
        : Sorted(false)
    {}
    void Add(const TUtf16String &name, const TUtf16String &val);
    TUtf16String Subst(const TUtf16String &w, IOutputStream& warningsStream);
};
////////////////////////////////////////////////////////////////////////////////////////////////////
void TScriptAliases::Add(const TUtf16String &name, const TUtf16String &val)
{
    Data.emplace_back();
    Data.back().Name = name;
    Data.back().Val = val;
    Sorted = false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
TUtf16String TScriptAliases::Subst(const TUtf16String &w, IOutputStream& warningsStream)
{
    if (!Sorted) {
        std::sort(Data.begin(), Data.end());
        Sorted = true;
    }
    TUtf16String ret(w);
    for (TData::const_iterator it = Data.begin(); it != Data.end(); ++it) {
        size_t pos = ret.find(it->Name, 1);
        while (pos != TUtf16String::npos) {
            ret = ret.substr(0, pos) + it->Val + ret.substr(pos + it->Name.size());
            pos = ret.find(it->Name);
        }
    }
    if (ret.find('$', 1) != TUtf16String::npos)
        warningsStream << "Warning: unknown alias in " << WideToUTF8(w) << Endl;
    return ret;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
inline TUtf16String ToWide(const TString &s, ECharset enc) {
    return enc == CODES_UTF8? UTF8ToWide(s) : CharToWide(s, enc);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool LoadScriptIncludesAndAliases(const IFileSystem& fs, const TString &fileName, ECharset enc, TScriptAliases *aliases,
    TVector<TString> *lines, TVector<TVector<TUtf16String> > *prepared, THashSet<TString> *filesProcessed, IOutputStream& warningsStream)
{
    if (filesProcessed->find(fileName) != filesProcessed->end())
        return true;
    filesProcessed->insert(fileName);
    if (!fs.Exists(fileName)) {
        warningsStream << "Cannot open " << fileName << Endl;
        return false;
    }
    THolder<IInputStream> in = fs.LoadInputStream(fileName);
    TString lineWhole;
    while (in->ReadLine(lineWhole)) {
        lineWhole = StripInPlace(lineWhole);
        if (lineWhole.empty())
            continue;
        if (lineWhole.StartsWith("#include")) {
            TString rest = Strip(lineWhole.substr(8));
            if ((rest[0] == '"' && rest.back() == '"') || (rest[0] == '\'' && rest.back() == '\''))
                rest = Strip(rest.substr(1, rest.size() - 2));
            LoadScriptIncludesAndAliases(fs, rest, enc, aliases, lines, prepared, filesProcessed, warningsStream);
        } else if (lineWhole[0] == '#') {
            warningsStream << "Unknown preprocessor directive in " << lineWhole << Endl;
            continue;
        }

        TUtf16String linePrepared = aliases->Subst(ToWide(lineWhole, enc), warningsStream);
        if (linePrepared[0] == '$') {
            size_t tabPos = linePrepared.find('\t');
            if (tabPos == TUtf16String::npos) {
                warningsStream << "Cannot understand line: " << lineWhole << Endl;
            } else {
                aliases->Add(linePrepared.substr(0, tabPos), linePrepared.substr(tabPos + 1));
            }
            continue;
        }

        prepared->emplace_back();
        Wsplit(linePrepared.begin(), '\t', &prepared->back());
        lines->push_back(lineWhole);
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool LoadScriptIncludesAndAliases(const IFileSystem& fs, const TString &fname, ECharset enc,
                                  TVector<TString> *lines, TVector<TVector<TUtf16String> > *prepared, IOutputStream& warningsStream)
{
    THashSet<TString> filesProcessed;
    TScriptAliases aliases;
    return LoadScriptIncludesAndAliases(fs, fname, enc, &aliases, lines, prepared, &filesProcessed, warningsStream);
}

bool LoadScriptIncludesAndAliases(const TString &fname, ECharset enc,
                                  TVector<TString> *lines, TVector<TVector<TUtf16String> > *prepared, IOutputStream& warningsStream)
{
    THashSet<TString> filesProcessed;
    TScriptAliases aliases;
    IFileSystem fs;
    return LoadScriptIncludesAndAliases(fs, fname, enc, &aliases, lines, prepared, &filesProcessed, warningsStream);
}
