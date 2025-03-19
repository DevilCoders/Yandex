#include "docsattrsdata.h"

#include "config.h"
#include "metainfos.h"

#include <library/cpp/pop_count/popcount.h>
#include <library/cpp/on_disk/chunks/chunked_helpers.h>

#include <kernel/externalrelev/relev.h>

#include <util/generic/algorithm.h>
#include <util/generic/cast.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/string/vector.h>
#include <util/system/tempfile.h>
#include <util/system/tls.h>
#include <util/string/builder.h>
#include <util/string/split.h>

namespace NGroupingAttrs {

namespace NBlock {
    const char* const Version = "Version";
    const char* const Format = "Format";
    const char* const Names = "Names";
    const char* const Types = "Types";
    const char* const Offsets = "Offsets";
    const char* const Lens = "Lens";
    const char* const Data = "Data";
    const char* const Unique = "Unique";
};

static const char* BAD_DATA_MSG = "bad groupattrs data: ";

template<typename T>
inline static bool CastBlockNoThrow(const TNamedChunkedDataReader &reader, const char *blockName, T *&out, size_t *len = nullptr) {
    if (!reader.HasBlock(blockName))
        return false;

    if (len)
        *len = reader.GetBlockLenByName(blockName) / sizeof(T);
    out = static_cast<T*>(reader.GetBlockByName(blockName));

    return true;
}

template<typename T>
inline static void CastBlockThrow(const TNamedChunkedDataReader &reader, const char *blockName, T *&out, size_t *len = nullptr) {
    if (!CastBlockNoThrow(reader, blockName, out, len))
        ythrow TBadDataException() << BAD_DATA_MSG << "no " << blockName << " block found";
}

typedef ui8  TLen;
typedef ui32 TLen2;
typedef ui32 TOffset;

class TDocsAttrsEmptyData : public IDocsAttrsData {
public:
    TDocsAttrsEmptyData()
        : Metainfos_(false, Config_) {}

    void NoGroupattrs() const {
        ythrow yexception() << "no groupattrs data exist";
    }

    ui32 DocCount() const override { return 0; }
    bool DocCategs(ui32, ui32, TCategSeries&, IRelevance*) const override {
        NoGroupattrs();
        return false;
    }
    void DocCategs(ui32, const ui32[], size_t, TCategSeries*[], IRelevance*) const override { NoGroupattrs(); }
    TVersion Version() const override { return 0; }
    TFormat Format() const override { return 0; }
    const TConfig& Config() const override { return Config_; }
    TConfig& MutableConfig() override { return Config_; }
    const TMetainfos& Metainfos() const override { return Metainfos_; }
    TMetainfos& MutableMetainfos() override { return Metainfos_; }
    TAutoPtr<IIterator> CreateIterator(ui32) const override {
        NoGroupattrs();
        return nullptr;
    }
    const void* GetRawDocumentData(ui32) const override {
        NoGroupattrs();
        return nullptr;
    }
    bool SetDocCateg(ui32, ui32, ui64) override {
        NoGroupattrs();
        return false;
    }

private:
    TConfig Config_;
    TMetainfos Metainfos_;
};

template <typename TReader>
class TDocsAttrsIndexBasedData : public IDocsAttrsData {
public:
    TDocsAttrsIndexBasedData(const TReader& reader, bool dynamicC2N, const char* path, bool lockMemory = false)
        : Version_(0)
        , Format_(0)
        , Metainfos_(dynamicC2N, Config_)
    {
        Parse(reader);
        Metainfos_.Init(path, lockMemory);
    }

    TVersion Version() const override {
        return Version_;
    }
    TFormat Format() const override {
        return Format_;
    }
    const TConfig& Config() const override {
        return Config_;
    }
    TConfig& MutableConfig() override {
        return Config_;
    }
    const TMetainfos& Metainfos() const override {
        return Metainfos_;
    }
    TMetainfos& MutableMetainfos() override {
        return Metainfos_;
    }

    const void* GetRawDocumentData(ui32) const override {
        ythrow yexception() << "This grouping attribute format does not support retrieving raw document data";
    }

private:
    void Parse(const TNamedChunkedDataReader& reader);
    void Parse(const TDocAttrsWadReader& reader);

protected:
    TVersion Version_;
    TFormat Format_;
    TConfig Config_;
    TMetainfos Metainfos_;
};

// Market search uses this attribute format as a primitive ERF for storing static features for documents.
// It should be eventually deleted and replaced with a proper ERF, but it doesn't look like it will happen anytime soon.
class TMarketAttrsDataIterator : public IIterator {
public:
    TMarketAttrsDataIterator(const TConfig& config, const i32* data, const ui32 realAttrCount)
        : Config_(config)
        , RealAttrCount_(realAttrCount)
        , Data_(data)
        , CurrentValue_()
        , CurrentValueWasRead_()
    {
    }

    void MoveToAttr(const char* attrname) override {
        MoveToAttr(Config_.AttrNum(attrname));
    }

    void MoveToAttr(ui32 attrnum) override {
        if (attrnum < RealAttrCount_) {
            CurrentValue_ = *(Data_ + attrnum);
            CurrentValueWasRead_ = false;
        }
    }

    bool NextValue(TCateg* result) override {
        if (CurrentValueWasRead_ || (CurrentValue_ == GROUP_ATTR_NO_VALUE)) {
            return false;
        }

        *result = CurrentValue_;
        CurrentValueWasRead_ = true;
        return true;
    }

private:
    const TConfig& Config_;
    const ui32 RealAttrCount_;
    const i32* Data_;
    i32 CurrentValue_;
    bool CurrentValueWasRead_;
};

class TMarketAttrsData : public IDocsAttrsData {
public:
    TMarketAttrsData(const TNamedChunkedDataReader& reader, bool dynamicC2N, const char* path)
        : Version_(0)
        , Format_(0)
        , Metainfos_(dynamicC2N, Config_)
        , Data_()
    {
        Parse(reader);
        Metainfos_.Init(path, /*lockMemory*/ false);
    }

    ui32 DocCount() const override {
        return DocCount_;
    }

    bool DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance* relevance) const override {
        const i32* docPtr = GetDocOffset(docid);
        if (attrnum < RealAttrCount_) {
            i32 value = *(docPtr + attrnum);
            if (value == GROUP_ATTR_NO_VALUE) {
                return false;
            }
            result.AddCateg(value);
            return true;
        }
        const ui32 virtualAttrIndex = attrnum - RealAttrCount_;
        if (virtualAttrIndex < MAX_VIRTUAL_GROUP_ATTRS && relevance) {
            TCateg value;
            if (relevance->CalculateVirtualGroupAttr(docid, virtualAttrIndex, value) && value != GROUP_ATTR_NO_VALUE) {
                result.AddCateg(value);
                return true;
            }
        }
        return false;
    }

    void DocCategs(ui32 docid, const ui32 attrnums[], size_t attrcount, TCategSeries* results[], IRelevance* externalRelevance) const override {
        for (size_t i = 0; i < attrcount; ++i) {
            TMarketAttrsData::DocCategs(docid, attrnums[i], *results[i], externalRelevance);
        }
    }

    TVersion Version() const override {
        return Version_;
    }
    TFormat Format() const override {
        return Format_;
    }

    const TConfig& Config() const override {
        return Config_;
    }

    TConfig& MutableConfig() override {
        return Config_;
    }

    const TMetainfos& Metainfos() const override {
        return Metainfos_;
    }

    TMetainfos& MutableMetainfos() override {
        return Metainfos_;
    }

    TAutoPtr<IIterator> CreateIterator(ui32 docid) const override {
        return new TMarketAttrsDataIterator(Config_, GetDocOffset(docid), RealAttrCount_);
    }

    const void* GetRawDocumentData(ui32 docid) const override {
        return GetDocOffset(docid);
    }

