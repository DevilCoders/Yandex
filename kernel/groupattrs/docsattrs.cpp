#include "docsattrs.h"

#include "attrmap.h"
#include "metainfos.h"

#include <library/cpp/on_disk/chunks/chunked_helpers.h>

#include <util/folder/dirut.h>
#include <util/generic/singleton.h>

namespace NGroupingAttrs {

TDocsAttrs::TDocsAttrs(EMode mode, bool lockMemory)
    : Mode_(mode)
    , Data_(CreateEmptyDocsAttrsImpl())
    , Realtime_(nullptr)
    , Config_(nullptr)
    , Metainfos_(nullptr)
    , LockMemory_(lockMemory)
{
    StoreConfigAndMetainfos();
}

TDocsAttrs::TDocsAttrs(bool dynamicC2N, const char* indexName, bool lockMemory, bool readOnly, bool ignoreNoAttrs)
    : Mode_(CommonMode)
    , Data_(CreateEmptyDocsAttrsImpl())
    , Realtime_(nullptr)
    , Config_(nullptr)
    , Metainfos_(nullptr)
    , LockMemory_(lockMemory)
{
    if (!InitCommonMode(dynamicC2N, indexName, readOnly) && !ignoreNoAttrs)
        ythrow yexception() << "Can't open grouping attributes base " << GetIndexFileName(indexName).Quote();
    else
        StoreConfigAndMetainfos();
}

TDocsAttrs::TDocsAttrs(bool dynamicC2N, const char* indexName, NDoom::IChunkedWad* wad, const NDoom::IDocLumpMapper* mapper, bool index64, bool lockMemory)
    : Mode_(CommonMode)
    , LockMemory_(lockMemory)
{
    if (Data_.Get()) {
        Data_.Destroy();
    }

    Data_ = CreateDocsAttrsData(
        wad,
        mapper,
        TDocAttrsWadReader{wad},
        dynamicC2N,
        TDocsAttrs::GetWadIndexFileName(indexName).c_str(),
        LockMemory_,
        index64);

    StoreConfigAndMetainfos();
}

TDocsAttrs::TDocsAttrs(bool dynamicC2N, const TMemoryMap& mapping, bool lockMemory)
    : Mode_(CommonMode)
    , Realtime_(nullptr)
    , Config_(nullptr)
    , Metainfos_(nullptr)
    , LockMemory_(lockMemory)
{
    if (!InitCommonMode(dynamicC2N, mapping))
        ythrow yexception() << "Can't open grouping attributes base " << mapping.GetFile().GetName().Quote();
}

TDocsAttrs::TDocsAttrs(IDocsAttrsData* data)
    : Mode_(CommonMode)
    , Data_(data)
    , Realtime_(nullptr)
    , Config_(nullptr)
    , Metainfos_(nullptr)
    , LockMemory_(false)
{
    StoreConfigAndMetainfos();
}

bool TDocsAttrs::InitCommonMode(bool dynamicC2N, const char* indexName, bool readOnly) {
    if (CommonMode != Mode_) {
        Y_ASSERT(0);
        ythrow yexception() << "compatible only with Realtime mode";
    }

    if (IndexWadExists(indexName)) {
        SEARCH_INFO << "Loading indexaa.wad";
        return DoWadInit(dynamicC2N, indexName);
    }

    if (IndexWad64Exists(indexName)) {
        SEARCH_INFO << "Loading indexaa64.wad";
        return DoWad64Init(dynamicC2N, indexName);
    }

    SEARCH_INFO << "Cannot find indexaa.wad, falling back to indexaa";
    if (!IndexExists(indexName))
        return false;

    if (FileMap_.Get())
        FileMap_.Destroy();

    FileMap_.Reset(new TMemoryMap(GetIndexFileName(indexName).data(), readOnly ? TMemoryMapCommon::oRdOnly : TMemoryMapCommon::oRdWr));
    return DoInit(dynamicC2N);
}

bool TDocsAttrs::InitCommonMode(bool dynamicC2N, const TMemoryMap& mapping) {
    if (CommonMode != Mode_) {
        Y_ASSERT(0);
        ythrow yexception() << "compatible only with Realtime mode";
    }

    if (FileMap_.Get())
        FileMap_.Destroy();

    FileMap_.Reset(new TMemoryMap(mapping));
    return DoInit(dynamicC2N);
}

void TDocsAttrs::InitRealtimeMode(const TString& configDir, size_t size) {
    if (RealtimeMode != Mode_) {
        Y_ASSERT(0);
        ythrow yexception() << "compatible only with Common mode";
    }

    if (Data_.Get())
        Data_.Destroy();

    Realtime_ = new TMemoryAttrMap(configDir, size);
    Data_.Reset(Realtime_);
    StoreConfigAndMetainfos();
}

void TDocsAttrs::Clear() {
    if (CommonMode == Mode_) {
        Data_.Destroy();
        File_.Drop();
        FileMap_.Destroy();
    } else {
        Data_.Destroy();
    }
}

bool TDocsAttrs::DoInit(bool dynamicC2N) {
    Y_ASSERT(CommonMode == Mode_);

    File_ = TBlob::FromMemoryMap(*FileMap_, 0, (size_t)FileMap_->Length());
    TNamedChunkedDataReader reader(File_);

    if (Data_.Get())
        Data_.Destroy();

    try {
        Data_.Reset(CreateDocsAttrsImpl(reader, dynamicC2N, FileMap_->GetFile().GetName().c_str()).Release());
    } catch (const TBadDataException& ) {
        return false;
    }

    StoreConfigAndMetainfos();

    return true;
}

bool TDocsAttrs::DoWadInit(bool dynamicC2N, const char* indexName) {
    Y_ASSERT(CommonMode == Mode_);
    if (Data_.Get()) {
        Data_.Destroy();
    }

    TDocAttrsWadReader reader(GetWadIndexFileName(indexName), LockMemory_);

    try {
        Data_.Reset(CreateDocsAttrsImpl(std::move(reader), dynamicC2N, TDocsAttrs::GetWadIndexFileName(indexName).c_str()).Release());
    } catch (const TBadDataException&) {
        return false;
    }

    StoreConfigAndMetainfos();

    return true;
}

//bool TDocsAttrs::DoWad64Init()

bool TDocsAttrs::DoWad64Init(bool dynamicC2N, const char* indexName) {
    Y_ASSERT(CommonMode == Mode_);
    if (Data_.Get()) {
        Data_.Destroy();
    }

    TDocAttrsWadReader reader(GetWad64IndexFileName(indexName), LockMemory_);

    try {
        Data_.Reset(CreateDocsAttrs64Impl(std::move(reader), dynamicC2N, TDocsAttrs::GetWadIndexFileName(indexName).c_str()).Release());
    } catch (const TBadDataException&) {
        return false;
    }

    StoreConfigAndMetainfos();

    return true;
}

void TDocsAttrs::StoreConfigAndMetainfos() {
    Y_ASSERT(!!Data_);

    Config_ = &Data_->Config();
    Metainfos_ = &Data_->Metainfos();
}

bool TDocsAttrs::IndexExists(const TString& indexName) {
    return NFs::Exists(GetIndexFileName(indexName));
}

bool TDocsAttrs::IndexWadExists(const TString& indexName) {
    return NFs::Exists(GetWadIndexFileName(indexName));
}

bool TDocsAttrs::IndexWad64Exists(const TString& indexName) {
    return NFs::Exists(GetWad64IndexFileName(indexName));
}

struct TNamesSingleton {
    TString FilenameSuffix;

