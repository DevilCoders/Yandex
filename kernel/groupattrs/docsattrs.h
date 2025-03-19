#pragma once

#include "attrmap.h"
#include "config.h"
#include "docattrs.h"
#include "docsattrsdata.h"
#include "iterator.h"
#include "metainfos.h"

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>
#include <util/system/defaults.h>
#include <util/system/filemap.h>

#include <kernel/searchlog/errorlog.h>

class TNamedChunkedDataReader;
class TCategSeries;

namespace NGroupingAttrs {

enum EMode {
    CommonMode,
    RealtimeMode
};

class TDocAttrs;

class TDocsAttrs : TNonCopyable {
public:
    explicit TDocsAttrs(EMode mode, bool lockMemory = false);
    explicit TDocsAttrs(bool dynamicC2N, const char* indexName, bool lockMemory = false, bool readOnly = true, bool ignoreNoAttrs = false);
    explicit TDocsAttrs(bool dynamicC2N, const TMemoryMap& mapping, bool lockMemory);
    explicit TDocsAttrs(IDocsAttrsData* data);

    TDocsAttrs(bool dynamicC2N, const char* indexName, NDoom::IChunkedWad* wad, const NDoom::IDocLumpMapper* mapper, bool index64, bool lockMemory = false);

    virtual ~TDocsAttrs() {}

    bool InitCommonMode(bool dynamicC2N, const char* indexName, bool readOnly = true);
    bool InitCommonMode(bool dynamicC2N, const TMemoryMap& mapping); // always read-only

    void InitRealtimeMode(const TString& configDir, size_t size);

    void Clear();

    THolder<IDocsAttrsPreloader> MakeDocsPreloader() const {
        return Data_->MakePreLoader();
    }

    template <class T>
    bool SetDocCateg(ui32 docid, ui32 attrnum, T value) {
        Y_VERIFY(Mode_ == CommonMode, "Incorrect usage SetDocCateg");

        if (!Config().IsAttrUnique(attrnum))
            ythrow yexception() << "only unique attributes can be reset";

        Y_ASSERT(!!Data_);
        return Data_->SetDocCateg(docid, attrnum, value);
    }

    inline void DocAttrs(ui32 docid, TDocAttrs* docattrs, IRelevance* externalRelevance = nullptr) const {
        Y_ASSERT(docattrs);
        Y_ASSERT(Config() == docattrs->Config());

        docattrs->Clear();

        for (ui32 attrnum = 0; attrnum < Config().AttrCount(); ++attrnum) {
            DocCategs(docid, attrnum, docattrs->MutableAttrs(attrnum), externalRelevance);
        }
    }

    inline bool DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance* externalRelevance = nullptr) const {
        Y_ASSERT(!!Data_);
        result.Clear();
        return Data_->DocCategs(docid, attrnum, result, externalRelevance);
    }

    inline bool DocCategs(ui32 docid, const char* attrname, TCategSeries& result, IRelevance* externalRelevance = nullptr) const {
        ui32 attrNum = Config().AttrNum(attrname);
        if (attrNum != TConfig::NotFound)
            return DocCategs(docid, attrNum, result, externalRelevance);
        else
            return false;
    }

    inline void DocCategs(ui32 docid, const ui32 attrnums[], size_t attrcount, TCategSeries* results[], IRelevance* externalRelevance = nullptr) const {
        Y_ASSERT(!!Data_);
        for (size_t i = 0; i < attrcount; results[i++]->Clear());
        Data_->DocCategs(docid, attrnums, attrcount, results, externalRelevance);
    }

    inline void DocCategs(ui32 docid, const char* attrnames[], size_t attrcount, TCategSeries* results[], IRelevance* externalRelevance = nullptr) const {
        TVector<ui32> attrnums(attrcount);
        for (size_t i = 0; i < attrcount; ++i)
            attrnums[i] = Config().AttrNum(attrnames[i]);

        DocCategs(docid, &*attrnums.begin(), attrcount, results, externalRelevance);
    }