    bool SetDocCateg(ui32, ui32, ui64) override {
        ythrow yexception() << "Market grouping attribute format does not support changing attribute values";
    }

private:
    void Parse(const TNamedChunkedDataReader& reader) {
        Version_ = *(static_cast<const TVersion*>(reader.GetBlockByName(NBlock::Version)));

        const TFormat *format;
        CastBlockThrow(reader, NBlock::Format, format);
        Format_ = *format;

        const char* names;
        size_t nlen;
        CastBlockThrow(reader, NBlock::Names, names, &nlen);
        TVector<TString> vnames;
        StringSplitter(TString(names, nlen).data()).Split(' ').SkipEmpty().Collect(&vnames);
        vnames.reserve(vnames.size() + MAX_VIRTUAL_GROUP_ATTRS);

        TVector<TConfig::TPackedType> types;
        const TConfig::TPackedType* typesPtr;
        size_t tlen;
        CastBlockThrow(reader, NBlock::Types, typesPtr, &tlen);
        types.reserve(tlen + MAX_VIRTUAL_GROUP_ATTRS);
        for (size_t i = 0; i < tlen; ++i) {
            types.push_back(typesPtr[i]);
        }

        TVector<bool> unique;
        const bool* uniquePtr = nullptr;
        size_t uniqlen;
        CastBlockNoThrow(reader, NBlock::Unique, uniquePtr, &uniqlen);
        Y_ASSERT(!uniquePtr || uniqlen == tlen);
        if (uniquePtr) {
            unique.reserve(uniqlen + MAX_VIRTUAL_GROUP_ATTRS);
            for (size_t i = 0; i < uniqlen; ++i) {
                unique.push_back(uniquePtr[i]);
            }
        }

        for (const size_t i : xrange(MAX_VIRTUAL_GROUP_ATTRS)) {
            vnames.push_back(TStringBuilder() << "_virtual" << LeftPad(MAX_VIRTUAL_GROUP_ATTRS - i - 1, 2, '0'));
            types.push_back(TConfig::Type::I32);
            if (uniquePtr) {
                unique.push_back(false);
            }
        }
        Config_.Init(vnames, types.data(), vnames.size(), uniquePtr ? unique.data() : nullptr);
        RealAttrCount_ = tlen;
        size_t dataSize;
        CastBlockThrow(reader, NBlock::Data, Data_, &dataSize);
        DocCount_ = dataSize / RealAttrCount_;
    }


    const i32* GetDocOffset(ui32 docid) const {
        return Data_ + docid * RealAttrCount_;
    }

    TVersion Version_;
    TFormat Format_;
    TConfig Config_;
    TMetainfos Metainfos_;
    size_t RealAttrCount_;
    const i32* Data_;
    size_t DocCount_;
};

class TDocsAttrsPlainData : public TDocsAttrsIndexBasedData<TNamedChunkedDataReader> {
public:
    TDocsAttrsPlainData(const TNamedChunkedDataReader& reader, bool dynamicC2N, const char* path);
    ui32 DocCount() const override;
    inline bool DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance* externalRelevance) const override;
    void DocCategs(ui32 docid, const ui32 attrnums[], size_t attrcount, TCategSeries* results[], IRelevance* externalRelevance) const override;
    TAutoPtr<IIterator> CreateIterator(ui32 docId) const override;
    bool SetDocCateg(ui32 docid, ui32 attrnum, ui64 value) override;

private:
    bool InitOffsetLen(ui32 docid, ui32 attrnum, TOffset& offset, ui32& len) const;
    template<typename T>
    void ReadAttrs(TOffset offset, ui32 len, TCategSeries& result) const;
    ui32 ReadLen(TOffset offset) const;
    template<typename T>
    void WriteAttr(TOffset offset, ui32 len, T value);

private:
    size_t BlockLen_;
    const TOffset* Offsets_;
    size_t OffsetsSize_;
    const TLen* Lens_;
    size_t LensSize_;
    const char* Data_;
    size_t DataLen_;
    ui32 DocCount_;

    friend class TDocsAttrsPlainDataIterator;
};

template<typename TWadIo>
class TDocAttrsWadPreloader: public IDocsAttrsPreloader {
    using TDocAttrsWadIterator = typename TWadIo::TSearcher::TIterator;
    using TDocAttrsWadSearcher = typename TWadIo::TSearcher;

public:
    TDocAttrsWadPreloader(const TDocAttrsWadSearcher* searcher, TDocAttrsWadIterator* iterator)
        : Searcher_(searcher)
        , Iterator_(iterator)
    {
    }

    void AnnounceDocIds(TConstArrayRef<ui32> ids) override {
        Searcher_->AnnounceDocIds(ids, Iterator_);
    }

    void PreloadDoc(ui32 doc, NDoom::TSearchDocLoader* loader) override {
        Searcher_->PreloadDoc(doc, loader, Iterator_);
    }

    ~TDocAttrsWadPreloader() {
        Searcher_->ClearPreloadedDocs(Iterator_);
    }

private:
    const TDocAttrsWadSearcher* Searcher_ = nullptr;
    TDocAttrsWadIterator* Iterator_ = nullptr;
};

template<typename TWadIo>
class TDocsAttrsWadData : public TDocsAttrsIndexBasedData<TDocAttrsWadReader> {
    using TDocAttrsWadIterator = typename TWadIo::TSearcher::TIterator;
    using TDocAttrsWadSearcher = typename TWadIo::TSearcher;
    using TDocAttrsHit = typename TWadIo::TReader::THit;

public:
    TDocsAttrsWadData(TDocAttrsWadReader&& reader, NDoom::IChunkedWad* wad, const NDoom::IDocLumpMapper* mapper, bool dynamicC2N, const char* path, bool lockMemory)
        : TDocsAttrsIndexBasedData<TDocAttrsWadReader>(reader, dynamicC2N, path, lockMemory)
        , WadReader_(std::move(reader))
    {
        Searcher_.Reset(wad, mapper);
    }

    TDocsAttrsWadData(TDocAttrsWadReader&& reader, bool dynamicC2N, const char* path, bool lockMemory)
        : TDocsAttrsIndexBasedData<TDocAttrsWadReader>(reader, dynamicC2N, path, lockMemory)
        , WadReader_(std::move(reader))
    {
        Searcher_.Reset(WadReader_.Wad());
    }

    THolder<IDocsAttrsPreloader> MakePreLoader() const override {
        return MakeHolder<TDocAttrsWadPreloader<TWadIo>>(&Searcher_, Iterator_.GetPtr());
    }

    ui32 DocCount() const override {
        return Searcher_.Size();
    }

    inline bool DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance*) const override {
        Y_ASSERT(attrnum < Config_.AttrCount());
        if (!Searcher_.Find(docid, Iterator_.GetPtr())) {
            return false;
        }

        TDocAttrsHit outHit, hit(attrnum, 0);

        if (!Iterator_.Get().LowerBound(hit, &outHit)) {
            return false;
        }

        while (Iterator_.Get().ReadHit(&outHit) && outHit.AttrNum() == attrnum) {
            result.AddCateg(outHit.Categ());
        }

        return true;
    }

    void DocCategs(ui32 docid, const ui32* attrnums, size_t attrcount, TCategSeries** results, IRelevance* externalRelevance) const override {
        for (size_t i = 0; i < attrcount; ++i) {
            TDocsAttrsWadData::DocCategs(docid, attrnums[i], *results[i], externalRelevance);
        }
    }

    bool SetDocCateg(ui32, ui32, ui64) override {
        ythrow yexception() << "not needed for basesearch";
    }

    TAutoPtr<IIterator> CreateIterator(ui32) const override {
        ythrow yexception() << "not needed for basesearch";
    }

private:
    TDocAttrsWadReader WadReader_;
    Y_THREAD(TDocAttrsWadIterator) Iterator_;
    TDocAttrsWadSearcher Searcher_;
};