    TNamesSingleton() {
        FilenameSuffix = "aa";
    }
};

const TString& TDocsAttrs::GetFilenameSuffix() {
    return Default<TNamesSingleton>().FilenameSuffix;
}

TString TDocsAttrs::GetIndexFileName(const TString& indexName) {
    return TString::Join(indexName, TDocsAttrs::GetFilenameSuffix());
}

TString TDocsAttrs::GetWadIndexFileName(const TString& indexName) {
    return TString::Join(indexName, TDocsAttrs::GetFilenameSuffix(), ".wad");
}

TString TDocsAttrs::GetWad64IndexFileName(const TString& indexName) {
    return TString::Join(indexName, TDocsAttrs::GetFilenameSuffix(), "64.wad");
}

TAutoPtr<IDocsAttrsData> TDocsAttrs::CreateEmptyDocsAttrsImpl() {
    return CreateDocsAttrsData();
}

TAutoPtr<IDocsAttrsData> TDocsAttrs::CreateDocsAttrsImpl(const TNamedChunkedDataReader& reader,
                                                         bool dynamicC2N, const char* path) {
    return CreateDocsAttrsData(reader, dynamicC2N, path);
}

TAutoPtr<IDocsAttrsData> TDocsAttrs::CreateDocsAttrsImpl(TDocAttrsWadReader&& reader,
                                                         bool dynamicC2N, const char* path) {
    return CreateDocsAttrsData(std::move(reader), dynamicC2N, path, LockMemory_);
}

TAutoPtr<IDocsAttrsData> TDocsAttrs::CreateDocsAttrs64Impl(TDocAttrsWadReader&& reader,
                                                         bool dynamicC2N, const char* path) {
    return CreateDocsAttrs64Data(std::move(reader), dynamicC2N, path, LockMemory_);
}

}
