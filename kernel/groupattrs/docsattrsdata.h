#pragma once

#include "config.h"
#include "iterator.h"
#include "metainfos.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/system/defaults.h>

#include <kernel/doom/offroad_doc_attrs_wad/offroad_doc_attrs_wad_io.h>

#include <kernel/doom/search_fetcher/search_fetcher.h>

class TNamedChunkedDataReader;
class TNamedChunkedDataWriter;
class TCategSeries;
class IRelevance;

namespace NGroupingAttrs {

typedef ui32 TVersion;
typedef ui32 TFormat;

class IDocsAttrsPreloader {
public:
    virtual void AnnounceDocIds(TConstArrayRef<ui32> docIds) = 0;

    virtual void PreloadDoc(ui32 doc, NDoom::TSearchDocLoader* loader) = 0;

    virtual ~IDocsAttrsPreloader() = default;
};

const i32 GROUP_ATTR_NO_VALUE = (2u << (8 * 4 - 2)) - 1;
constexpr size_t MAX_VIRTUAL_GROUP_ATTRS = 100;

class IDocsAttrsData {
public:
    virtual THolder<IDocsAttrsPreloader> MakePreLoader() const { return nullptr; }

    virtual ~IDocsAttrsData() {}

    virtual ui32 DocCount() const = 0;

    virtual bool DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance* externalRelevance) const = 0;

    virtual void DocCategs(ui32 docid, const ui32 attrnums[], size_t attrcount, TCategSeries* results[], IRelevance* externalRelevance) const = 0;

    virtual TVersion Version() const = 0;

    virtual TFormat Format() const = 0;

    virtual const TConfig& Config() const = 0;
    virtual TConfig& MutableConfig() = 0;

    virtual const TMetainfos& Metainfos() const = 0;
    virtual TMetainfos& MutableMetainfos() = 0;

    virtual TAutoPtr<IIterator> CreateIterator(ui32 docId) const = 0;

    virtual const void* GetRawDocumentData(ui32 docid) const = 0;

    virtual bool SetDocCateg(ui32 docid, ui32 attrnum, ui64 value) = 0;
};

class TBadDataException : public yexception {};


class TDocAttrsWadReader {
public:
    TDocAttrsWadReader(NDoom::IWad* wad) {
        Wad_ = wad;
    }

    TDocAttrsWadReader(const TString& indexName, bool lockMemory) {
        WadHolder_ = NDoom::IWad::Open(indexName, lockMemory);
        Wad_ = WadHolder_.Get();
    }

    const NDoom::IWad* Wad() const {
        return Wad_;
    }

    TString GetConfig() const {
        TBlob blob = Wad_->LoadGlobalLump(Id_);
        return TString(blob.AsCharPtr(), blob.Size());
    }

private:
    static constexpr NDoom::TWadLumpId Id_ = { NDoom::EWadIndexType::DocAttrsIndexType, NDoom::EWadLumpRole::Struct };
    THolder<NDoom::IWad> WadHolder_;
    NDoom::IWad* Wad_;
};

THolder<IDocsAttrsData> CreateDocsAttrsData(
    NDoom::IChunkedWad* wad,
    const NDoom::IDocLumpMapper* mapper,
    TDocAttrsWadReader&& reader,
    bool dynamicC2N,
    const char* path,
    bool lockMemory,
    bool is64);

TAutoPtr<IDocsAttrsData> CreateDocsAttrsData(const TNamedChunkedDataReader& reader,
                                             bool dynamicC2N,
                                             const char* path);

TAutoPtr<IDocsAttrsData> CreateDocsAttrsData(TDocAttrsWadReader&& reader,
                                             bool dynamicC2N,
                                             const char* path,
                                             bool lockMemory);

TAutoPtr<IDocsAttrsData> CreateDocsAttrs64Data(TDocAttrsWadReader&& reader,
                                               bool dynamicC2N,
                                               const char* path,
                                               bool lockMemory);

TAutoPtr<IDocsAttrsData> CreateDocsAttrsData(); // creates empty docs attrs data


class IDocsAttrsDataWriter {
public:
    virtual ~IDocsAttrsDataWriter() {}

    // writes one particular attr categs for current document
    virtual void WriteAttr(ui32 attrNum, bool unique, const TCategSeries& categs) = 0;
    // switches on the next document (need to be called after all existing attributes
    // for the current document have been written
    virtual void NextDoc() = 0;
    // writes all attributes empty for the current document
    virtual void WriteEmpty() = 0;
    // closes data section
    virtual void CloseData() = 0;

    virtual void WriteConfig() = 0;
    virtual void WriteFormat() = 0;
    virtual void WriteVersion() = 0;
};

TAutoPtr<IDocsAttrsDataWriter> CreateDocsAttrsDataWriter(const TConfig& config,
                                                         TVersion version,
                                                         TFormat format,
                                                         const TString& tmpdir,
                                                         const TString& prefix,
                                                         TNamedChunkedDataWriter& writer);

TAutoPtr<IDocsAttrsDataWriter> CreateWadDocsAttrsDataWriter(const TConfig& config,
                                                            const TString& output);

TAutoPtr<IDocsAttrsDataWriter> CreateWadDocsAttrs64DataWriter(const TConfig& config,
                                                              const TString& output);

}