class TDocsAttrsPackedData : public TDocsAttrsIndexBasedData<TNamedChunkedDataReader> {
public:
    TDocsAttrsPackedData(const TNamedChunkedDataReader& reader, bool dynamicC2N, const char* path);
    ui32 DocCount() const override;
    inline bool DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance* externalRelevance) const override;
    void DocCategs(ui32 docid, const ui32 attrnums[], size_t attrcount, TCategSeries* results[], IRelevance* externalRelevance) const override;
    TAutoPtr<IIterator> CreateIterator(ui32 docid) const override;
    bool SetDocCateg(ui32 docid, ui32 attrnum, ui64 value) override;

    static inline size_t CalcIndicesSize(ui32 endDocId);

private:
    void ParseData(const TNamedChunkedDataReader& reader);

    template<typename TOffsets>
    bool DecodeDocAttrRange(ui32 docid, ui32 attrnum, const ui8*& base, size_t& datType, TOffsets& offsets) const;

private:
    typedef ui32 TIndicesItem;
    typedef ui8 TRawItem;
    typedef ui32 TDocId;
    struct TAttributeData {
        TDocId EndDocId;
        const TIndicesItem* Indices;
        const TRawItem* Raw;
    };
    typedef TVector<TAttributeData> TAttributesData;

    TAttributesData Data_;
    TDocId EndDocId_;

    friend class TDocsAttrsPackedDataIterator;
};

struct TTemp {
    TTempFileHandle Handle;
    TFixedBufferFileOutput File;

    TTemp(const TString& filename)
        : Handle(filename)
        , File(filename, 1 << 20)
    {
    }
};

THolder<IDocsAttrsData> CreateDocsAttrsData(
    NDoom::IChunkedWad* wad,
    const NDoom::IDocLumpMapper* mapper,
    TDocAttrsWadReader&& reader,
    bool dynamicC2N,
    const char* path,
    bool lockMemory,
    bool is64)
{
    if (is64) {
        return MakeHolder<TDocsAttrsWadData<NDoom::TOffroadDocAttrs64WadIo>>(std::move(reader), wad, mapper, dynamicC2N, path, lockMemory);
    } else {
        return MakeHolder<TDocsAttrsWadData<NDoom::TOffroadDocAttrsWadIo>>(std::move(reader), wad, mapper, dynamicC2N, path, lockMemory);
    }
}

TAutoPtr<IDocsAttrsData> CreateDocsAttrsData() {
    return new TDocsAttrsEmptyData();
}

TAutoPtr<IDocsAttrsData> CreateDocsAttrsData(TDocAttrsWadReader&& reader, bool dynamicC2N, const char* path, bool lockMemory) {
    return TAutoPtr<IDocsAttrsData>(new TDocsAttrsWadData<NDoom::TOffroadDocAttrsWadIo>(std::move(reader), dynamicC2N, path, lockMemory));
}

TAutoPtr<IDocsAttrsData> CreateDocsAttrs64Data(TDocAttrsWadReader&& reader, bool dynamicC2N, const char* path, bool lockMemory) {
    return TAutoPtr<IDocsAttrsData>(new TDocsAttrsWadData<NDoom::TOffroadDocAttrs64WadIo>(std::move(reader), dynamicC2N, path, lockMemory));
}

TAutoPtr<IDocsAttrsData> CreateDocsAttrsData(const TNamedChunkedDataReader& reader, bool dynamicC2N, const char* path) {
    const TVersion* version = nullptr;
    CastBlockThrow(reader, NBlock::Version, version);
    switch (*version) {
    case 0:
        return TAutoPtr<IDocsAttrsData>(new TMarketAttrsData(reader, dynamicC2N, path));
    case 1:    case 2:    case 3:
        return TAutoPtr<IDocsAttrsData>(new TDocsAttrsPlainData(reader, dynamicC2N, path));
    case 4:
        return TAutoPtr<IDocsAttrsData>(new TDocsAttrsPackedData(reader, dynamicC2N, path));
    default:
        ythrow TBadDataException() << BAD_DATA_MSG << "bad version number";
    }
}

class TDocsAttrsDataWriterBase: public IDocsAttrsDataWriter {
public:
    TDocsAttrsDataWriterBase(const TConfig& config,
                             TVersion version,
                             TFormat format,
                             TNamedChunkedDataWriter& writer
                             )
        : Config_(config)
        , Version_(version)
        , Format_(format)
        , Writer_(writer)
    {
    }

    void WriteConfig() override {
        Writer_.NewBlock(NBlock::Names);
        for (ui32 attrnum = 0; attrnum < Config_.AttrCount(); ++attrnum) {
            const char* name = Config_.AttrName(attrnum);
            Writer_.Write(name, strlen(name));
            if (attrnum != Config_.AttrCount() - 1) {
                Writer_.Write(' ');
            }
        }

        Writer_.NewBlock(NBlock::Types);
        for (ui32 attrnum = 0; attrnum < Config_.AttrCount(); ++attrnum) {
            Writer_.WriteBinary<TConfig::TPackedType>(static_cast<TConfig::TPackedType>(Config_.AttrType(attrnum)));
        }
        if (Version_ >= 3) {
            // Information is attributes unique or not.
            // If attribute is not unique same attr values will be collapsed by m2n
            Writer_.NewBlock(NBlock::Unique);
            for (ui32 attrnum = 0; attrnum < Config_.AttrCount(); ++attrnum)
                Writer_.WriteBinary<bool>(Config_.IsAttrUnique(attrnum));
        }
    }

    void WriteFormat() override {
        Writer_.NewBlock(NBlock::Format);
        Writer_.WriteBinary<TFormat>(Format_);
    }
    void WriteVersion() override {
        Writer_.NewBlock(NBlock::Version);
        Writer_.WriteBinary<TVersion>(Version_);
    }

protected:
    const TConfig& Config_;
    const TVersion Version_;
    const TFormat Format_;
    TNamedChunkedDataWriter& Writer_;
};

class TMarketAttrsDataWriter : public TDocsAttrsDataWriterBase {
public:
    TMarketAttrsDataWriter(const TConfig& config,
                              TVersion version,
                              TFormat format,
                              TNamedChunkedDataWriter& writer)
        : TDocsAttrsDataWriterBase(config, version, format, writer)
        , AttrCount_(config.AttrCount())
        , CurrentAttrNum_()
    {
        Writer_.NewBlock(NBlock::Data);
    }

    void NextDoc() override {
        FinishDoc();
    }

    void WriteAttr(ui32 attrNum, bool, const TCategSeries& categs) override {
        Y_VERIFY(attrNum >= CurrentAttrNum_, "Already wrote a value for attribute %d", attrNum);
        Y_VERIFY(categs.size() <= 1, "Market grouping attribute format can store no more than one category per attribute per document: %s", Config_.AttrName(attrNum));
        AdvanceToAttr(attrNum);
        Writer_.WriteBinary<i32>(categs.Empty() ? GROUP_ATTR_NO_VALUE : static_cast<i32>(*categs.Begin()));
        CurrentAttrNum_++;
    }

    void WriteEmpty() override {
    }

    void CloseData() override {
    }

private:
    void AdvanceToAttr(ui32 attrNum) {
        while (CurrentAttrNum_ < attrNum) {
            Writer_.WriteBinary<i32>(GROUP_ATTR_NO_VALUE);
            CurrentAttrNum_++;
        }
    }

    void FinishDoc() {
        AdvanceToAttr(AttrCount_);
        CurrentAttrNum_ = 0;
    }

    size_t AttrCount_;
    ui32 CurrentAttrNum_;
};

class TDocsAttrsPlainDataWriter : public TDocsAttrsDataWriterBase {
public:
    TDocsAttrsPlainDataWriter(const TConfig& config,
                              TVersion version,
                              TFormat format,
                              const TString& tmpdir,
                              const TString& prefix,
                              TNamedChunkedDataWriter& writer);

