#pragma once

#include <cstdio>

#include <util/stream/output.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/noncopyable.h>

#include "indexstorageface.h"

namespace NIndexerCore {

class TIndexStorageFactory : public IYndexStorageFactory, private TNonCopyable {
public:
    explicit TIndexStorageFactory(IOutputStream& log = Clog);
    ~TIndexStorageFactory() override;
    void GetStorage(IYndexStorage** Storage, IYndexStorage::FORMAT) override;
    void ReleaseStorage(IYndexStorage* Storage) override;
    void ClearPortionNames();
    void RemovePortions();
    void SaveYandexPortions(const char* fname);
    void LoadYandexPortions(const char* fname);
    void CopyFrom(const TIndexStorageFactory& other);
    const TVector<TString>& GetNames() const {
        return Names;
    }
    void FreeIndexResources();
    void InitIndexResources(const char* indexName, const char* workDir, const char* pref);
    int RemovePortion(size_t i);
    void RemovePortionWithName(size_t i);
    const TString& GetIndexPrefix() const {
        return IndexPrefix;
    }
    const TString& GetWorkDir() const {
        return WorkDir;
    }
protected:
    TString GetNextPortionName(IYndexStorage::FORMAT type);
protected:
    TString Index0_Name;
    TString Index1_Name;

    // нельзя индексировать в одной директории
    // двум процессам одновременно
    TVector<TString> Names;
    TString YandFile;
    //! @todo PORTION_FORMAT_LEMM and PORTION_FORMAT_ATTR should be removed
    //!       attributes and lemmas should be stored into single portion having PORTION_FORMAT

    IOutputStream& Log;
private:
    TString IndexPrefix;
    TString WorkDir;
    bool LemmPortionNameUsed;
    bool AttrPortionNameUsed;
};

}