    inline bool DocCateg(ui32 docid, ui32 attrnum, TCateg& result, IRelevance* externalRelevance = nullptr) const {
        TCategSeries categs;
        if (!DocCategs(docid, attrnum, categs, externalRelevance))
            return false;

        Y_ASSERT(categs.size() <= 1);

        if (categs.size() > 0) {
            result = categs.GetCateg(0);
            return true;
        }

        return false;
    }

    inline bool DocCateg(ui32 docid, const char* attrname, TCateg& result, IRelevance* externalRelevance = nullptr) const {
        return DocCateg(docid, Config().AttrNum(attrname), result, externalRelevance);
    }

    inline const TConfig& Config() const {
        Y_ASSERT(!!Config_);
        return *Config_;
    }

    inline const TMetainfos& Metainfos() const {
        Y_ASSERT(!!Metainfos_);
        return *Metainfos_;
    }

    inline TVersion Version() const {
        Y_ASSERT(!!Data_);
        return Data_->Version();
    }

    inline TFormat Format() const {
        Y_ASSERT(!!Data_);
        return Data_->Format();
    }

    inline size_t DocCount() const {
        Y_ASSERT(!!Data_);
        return Data_->DocCount();
    }

    inline TAutoPtr<IIterator> CreateIterator(ui32 docid) const {
        Y_ASSERT(!!Data_);
        return Data_->CreateIterator(docid);
    }

    // specific only for Realtime mode
    inline TMemoryAttrMap* RealtimePtr() {
        Y_ASSERT(RealtimeMode == Mode_);
        return Realtime_;
    }

    // specific only for Realtime mode
    inline void GetRTFormatDocAttrs(ui32 docid, TDocAttrs* docattrs) const {
        Y_ASSERT(docattrs);
        Y_ASSERT(Config() == docattrs->Config());

        docattrs->Clear();
        for (ui32 attrnum = 0; attrnum < Config().AttrCount(); ++attrnum) {
            Realtime_->GetDocCategs(docid, attrnum, &docattrs->MutableAttrs(attrnum));
        }
    }

    inline const void* GetRawDocumentData(ui32 docid) const {
        Y_ASSERT(!!Data_);
        return Data_->GetRawDocumentData(docid);
    }

    static bool IndexExists(const TString& indexName);
    static bool IndexWadExists(const TString& indexName);
    static bool IndexWad64Exists(const TString& indexName);
    static TString GetIndexFileName(const TString& indexName);
    static TString GetWadIndexFileName(const TString& indexName);
    static TString GetWad64IndexFileName(const TString& indexName);
    static const TString& GetFilenameSuffix();

private:
    bool DoInit(bool dynamicC2N);
    bool DoWadInit(bool dynamicC2N, const char* indexName);
    bool DoWad64Init(bool dynamicC2N, const char* indexName);
    void StoreConfigAndMetainfos();

    virtual TAutoPtr<IDocsAttrsData> CreateEmptyDocsAttrsImpl();

    virtual TAutoPtr<IDocsAttrsData> CreateDocsAttrsImpl(const TNamedChunkedDataReader& reader,
                                                         bool dynamicC2N, const char* path);

    virtual TAutoPtr<IDocsAttrsData> CreateDocsAttrsImpl(TDocAttrsWadReader&& reader,
                                                 bool dynamicC2N, const char* path);

    virtual TAutoPtr<IDocsAttrsData> CreateDocsAttrs64Impl(TDocAttrsWadReader&& reader,
                                                 bool dynamicC2N, const char* path);

public:
    static const TString FilenameSuffix;

private:
    const EMode Mode_;
    TBlob File_;
    THolder<TMemoryMap> FileMap_;
    THolder<IDocsAttrsData> Data_;
    TMemoryAttrMap* Realtime_;
    const TConfig* Config_;
    const TMetainfos* Metainfos_;
    bool LockMemory_;
};

}