    void NextDoc() override;
    void WriteAttr(ui32 attrNum, bool unique, const TCategSeries& categs) override;
    void WriteEmpty() override;
    void CloseData() override;

private:
    void SaveLen(ui32 len);
    void WriteLen(TLen2 len);
    void WriteCateg(TCateg categ, TConfig::Type type);
    void WriteOffsets();
    void WriteLens();
    void WritePadding();

private:
    struct TAttrs {
        size_t Num;
        TCategSeries Categs;

        TAttrs();
        TAttrs(size_t num, const TCategSeries& categs);
        TAttrs(const TAttrs& other);
        bool operator < (const TAttrs& other) const;
    };

    struct TValue {
        TOffset Offset;
        TLen Len;

        TValue();
        TValue(TOffset offset, TLen len);
    };

    typedef TMap<TAttrs, TValue> TUsedAttrs;

private:
    TString OffsetsPath_;
    TString LensPath_;
    TTemp Offsets_;
    TTemp Lens_;
    TUsedAttrs Used_;
    size_t DataSize_;
    size_t BlockLen_;
};

class TDocsAttrsPackedDataWriter: public TDocsAttrsDataWriterBase {
public:
    TDocsAttrsPackedDataWriter(const TConfig& config,
                               TVersion version,
                               TFormat format,
                               const TString& tmpdir,
                               const TString& prefix,
                               TNamedChunkedDataWriter& writer);
    void NextDoc() override;
    void WriteAttr(ui32 attrNum, bool unique, const TCategSeries& categs) override;
    void WriteEmpty() override;
    void CloseData() override;

private:
    TString MakeIndicesTmpPath(ui32 attrNum);
    TString MakeRawTmpPath(ui32 attrNum);
    size_t MergeFile(const TString& filePath);
    void MergeData();
    size_t WritePadding(size_t cur);


private:
    class TSingleAttrWriter {
    public:
        TSingleAttrWriter();
        TSingleAttrWriter(const TSingleAttrWriter&);

        void Open(const TString& indicesPath, const TString& rawPath);
        void AddDoc(ui32 docId, const TCateg* begin, const TCateg* end);
        void Close();

        TSingleAttrWriter& operator=(const TSingleAttrWriter&);

        inline size_t GetRawSize() const;

    private:
        void Clean(ui32 docId = 0);

    private:
        ui32 Base_;
        ui32 Index_;
        size_t RawSize_;
        TMap<TVector<ui64>, std::pair<ui32, ui32> > LocalStore_;
        TVector<ui64> Offsets_;
        TVector<ui64> Data_;

        THolder<TTemp> Indices_;
        THolder<TTemp> Raw_;

        bool Used_;
    };

private:
    static const TString IndicesFileName;
    static const TString RawFileName;

    ui32 DocId_;
    const TString TmpDir_;
    const TString Prefix_;
    TVector<TSingleAttrWriter> TmpData_;
};

template<typename TWadIo>
class TDocsAttrsWadDataWriter : public IDocsAttrsDataWriter {
public:
    using TModel = typename TWadIo::TModel;
    using TWriter = typename TWadIo::TWriter;
    using THit = typename TWadIo::TWriter::THit;

    TDocsAttrsWadDataWriter(const TConfig& config,
                            const TString& output)
        : Config_(config)
        , Model_(NDoom::TStandardIoModelsStorage::Model<TModel>(TWadIo::DefaultModel))
    {
        Wad_ = MakeHolder<NDoom::TMegaWadWriter>(output);
        Writer_ = MakeHolder<TWriter>(Model_, Wad_.Get());
    }
    ~TDocsAttrsWadDataWriter() {
        Writer_->Finish();
        Wad_->Finish();
    }

    void WriteAttr(ui32 attrNum, bool /*unique*/, const TCategSeries& categs) override {
        TSet<TCateg> uniqCategs(categs.Begin(), categs.End());
        for (const auto categ : uniqCategs) {
            THit hit(attrNum, categ);
            Writer_->WriteHit(hit);
        }
    }
    void NextDoc() override {
        Writer_->WriteDoc(DocId_);
        ++DocId_;
    }
    void WriteEmpty() override {};
    void CloseData() override {};

    void WriteConfig() override {
        NDoom::TWadLumpId id(NDoom::EWadIndexType::DocAttrsIndexType, NDoom::EWadLumpRole::Struct);

        TString serializedConfig(Config_.ToString());
        Wad_->WriteGlobalLump(
                id,
                TArrayRef<const char>(serializedConfig.data(), serializedConfig.size()));
    }
    void WriteFormat() override {};
    void WriteVersion() override {};
private:
    const TConfig &Config_;
    ui32 DocId_ = 0;
    TModel Model_;
    THolder<NDoom::IWadWriter> Wad_;
    THolder<TWriter> Writer_;
};

TAutoPtr<IDocsAttrsDataWriter> CreateDocsAttrsDataWriter(const TConfig& config,
                                                         TVersion version,
                                                         TFormat format,
                                                         const TString& tmpdir,
                                                         const TString& prefix,
                                                         TNamedChunkedDataWriter& writer) {
    switch (version) {
    case 0:
        return new TMarketAttrsDataWriter(config, version, format, writer);

    case 1: case 2: case 3:
        return new TDocsAttrsPlainDataWriter(config, version, format, tmpdir, prefix, writer);
    case 4:
        return new TDocsAttrsPackedDataWriter(config, version, format, tmpdir, prefix, writer);
    default:
        Y_FAIL("Invalid usage");
        return nullptr;
    }
}

TAutoPtr<IDocsAttrsDataWriter> CreateWadDocsAttrsDataWriter(const TConfig& config,
                                                            const TString& output) {
    return new TDocsAttrsWadDataWriter<NDoom::TOffroadDocAttrsWadIo>(config, output + ".wad");
}

TAutoPtr<IDocsAttrsDataWriter> CreateWadDocsAttrs64DataWriter(const TConfig& config,
                                                            const TString& output) {
    return new TDocsAttrsWadDataWriter<NDoom::TOffroadDocAttrs64WadIo>(config, output + "64.wad");
}

// packs two 32-bit numbers into a 64-bit one, even 4-bit chunks are from the first number, odd ones - from the second one
static ui64 PackZipper(ui32 lo, ui32 hi) {
    ui64 ret = 0;
    ui64 lo64 = lo;
    ui64 hi64 = hi;
    for (size_t i = 0; i < 8; ++i) {
        ret += ((lo64 & 0xf) << (i * 8)) + ((hi64 & 0xf) << (i * 8 + 4));
        lo64 >>= 4;
        hi64 >>= 4;
    }
    return ret;
}

template<class TNumber>
static void UnpackZipper(TNumber source, size_t &lo, size_t &hi) {
    size_t loRet = 0;
    size_t hiRet = 0;
    for (size_t i = 0; i < sizeof(TNumber); ++i) {
        loRet += (source & 0xf) << (i * 4);
        source >>= 4;
        hiRet += (source & 0xf) << (i * 4);
        source >>= 4;
    }
    lo = loRet;
    hi = hiRet;
}

template<class X, class Y>
static X Conv(const Y &y) {
    X x;
    x = (X)y;
    Y_ASSERT(x == y);
    return x;
}

template<class TConvTo>
static void ConvAndCopy(const TVector<ui64> &data, TVector<ui8> &raw) {
    size_t oldSize = raw.size();
    size_t cpySize = data.size();
    raw.resize(oldSize + sizeof(TConvTo) * cpySize);
    TConvTo *dst = (TConvTo *)&raw[oldSize];
    for (size_t i = 0; i < cpySize; ++i) {
        dst[i] = Conv<TConvTo, ui64>(data[i]);
    }
}

