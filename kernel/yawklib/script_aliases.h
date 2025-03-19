#pragma once

#include <library/cpp/charset/doccodes.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/split.h>
#include <util/folder/path.h>
#include <util/system/fs.h>
#include <util/stream/file.h>

class IFileSystem {
public:
    virtual THolder<IInputStream> LoadInputStream(const TFsPath& path) const {
        return THolder<IInputStream>(new TFileInput(path));
    }

    virtual bool Exists(const TFsPath& path) const {
        return NFs::Exists(path);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
template <class TScript>
void LoadScriptLine(TScript *res, const TVector<const char*> &tabsS, const TVector<TUtf16String> &tabs);
////////////////////////////////////////////////////////////////////////////////////////////////////
bool LoadScriptIncludesAndAliases(const TString &fileName, ECharset enc, TVector<TString> *lines, TVector<TVector<TUtf16String> > *prepared, IOutputStream& warningsStream = Cerr);

bool LoadScriptIncludesAndAliases(const IFileSystem& fs, const TString &fileName, ECharset enc, TVector<TString> *lines, TVector<TVector<TUtf16String> > *prepared, IOutputStream& warningsStream = Cerr);
////////////////////////////////////////////////////////////////////////////////////////////////////
template <class TScript>
bool LoadTabDelimitedScript(const TString &fileName, TScript *res, ECharset enc = CODES_YANDEX)
{
    TVector<TString> lines;
    TVector<TVector<TUtf16String> > prepared;
    if (!LoadScriptIncludesAndAliases(fileName, enc, &lines, &prepared))
        return false;
    Y_ASSERT(lines.size() == prepared.size());
    for (size_t n = 0; n < lines.size(); ++n) {
        TVector<const char*> tabsS;
        Split(lines[n].begin(), '\t', &tabsS);
        LoadScriptLine(res, tabsS, prepared[n]);
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
