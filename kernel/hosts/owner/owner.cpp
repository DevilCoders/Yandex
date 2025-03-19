#include "owner.h"

#include <library/cpp/archive/yarchive.h>

#include <util/generic/algorithm.h>
#include <util/generic/singleton.h>
#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/mem.h>
#include <library/cpp/string_utils/url/url.h>

TStringBuf TOwnerCanonizer::GetUrlOwner(const TStringBuf& url) const {
    return GetHostOwner(GetHost(CutHttpPrefix(url)));
}

void TOwnerCanonizer::AddDom2(const char* host) {
    Dom2Set.Add(host);
}

void TOwnerCanonizer::LoadDom2(TBlob ownersfile) {
    TMemoryInput min(ownersfile.AsCharPtr(), ownersfile.Size());
    TString s;
    while (min.ReadLine(s))
        AddDom2(s.data());
}

void TOwnerCanonizer::UnloadDom2(TBlob ownersfile) {
    TMemoryInput min(ownersfile.AsCharPtr(), ownersfile.Size());
    TString s;
    while (min.ReadLine(s))
        Dom2Set.erase(s.data());
}

void TOwnerCanonizer::LoadDom2(const TString& ownersFilename) {
    if (!!ownersFilename)
        LoadDom2(TBlob::FromFile(ownersFilename));
}

namespace NOwner {

static TString PreparePath(const TString& urlrulesdir, const TString& file) {
    return TFsPath(!urlrulesdir ? "." : urlrulesdir) / file;
}

static const unsigned char URLRULES_DATA[] = {
    #include "urlrules.inc"
};

struct TUrlRulesData {
    THashMap<TString, TBlob> Data;

    TUrlRulesData()
    {
        TArchiveReader archive(TBlob::NoCopy(URLRULES_DATA, Y_ARRAY_SIZE(URLRULES_DATA)));

        for (ui32 i = 0; i < archive.Count(); ++i) {
            TString key = archive.KeyByIndex(i);
            TString key1 = key;
            if (key1.StartsWith('/'))
                key1.erase(0, 1);

            Data[key1] = archive.ObjectBlobByKey(key);
        }
    }

    TBlob Get(const TString& file) const {
        const TBlob* p = Data.FindPtr(file);
        Y_ENSURE(!!p, "do not know file " << file);
        return *p;
    }
};

TBlob GetCompiledDom2(const TString& name) {
    return Default<TUrlRulesData>().Get(name);
}

}

void TOwnerCanonizer::LoadTrueOwners(const TString& urlrulesdir) {
    using namespace NOwner;
    LoadDom2(PreparePath(urlrulesdir, AREAS_LST_FILENAME));
}

void TOwnerCanonizer::LoadTrueOwners() {
    using namespace NOwner;
    LoadDom2(GetCompiledDom2(AREAS_LST_FILENAME));
}

const TOwnerCanonizer& TOwnerCanonizer::WithTrueOwners() {
    struct TInit : TOwnerCanonizer {
        TInit() { LoadTrueOwners(); }
    };
    return Default<TInit>();
}

void TOwnerCanonizer::LoadSerpCategOwners(const TString& urlrulesdir) {
    using namespace NOwner;
    LoadDom2(PreparePath(urlrulesdir, _2LD_LIST_FILENAME));
    LoadDom2(PreparePath(urlrulesdir, UNGROUPED_LIST_FILENAME));
}

void TOwnerCanonizer::LoadSerpCategOwners() {
    using namespace NOwner;
    LoadDom2(GetCompiledDom2(_2LD_LIST_FILENAME));
    LoadDom2(GetCompiledDom2(UNGROUPED_LIST_FILENAME));
}

const TOwnerCanonizer& TOwnerCanonizer::WithSerpCategOwners() {
    struct TInit : TOwnerCanonizer {
        TInit() { LoadSerpCategOwners(); }
    };
    return Default<TInit>();
}

void TOwnerCanonizer::UnloadDom2(const TString& ownersFilename) {
    if (!!ownersFilename)
        UnloadDom2(TBlob::FromFile(ownersFilename));
}

// (un)loads compiled in file from yweb/urlrules
void CompiledDom2(TOwnerCanonizer& c, const TString& filename, bool load) {
    TBlob b = NOwner::GetCompiledDom2(filename);
    if (load)
        c.LoadDom2(b);
    else
        c.UnloadDom2(b);
}