static ui8 ConvAndCopy(const TVector<ui64> &data, TVector<ui8> &raw) {
    if (data.empty()) {
        return 0;
    }

    ui64 maxValue = *MaxElement(data.begin(), data.end());

    if ((maxValue >> 8) == 0) {
        ConvAndCopy<ui8>(data, raw);
        return 1;
    } else if ((maxValue >> 16) == 0) {
        ConvAndCopy<ui16>(data, raw);
        return 2;
    } else if ((maxValue >> 32) == 0) {
        ConvAndCopy<ui32>(data, raw);
        return 4;
    } else {
        ConvAndCopy<ui64>(data, raw);
        return 8;
    }
}

template<>
void TDocsAttrsIndexBasedData<TNamedChunkedDataReader>::Parse(const TNamedChunkedDataReader& reader) {
    Version_ = *(static_cast<const TVersion*>(reader.GetBlockByName(NBlock::Version)));

    const TFormat *format;
    CastBlockThrow(reader, NBlock::Format, format);
    Format_ = *format;

    const char* names;
    size_t nlen;
    CastBlockThrow(reader, NBlock::Names, names, &nlen);
    TVector<TString> vnames;
    StringSplitter(TString(names, nlen).data()).Split(' ').SkipEmpty().Collect(&vnames);

    const TConfig::TPackedType* types;
    size_t tlen;
    CastBlockThrow(reader, NBlock::Types, types, &tlen);

    const bool* unique = nullptr;
    size_t uniqlen;
    CastBlockNoThrow(reader, NBlock::Unique, unique, &uniqlen);
    Y_ASSERT(!unique || uniqlen == tlen);

    Config_.Init(vnames, types, tlen, unique);
}

template<>
void TDocsAttrsIndexBasedData<TDocAttrsWadReader>::Parse(const TDocAttrsWadReader& reader) {
    Version_ = 5; // Let it be version for indexaa.wad
    Config_ = {};
    Config_.InitFromStringWithTypes(reader.GetConfig());
}

TDocsAttrsPlainData::TDocsAttrsPlainData(const TNamedChunkedDataReader& reader, bool dynamicC2N, const char* path)
    : TDocsAttrsIndexBasedData<TNamedChunkedDataReader>(reader, dynamicC2N, path)
    , BlockLen_(Config_.AttrCount())
    , Offsets_(nullptr)
    , OffsetsSize_(0)
    , Lens_(nullptr)
    , LensSize_(0)
    , Data_(nullptr)
    , DataLen_(0)
    , DocCount_(0)
{
    if (!BlockLen_)
        return;

    if (reader.GetBlocksCount() != (2 == Version_ ? 6u : 7u))
        ythrow TBadDataException() << BAD_DATA_MSG << "bad blocks count number";

    CastBlockThrow(reader, NBlock::Offsets, Offsets_, &OffsetsSize_);

    if (1 == Version_)
        CastBlockThrow(reader, NBlock::Lens, Lens_, &LensSize_);

    CastBlockThrow(reader, NBlock::Data, Data_, &DataLen_);

    DocCount_ = OffsetsSize_ / Config_.AttrCount();
}

template<typename T>
void TDocsAttrsPlainData::ReadAttrs(TOffset offset, ui32 len, TCategSeries& result) const {
    TOffset offsetend = offset + len * sizeof(T);
    const T* begin = reinterpret_cast<const T*>(Data_ + offset);
    const T* end = reinterpret_cast<const T*>(Data_ + offsetend);

    while (begin < end) {
        result.AddCateg(static_cast<TCateg>(*begin));
        ++begin;
    }
}

ui32 TDocsAttrsPlainData::DocCount() const {
    return DocCount_;
}

inline bool TDocsAttrsPlainData::DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance*) const {
    TOffset offset;
    ui32 len;

    if (!InitOffsetLen(docid, attrnum, offset, len))
        return false;

    switch (Config_.AttrType(attrnum)) {
    case TConfig::I64:
        ReadAttrs<i64>(offset, len, result);
        break;
    case TConfig::I32:
        ReadAttrs<i32>(offset, len, result);
        break;
    case TConfig::I16:
        ReadAttrs<i16>(offset, len, result);
        break;
    default:
        ythrow TBadDataException() << BAD_DATA_MSG << "bad attr type";
    }

    return true;
}

void TDocsAttrsPlainData::DocCategs(ui32 docid, const ui32 attrnums[], size_t attrcount, TCategSeries* results[], IRelevance* externalRelevance) const {
    for (size_t i = 0; i < attrcount; ++i)
        TDocsAttrsPlainData::DocCategs(docid, attrnums[i], *results[i], externalRelevance);
}

ui32 TDocsAttrsPlainData::ReadLen(TOffset offset) const {
    const TLen2* len = reinterpret_cast<const TLen2*>(Data_ + offset);
    return *len;
}

bool TDocsAttrsPlainData::InitOffsetLen(ui32 docid, ui32 attrnum, TOffset& offset, ui32& len) const {
    Y_VERIFY(attrnum < Config_.AttrCount(), "Incorrect attrnum for InitOffsetLen method (%d)", attrnum);

    if (docid >= DocCount_) {
        return false;
    }

    size_t index = docid * BlockLen_ + attrnum;

    Y_ASSERT(index < OffsetsSize_);

    offset = Offsets_[index];

    len = 0;
    switch (Version_) {
    case 1:
        len = Lens_[index];
        break;
    case 2:
    case 3:
        len = ReadLen(offset);
        offset += sizeof(TLen2);
        break;
    default:
        ythrow TBadDataException() << BAD_DATA_MSG << "bad version";
    }

    if (len == 0) {
        return false;
    }

    return true;
}


template<typename T>
void TDocsAttrsPlainData::WriteAttr(TOffset offset, ui32 len, T value) {
    TOffset offsetend = offset + len * sizeof(T);
    T* begin = const_cast<T*>(reinterpret_cast<const T*>(Data_ + offset));
    T* end = const_cast<T*>(reinterpret_cast<const T*>(Data_ + offsetend));
    if (begin == end)
        ythrow yexception() << "Incorrect usage WriteAttr with no attribute";
    *begin = value;
}


bool TDocsAttrsPlainData::SetDocCateg(ui32 docid, ui32 attrnum, ui64 value) {
    TOffset offset;
    ui32 len;

    if (!InitOffsetLen(docid, attrnum, offset, len)) {
        return false;
    }

    switch (Config_.AttrType(attrnum)) {
    case TConfig::I64:
        WriteAttr<ui64>(offset, len, value);
        break;
    case TConfig::I32:
        WriteAttr<ui32>(offset, len, value);
        break;
    case TConfig::I16:
        WriteAttr<ui16>(offset, len, value);
        break;
    default:
        ythrow yexception() << "bad type";
    }

    return true;
}

class TDocsAttrsPlainDataIterator: public IIterator {
public:
    TDocsAttrsPlainDataIterator(const TDocsAttrsPlainData* owner, ui32 docid)
        : Owner_(owner)
        , DocId_(docid)
        , DocOffset_(0)
        , Begin_(nullptr)
        , Len_(0)
        , Type_(TConfig::BAD)
    {
        if (Y_UNLIKELY(!Owner_)) {
            DocOffset_ = 0;
            return;
        }

        Y_ASSERT(DocId_ <= Owner_->DocCount_);
        DocOffset_ = docid * Owner_->BlockLen_;
    }

    void MoveToAttr(const char* attrname) override {
        Y_ASSERT(Owner_);
        // Some hack to avoid basesearch crashing if attribute absent
        if (!Owner_->Config_.HasAttr(attrname))
            Len_ = 0; // Due NextValue(see below) return false at once
        else
            MoveToAttr(Owner_->Config_.AttrNum(attrname));
    }

