#include <util/generic/ptr.h>
#include <util/string/strip.h>
#include <util/string/cast.h>
#include <util/stream/file.h>

#include "indexstoragefactory.h"
#include "indexstorage.h"

namespace NIndexerCore {

int TIndexStorageFactory::RemovePortion(size_t i) {
    assert(i < Names.size());
    const TString& fn = Names[i];
    assert(!!fn);
    int ret = 0;
    if (fn.at(fn.size()-1) == 'w') {
        ret += ::remove((fn+"k").data());
        ret += ::remove((fn+"i").data());
    } else {
        ret += ::remove((fn+"ak").data());
        ret += ::remove((fn+"ai").data());
        ret += ::remove((fn+"lk").data());
        ret += ::remove((fn+"li").data());
    }
    return ret;
}

void TIndexStorageFactory::RemovePortionWithName(size_t i) {
    RemovePortion(i);
    Names[i].clear();
}

void TIndexStorageFactory::SaveYandexPortions(const char* fname) {
    TFixedBufferFileOutput outFile(fname);
    for (size_t i = 0; i < Names.size(); i++)
        outFile << Names[i] << '\n';
}

void TIndexStorageFactory::LoadYandexPortions(const char* fname) {
    TFileInput inFile(fname);
    TString name;
    while (inFile.ReadLine(name)) {
        Names.push_back(StripInPlace(name));
    }
}

void TIndexStorageFactory::RemovePortions() {
    for (size_t i = 0; i < Names.size(); i++) {
        if (!Names[i].empty())
            RemovePortionWithName(i);
    }
    Names.clear();
}

void TIndexStorageFactory::ClearPortionNames() {
    Names.clear();
}

void TIndexStorageFactory::GetStorage(IYndexStorage** storage, IYndexStorage::FORMAT type)
{
    if (type == IYndexStorage::FINAL_FORMAT) {
        *storage = new TIndexStorage(Index0_Name.data(), Index1_Name.data(), type,
            (Flags & Portion_NoForms ? YNDEX_VERSION_RAW64_HITS : YNDEX_VERSION_CURRENT), (Flags & Portion_UniqueHits));
    } else {
        const TString fn = GetNextPortionName(type);
        *storage = new TIndexStorage((fn + "k").data(), (fn + "i").data(), type,
            (Flags & Portion_NoForms ? YNDEX_VERSION_RAW64_HITS : YNDEX_VERSION_CURRENT), (Flags & Portion_UniqueHits));
    }
}

TString TIndexStorageFactory::GetNextPortionName(IYndexStorage::FORMAT type) {
    if (type == IYndexStorage::PORTION_FORMAT ||
        (type == IYndexStorage::PORTION_FORMAT_LEMM && LemmPortionNameUsed) ||
        (type == IYndexStorage::PORTION_FORMAT_ATTR && AttrPortionNameUsed))
    {
        TString fname = YandFile + ToString<size_t>(Names.size()) +
            (type == IYndexStorage::PORTION_FORMAT ? 'w'/*hole*/ : 'p'/*artial*/);
        Names.push_back(fname);
        LemmPortionNameUsed = false;
        AttrPortionNameUsed = false;
    }
    Y_ASSERT(!Names.empty());
    TString fn = Names.back();
    if (type == IYndexStorage::PORTION_FORMAT_ATTR) {
        fn += 'a';
        AttrPortionNameUsed = true;
    } else if (type == IYndexStorage::PORTION_FORMAT_LEMM) {
        fn += 'l';
        LemmPortionNameUsed = true;
    }
    return fn;
}

void TIndexStorageFactory::ReleaseStorage(IYndexStorage* storage) {
    THolder<TIndexStorage> out((TIndexStorage*)storage);
    out->Close();
}

TIndexStorageFactory::TIndexStorageFactory(IOutputStream& log)
    : Log(log)
    , LemmPortionNameUsed(true)
    , AttrPortionNameUsed(true)
{
}

void TIndexStorageFactory::FreeIndexResources() {
    // it can be called more than once, e.g.
    // 1) from AbortIndex
    // 2) from destructor
    IndexPrefix.clear();
    Index0_Name.clear();
    Index1_Name.clear();

    ClearPortionNames();
    WorkDir.clear();
    YandFile.clear();
}

TIndexStorageFactory::~TIndexStorageFactory() {
    // TYndexStorageFactory::~TYndexStorageFactory meant to be called in usual situations
    // after Merging etc. was called and done properly
    // i supposed to leave CleanUp for atexit() functions
    // ????
    FreeIndexResources();
}

void TIndexStorageFactory::InitIndexResources(const char* catalogOrBD, const char* wrkDir, const char* pref) {
    FreeIndexResources(); // error-proof

    IndexPrefix = catalogOrBD;
    Index0_Name = TString::Join(catalogOrBD, KEY_SUFFIX);
    Index1_Name = TString::Join(catalogOrBD, INV_SUFFIX);

    Names.reserve(1024);

    if (wrkDir && *wrkDir) {
        WorkDir = wrkDir;
        YandFile = WorkDir;
        const char last = YandFile.back(); // wrkDir is not empty here
        if (last != ':' && last != '\\' && last != '/')
            YandFile += '/';
        if (pref)
            YandFile += pref;
    }
}

void TIndexStorageFactory::CopyFrom(const TIndexStorageFactory& other) {
    Y_ASSERT(this != &other);
    Y_ASSERT(Names.empty());

    Flags = other.Flags;
    IndexPrefix = other.IndexPrefix;
    Index0_Name = other.Index0_Name;
    Index1_Name = other.Index1_Name;
    Names = other.Names;
    WorkDir = other.WorkDir;
    YandFile = other.YandFile;
}

}