    void MoveToAttr(ui32 attrnum) override {
        Y_ASSERT(Owner_);
        Y_ASSERT(attrnum != TConfig::NotFound && attrnum < Owner_->Config_.AttrCount());

        size_t index = DocOffset_ + attrnum;
        Y_ASSERT(index < Owner_->OffsetsSize_);

        Begin_ = Owner_->Data_ + Owner_->Offsets_[index];
        switch (Owner_->Version_) {
        case 1:
            Len_ = Owner_->Lens_[index];
            break;
        case 2:
        case 3:
            Len_ = *(reinterpret_cast<const TLen2*>(Begin_));
            Begin_ += sizeof(TLen2);
            break;
        default:
            ythrow TBadDataException() << BAD_DATA_MSG << "incorrect version";
        }

        Type_ = Owner_->Config_.AttrType(attrnum);
    }

    template<typename T>
    void ReadAttr(TCateg* result) {
        const T* ptr = reinterpret_cast<const T*>(Begin_);
        *result = static_cast<TCateg>(*ptr);
        Begin_ += sizeof(T);
        --Len_;
    }

    bool NextValue(TCateg* result) override {
        if (Len_ < 1) {
            return false;
        }

        switch (Type_) {
        case TConfig::I64:
            ReadAttr<i64>(result);
            break;
        case TConfig::I32:
            ReadAttr<i32>(result);
            break;
        case TConfig::I16:
            ReadAttr<i16>(result);
            break;
        default:
            ythrow TBadDataException() << BAD_DATA_MSG << "unknown type";
        }

        return true;
    }

private:
    const TDocsAttrsPlainData* Owner_;
    ui32 DocId_;
    size_t DocOffset_;
    const char* Begin_;
    size_t Len_;
    TConfig::Type Type_;
};

TAutoPtr<IIterator> TDocsAttrsPlainData::CreateIterator(ui32 docid) const {
    return new TDocsAttrsPlainDataIterator(this, docid);
}

TDocsAttrsPackedData::TDocsAttrsPackedData(const TNamedChunkedDataReader& reader, bool dynamicC2N, const char* path)
    : TDocsAttrsIndexBasedData<TNamedChunkedDataReader>(reader, dynamicC2N, path)
{
    if (reader.GetBlocksCount() != 7)
        ythrow TBadDataException() << BAD_DATA_MSG << "bad blocks count number";

    ParseData(reader);
}

ui32 TDocsAttrsPackedData::DocCount() const {
    return EndDocId_;
}

inline static void CheckBadOffset(const ui8 *cur, const ui8 *end) {
    if (cur > end)
        ythrow TBadDataException() << BAD_DATA_MSG
                                   << "offset points out of data";
}

size_t inline TDocsAttrsPackedData::CalcIndicesSize(ui32 endDocId) {
        return 2 * sizeof(TDocsAttrsPackedData::TIndicesItem) * (31 + endDocId >> 5);
}

void TDocsAttrsPackedData::ParseData(const TNamedChunkedDataReader& reader) {
    size_t attrCount = Config_.AttrCount();

    const TOffset *offsets = nullptr;
    size_t offsetsLen = 0;
    CastBlockThrow(reader, NBlock::Offsets, offsets, &offsetsLen);

    if (attrCount - 1 != offsetsLen)
        ythrow TBadDataException() << BAD_DATA_MSG << "length of "
                                   << NBlock::Offsets << " block doesn't match to attributes count";

    const ui8 *data = nullptr;
    size_t dataSize = 0;
    CastBlockThrow(reader, NBlock::Data, data, &dataSize);
    const ui8 *dataEnd = data + dataSize;

    Data_.resize(attrCount);

    EndDocId_ = 0;

    for (size_t i = 0; i < attrCount; ++i) {
        TAttributeData &attrData = Data_[i];

        const ui8 *cur = data + (i ? offsets[i-1] : 0);
        CheckBadOffset(cur, dataEnd);
        attrData.EndDocId = *reinterpret_cast<const TDocId*>(cur);
        EndDocId_ = Max(EndDocId_, attrData.EndDocId);

        cur += sizeof(TDocsAttrsPackedData::TDocId);
        CheckBadOffset(cur, dataEnd);
        attrData.Indices = reinterpret_cast<const TIndicesItem*>(cur);

        size_t indicesSize = CalcIndicesSize(attrData.EndDocId);
        cur += indicesSize;
        CheckBadOffset(cur, dataEnd);
        attrData.Raw = reinterpret_cast<const TRawItem*>(cur);
    }
}

template<class TProcessor>
static void inline ReadAndProcess(const ui8 *base, size_t index, size_t size, size_t length, TProcessor& proc) {
    if (size == 1) {
        return proc.Process(((const ui8  *)(base)) + index, length);
    }
    if (size == 2) {
        return proc.Process(((const ui16 *)(base)) + index, length);
    }
    if (size == 4) {
        return proc.Process(((const ui32 *)(base)) + index, length);
    }
    if (size == 8) {
        return proc.Process(((const ui64 *)(base)) + index, length);
    }
}

struct TGetCategs {
    TCategSeries& Out;
    template<class T>
    void Process(const T *data, size_t num) {
        for (size_t i = 0; i < num; ++i)
            Out.AddCateg(data[i]);
    }
};

struct TGetTwo {
    size_t Beg;
    size_t End;
    template<class T>
    void Process(const T *data, size_t) {
        UnpackZipper(data[0], Beg, End);
    }
};


template<typename TOffsets>
bool TDocsAttrsPackedData::DecodeDocAttrRange(ui32 docid, ui32 attrnum, const ui8*& base, size_t& datType, TOffsets& offsets) const {
    const TAttributeData& data = Data_[attrnum];
    if (docid >= data.EndDocId)
        return false;

    const size_t index = docid >> 5;
    const size_t subIndex = docid & 31;

    const ui32 mask = data.Indices[index * 2];
    const ui32 docMask = 1U << subIndex;

    if (mask & docMask) {
        const ui32 offset = data.Indices[index * 2 + 1];
        const ui8* off = data.Raw + offset;
        size_t typeOff = off[0];
        size_t offType = typeOff & 0xf;
        datType = typeOff >> 4;
        {
            ui8 offLength = off[1];
            off += 2;
            if (offLength == 1) {
                ReadAndProcess(off, 0, offType, 1, offsets);
                base = off + offType;
            } else {
                ReadAndProcess(off, PopCount<ui32>(mask & (docMask - 1)), offType, 1, offsets);
                base = off + offLength * offType;
            }
        }

        return true;
    }

    return false;
}

inline bool TDocsAttrsPackedData::DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance*) const {
    Y_ASSERT(attrnum < Config_.AttrCount());

    const ui8* base;
    size_t datType;
    TGetTwo offsets;

    if (!DecodeDocAttrRange(docid, attrnum, base, datType, offsets))
        return false;

    TGetCategs categs = { result };
    ReadAndProcess(base, offsets.Beg, datType, offsets.End - offsets.Beg, categs);

    return true;
}

void TDocsAttrsPackedData::DocCategs(ui32 docid, const ui32 attrnums[], size_t attrcount, TCategSeries* results[], IRelevance* externalRelevance) const {
    for (size_t i = 0; i < attrcount; ++i)
        TDocsAttrsPackedData::DocCategs(docid, attrnums[i], *results[i], externalRelevance);
}

class TDocsAttrsPackedDataIterator: public IIterator {
public:
    TDocsAttrsPackedDataIterator(const TDocsAttrsPackedData* owner, ui32 docid)
        : Owner_(owner)
        , DocId_(docid)
        , Current_(nullptr)
        , End_(nullptr)
        , DatType_(0)
    {
    }

    void MoveToAttr(const char* attrname) override {
        Y_ASSERT(Owner_);
        if (!Owner_->Config_.HasAttr(attrname))
            Current_ = End_ = nullptr;
        else
            MoveToAttr(Owner_->Config_.AttrNum(attrname));
    }

    void MoveToAttr(ui32 attrnum) override {
        TGetTwo offsets;
        if (Owner_->DecodeDocAttrRange(DocId_, attrnum, Current_, DatType_, offsets)) {
            End_ = Current_ + offsets.End * DatType_;
            Current_ += offsets.Beg * DatType_;
        } else {
            Current_ = End_ = nullptr;
        }
    }

    bool NextValue(TCateg* result) override;

private:
    template<typename T>
    inline void ReadAttr(TCateg* result);

private:
    const TDocsAttrsPackedData* Owner_;
    ui32 DocId_;
    const ui8* Current_;
    const ui8* End_;
    size_t DatType_;
};

template<typename T>
inline void TDocsAttrsPackedDataIterator::ReadAttr(TCateg* result) {
    *result = static_cast<TCateg>(*reinterpret_cast<const T*>(Current_));
    Current_ += sizeof(T);
}

bool TDocsAttrsPackedDataIterator::NextValue(TCateg* result) {
    if (Current_ == End_)
        return false;

    switch (DatType_) {
    case 1:
        ReadAttr<ui8>(result);
        break;
    case 2:
        ReadAttr<ui16>(result);
        break;
    case 4:
        ReadAttr<ui32>(result);
        break;
    case 8:
        ReadAttr<ui64>(result);
        break;
    default:
        ythrow TBadDataException() << BAD_DATA_MSG << "unknown type";
    }

    return true;
}

TAutoPtr<IIterator> TDocsAttrsPackedData::CreateIterator(ui32 docid) const {
    return new TDocsAttrsPackedDataIterator(this, docid);
}

bool TDocsAttrsPackedData::SetDocCateg(ui32, ui32, ui64) {
    ythrow yexception() << "writing doc attrs is not supported for packed format";
}

template <typename T>
void WriteTemp(IOutputStream& out, typename TTypeTraits<T>::TFuncParam t) {
    out.Write(&t, sizeof(t));
}

const TString OffsetsFilename = "aa.offsets.temp";
const TString LensFilename = "aa.lens.temp";

TDocsAttrsPlainDataWriter::TDocsAttrsPlainDataWriter(const TConfig& config,
                                                     TVersion version,
                                                     TFormat format,
                                                     const TString& tmpdir,
                                                     const TString& prefix,
                                                     TNamedChunkedDataWriter& writer)
    : TDocsAttrsDataWriterBase(config, version, format, writer)
    , OffsetsPath_(tmpdir + "/" + prefix + "." + OffsetsFilename)
    , LensPath_(tmpdir + "/" + prefix + "." + LensFilename)
    , Offsets_(OffsetsPath_)
    , Lens_(LensPath_)
    , DataSize_(0)
    , BlockLen_(config.AttrCount())
{
    Writer_.NewBlock(NBlock::Data);
}

void TDocsAttrsPlainDataWriter::NextDoc() {
    WritePadding();
}

void TDocsAttrsPlainDataWriter::WriteAttr(ui32 attrNum, bool unique, const TCategSeries& categs) {
    TAttrs attrs(attrNum, categs);
    TUsedAttrs::const_iterator found = unique ? Used_.end() : Used_.find(attrs);

    if (found == Used_.end()) {
        TValue value((TOffset)DataSize_, (TLen)categs.size());
        WriteTemp<TOffset>(Offsets_.File, value.Offset);
        SaveLen((ui32)categs.size());

        for (size_t i = 0; i < categs.size(); ++i) {
            WriteCateg(categs.GetCateg(i), Config_.AttrType(attrNum));
        }

        if (!unique)
            Used_[attrs] = value;
    } else {
        WriteTemp<TOffset>(Offsets_.File, found->second.Offset);
        if (Version_ == 1) {
            WriteTemp<TLen>(Lens_.File, found->second.Len);
        }
    }
}

void TDocsAttrsPlainDataWriter::WriteEmpty() {
    for (size_t i = 0; i < BlockLen_; ++i) {
        WriteTemp<TOffset>(Offsets_.File, (ui32)DataSize_);
        SaveLen(0);
    }
}

void TDocsAttrsPlainDataWriter::CloseData() {
    WriteOffsets();
    if (Version_ == 1) {
        WriteLens();
    }
}

void TDocsAttrsPlainDataWriter::SaveLen(ui32 len) {
    switch (Version_) {
        case 1:
            WriteTemp<TLen>(Lens_.File, static_cast<TLen>(len));
            break;
        case 2:
        case 3:
            WriteLen(len);
            break;
        default:
            Y_FAIL("Invalid usage");
    }
}

void TDocsAttrsPlainDataWriter::WriteCateg(TCateg categ, TConfig::Type type) {
    switch (type) {
    case TConfig::I16:
        Writer_.WriteBinary<i16>(static_cast<i16>(categ));
        DataSize_ += 2;
        break;
    case TConfig::I32:
        Writer_.WriteBinary<i32>(static_cast<i32>(categ));
        DataSize_ += 4;
        break;
    case TConfig::I64:
        Writer_.WriteBinary<i64>(static_cast<i64>(categ));
        DataSize_ += 8;
        break;
    default:
        ythrow yexception() << "bad type";
    }
}

void TDocsAttrsPlainDataWriter::WriteOffsets() {
    Offsets_.File.Finish();
    TFileInput input(OffsetsPath_, 1 << 20);

    Writer_.NewBlock(NBlock::Offsets);

    TOffset offset;
    while (input.Load(&offset, sizeof(offset)) == sizeof(offset)) {
        Writer_.WriteBinary<TOffset>(offset);
    }
}

void TDocsAttrsPlainDataWriter::WriteLens() {
    Lens_.File.Finish();
    TFileInput input(LensPath_, 1 << 20);

    Writer_.NewBlock(NBlock::Lens);

    TLen len;
    while (input.Load(&len, sizeof(len)) == sizeof(len)) {
        Writer_.WriteBinary<TLen>(len);
    }
}

void TDocsAttrsPlainDataWriter::WriteLen(TLen2 len) {
    Writer_.WriteBinary<TLen2>(len);
    DataSize_ += sizeof(TLen2);
}

void TDocsAttrsPlainDataWriter::WritePadding() {
    size_t size = (DataSize_ % 4 == 0 ? 0 : 4 - (DataSize_ % 4));
    for (size_t i = 0; i < size; ++i) {
        Writer_.WriteBinary<char>('\0');
        ++DataSize_;
    }
}

TDocsAttrsPlainDataWriter::TAttrs::TAttrs()
    : Num(0)
{
}

TDocsAttrsPlainDataWriter::TAttrs::TAttrs(size_t num, const TCategSeries& categs)
    : Num(num)
    , Categs(categs)
{
}

TDocsAttrsPlainDataWriter::TAttrs::TAttrs(const TAttrs& other) {
    Num = other.Num;
    Categs.Reset(other.Categs);
}

bool TDocsAttrsPlainDataWriter::TAttrs::operator< (const TDocsAttrsPlainDataWriter::TAttrs& other) const {
    if (Num != other.Num) {
        return Num < other.Num;
    }

    if (Categs.size() != other.Categs.size()) {
        return Categs.size() < other.Categs.size();
    }

    for (size_t i = 0; i < Categs.size(); ++i) {
        if (Categs.GetCateg(i) != other.Categs.GetCateg(i)) {
            return Categs.GetCateg(i) < other.Categs.GetCateg(i);
        }
    }

    return false;
}

TDocsAttrsPlainDataWriter::TValue::TValue()
    : Offset(0)
    , Len(0)
{
}

TDocsAttrsPlainDataWriter::TValue::TValue(TOffset offset, TLen len)
    : Offset(offset)
    , Len(len)
{
}

const TString TDocsAttrsPackedDataWriter::IndicesFileName = "aa.indices.temp";
const TString TDocsAttrsPackedDataWriter::RawFileName = "aa.raw.temp";


TDocsAttrsPackedDataWriter::TDocsAttrsPackedDataWriter(const TConfig& config,
                                                       TVersion version,
                                                       TFormat format,
                                                       const TString& tmpdir,
                                                       const TString& prefix,
                                                       TNamedChunkedDataWriter& writer)
    : TDocsAttrsDataWriterBase(config, version, format, writer)
    , DocId_(0)
    , TmpDir_(tmpdir)
    , Prefix_(prefix)
{
    if (version != 4) {
        Y_ASSERT(0);
        ythrow yexception() << "improper writer chosen for version " << version;
    }


    size_t attrCount = Config_.AttrCount();
    TmpData_.resize(attrCount);
    for (size_t i = 0; i < attrCount; ++i) {
        TmpData_[i].Open(MakeIndicesTmpPath(i), MakeRawTmpPath(i));
    }

    Writer_.NewBlock(NBlock::Data);
}

TString TDocsAttrsPackedDataWriter::MakeIndicesTmpPath(ui32 attrNum) {
    return TmpDir_ + "/" + Prefix_ + "." + IndicesFileName + "." + ToString(attrNum);
}

TString TDocsAttrsPackedDataWriter::MakeRawTmpPath(ui32 attrNum) {
    return TmpDir_ + "/" + Prefix_ + "." + RawFileName + "." + ToString(attrNum);
}

void TDocsAttrsPackedDataWriter::NextDoc() {
    ++DocId_;
}

void TDocsAttrsPackedDataWriter::WriteAttr(ui32 attrNum, bool, const TCategSeries& categs) {
    Y_ASSERT(attrNum < TmpData_.size());
    TmpData_[attrNum].AddDoc(DocId_, categs.Begin(), categs.End());
}

void TDocsAttrsPackedDataWriter::WriteEmpty() {
    for (size_t i = 0; i < TmpData_.size(); ++i) {
        TmpData_[i].AddDoc(DocId_, nullptr, nullptr);
    }
}

void TDocsAttrsPackedDataWriter::CloseData() {
    for (size_t i = 0; i < TmpData_.size(); ++i) {
        TmpData_[i].AddDoc(DocId_ + 31, nullptr, nullptr);
        TmpData_[i].Close();
    }
    MergeData();
    TmpData_.clear();
}

void TDocsAttrsPackedDataWriter::MergeData() {
    size_t written = 0;
    TVector<TOffset> offsets(TmpData_.size() - 1);
    for (size_t i = 0; i < TmpData_.size(); ++i) {
        if (i)
            offsets[i-1] = written;
        Writer_.WriteBinary<ui32>(DocId_);
        written += sizeof(ui32);

        const size_t indicesWritten = MergeFile(MakeIndicesTmpPath(i));
        Y_ASSERT(indicesWritten == TDocsAttrsPackedData::CalcIndicesSize(DocId_));
        written += indicesWritten;

        const size_t rawWritten = MergeFile(MakeRawTmpPath(i));
        Y_ASSERT(rawWritten == TmpData_[i].GetRawSize());
        written += rawWritten;

        written += WritePadding(written);
    }

    Writer_.NewBlock(NBlock::Offsets);
    Writer_.Write(static_cast<const void*>(&*offsets.begin()), sizeof(TOffset) * offsets.size());
}

size_t TDocsAttrsPackedDataWriter::WritePadding(size_t current) {
    static ui8 padding[] = { 0, 0, 0 };
    if (current %= 4)
        Writer_.Write(static_cast<void*>(padding), current);
    return current;
}

size_t TDocsAttrsPackedDataWriter::MergeFile(const TString& filePath) {
    TFileInput input(filePath, 1 << 20);
    return TransferData(static_cast<IInputStream*>(&input), static_cast<IOutputStream*>(&Writer_));
}

TDocsAttrsPackedDataWriter::TSingleAttrWriter::TSingleAttrWriter()
    : Base_(0)
    , Index_(0)
    , RawSize_(0)
    , Indices_(nullptr)
    , Raw_(nullptr)
    , Used_(false)
{
}

TDocsAttrsPackedDataWriter::TSingleAttrWriter::TSingleAttrWriter(const TDocsAttrsPackedDataWriter::TSingleAttrWriter& w)
    : Base_(0)
    , Index_(0)
    , RawSize_(0)
    , Indices_(nullptr)
    , Raw_(nullptr)
    , Used_(false)
{
    if (w.Used_) {
        // mustn't be called to copy not zero-state instance
        Y_ASSERT(0);
        ythrow yexception() << "unknown error: incorrect use of copying";
    }
}

TDocsAttrsPackedDataWriter::TSingleAttrWriter&
TDocsAttrsPackedDataWriter::TSingleAttrWriter::operator=(const TDocsAttrsPackedDataWriter::TSingleAttrWriter& w) {
    if (Used_ || w.Used_) {
        // mustn't be called to copy not zero-state instance
        Y_ASSERT(0);
        ythrow yexception() << "unknown error: incorrect use of copying";
    }
    return *this;
}

void TDocsAttrsPackedDataWriter::TSingleAttrWriter::Open(const TString& indicesPath, const TString& rawPath) {
    if (Used_) {
        Y_ASSERT(0);
        ythrow yexception() << "unknown error: cannot be re-used";
    }

    Used_ = true;

    Indices_.Reset(new TTemp(indicesPath));
    Raw_.Reset(new TTemp(rawPath));
}

void TDocsAttrsPackedDataWriter::TSingleAttrWriter::AddDoc(ui32 docId, const TCateg* beg, const TCateg* end) {
    if (docId / 32 != Base_ / 32) {
        WriteTemp<ui32>(Indices_->File, Index_);
        WriteTemp<ui32>(Indices_->File, Conv<ui32, size_t>(RawSize_));

        for (ui32 i = Base_ / 32; i + 1 < docId / 32; ++i) {
            WriteTemp<ui64>(Indices_->File, 0);
        }

        if (LocalStore_.size() == 1) {
            Offsets_.resize(1);
        }

        if (Index_) {
            TVector<ui8> raw;
            ui8 offType = ConvAndCopy(Offsets_, raw);
            ui8 datType = ConvAndCopy(Data_, raw);

            WriteTemp<ui8>(Raw_->File, offType + (datType << 4));
            WriteTemp<ui8>(Raw_->File, Conv<ui8, size_t>(Offsets_.size()));

            Raw_->File.Write(&*raw.begin(), raw.size());
            RawSize_ += 2 * sizeof(ui8) + raw.size();
        }

        Clean(docId);
    }

    if (beg == end) {
        return;
    }

    ui32 mask = 1U << (docId & 31);

    if (Index_ & mask) {
        return;
    }

    Index_ |= mask;
    size_t size = end - beg;
    TVector<ui64> localData(size);
    for (size_t i = 0; i < size; ++i)
        localData[i] = beg[i];

    std::pair<ui32,ui32>& key = LocalStore_[localData];
    if (key.first == 0 && key.second == 0) {
        size_t offset = Data_.size();
        key.first = Conv<ui32, size_t>(offset);
        Data_.resize(offset + size);
        for (size_t i = 0; i < size; ++i)
            Data_[offset + i] = beg[i];
        key.second = Conv<ui32, size_t>(Data_.size());
    }

    Offsets_.push_back(PackZipper(key.first, key.second));
}

void TDocsAttrsPackedDataWriter::TSingleAttrWriter::Clean(ui32 docId) {
    Data_.clear();
    Offsets_.clear();
    LocalStore_.clear();
    Base_ = docId;
    Index_ = 0;
}

void TDocsAttrsPackedDataWriter::TSingleAttrWriter::Close() {
    if (Indices_.Get() == nullptr || Raw_.Get() == nullptr) {
        Y_ASSERT(0);
        ythrow yexception() << "docattrs tmpfile isn't opened";
    }

    Indices_->File.Finish();
    Raw_->File.Finish();
}

inline size_t TDocsAttrsPackedDataWriter::TSingleAttrWriter::GetRawSize() const {
    return RawSize_;
}

}
