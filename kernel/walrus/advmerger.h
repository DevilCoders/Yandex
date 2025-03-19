#pragma once

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

#include <atomic>

#include <kernel/keyinv/indexfile/indexfile.h>
#include <kernel/keyinv/indexfile/indexwriter.h>
#include <kernel/keyinv/indexfile/indexstoragefactory.h>
#include <kernel/keyinv/indexfile/memoryportion.h>
#include <kernel/keyinv/indexfile/rdkeyit.h>

#include "deletelogic.h"
#include "finalremaptable.h"
#include "lfproc.h"
#include "sentlen.h"

//! input buffers of a memory portion
struct TPortionBuffers {
    const void* KeyBuf;
    size_t KeyBufSize;
    const void* InvBuf;
    size_t InvBufSize;

    TPortionBuffers(const void* keyBuf, size_t keyBufSize, const void* invBuf, size_t invBufSize)
        : KeyBuf(keyBuf)
        , KeyBufSize(keyBufSize)
        , InvBuf(invBuf)
        , InvBufSize(invBufSize)
    {
    }
    TPortionBuffers(const NIndexerCore::TMemoryPortion* portion)
        : KeyBuf(portion->GetKeyBuffer().Data())
        , KeyBufSize(portion->GetKeyBuffer().Size())
        , InvBuf(portion->GetInvBuffer().Data())
        , InvBufSize(portion->GetInvBuffer().Size())
    {
    }
};

/*
 * Task class that decribes the input and output indices and possible conversions while merging
 */
struct TAdvancedMergeTask {

    /*
     * input index for the merge
     */
    struct TMergeInput {
        const TString    FilePrefix;             // common prefix of the key, inv files, empty FilePrefix means memory portion
        const TPortionBuffers PortionBuffers;
        IYndexStorage::FORMAT Format;
        ui32            Version;
        bool            HasSubIndex;
        TVector<ui32>   RemapTable;             // monotonic remap: docid -> global docid; (ui32)-1 means deleted document
        bool            NeedsRemap;             // indicates whether remap will be used
        bool            UseDeleteLogic;         // indicates whether to use "Delete Logic" for this input index (apply ExcludedKeys and DeleteZero)

        explicit TMergeInput(const TString& filePrefix, IYndexStorage::FORMAT format = IYndexStorage::FINAL_FORMAT, bool needsRemap = false, bool useDeleteLogic = false)
            : FilePrefix(filePrefix)
            , PortionBuffers(nullptr, 0, nullptr, 0)
            , Format(format)
            , Version(YNDEX_VERSION_FINAL_DEFAULT)
            , HasSubIndex(true)
            , NeedsRemap(needsRemap)
            , UseDeleteLogic(useDeleteLogic)
        {
        }

        TMergeInput(const TPortionBuffers& portionBuffers, IYndexStorage::FORMAT format = IYndexStorage::PORTION_FORMAT, bool needsRemap = false, bool useDeleteLogic = false)
            : PortionBuffers(portionBuffers)
            , Format(format)
            , Version(YNDEX_VERSION_PORTION_DEFAULT)
            , HasSubIndex(false)
            , NeedsRemap(needsRemap)
            , UseDeleteLogic(useDeleteLogic)
        {
        }
    };

    /*
     * output index for the merge
     * @note merge output always has FINAL_FORMAT
     */
    struct TMergeOutput {
        const TString    FilePrefix;             // common prefix of the key, inv files
        ui32            Version;
        bool            HasSubIndex;

        explicit TMergeOutput(const TString& filePrefix, ui32 version = YNDEX_VERSION_FINAL_DEFAULT, bool hasSubIndex = true)
            : FilePrefix(filePrefix)
            , Version(version)
            , HasSubIndex(hasSubIndex)
        {
        }
    };

    TVector<TMergeInput>    Inputs;             // input indices
    TVector<TMergeOutput>   Outputs;            // output indices
    TFinalRemapTable        FinalRemapTable;

    const char**            ExcludedKeys = nullptr;     // keys that will be excluded in "Delete Logic"
    bool                    InvertExcludedKeys = false; // invert delete logic - filter all keys that not in "ExcludedKeys"
    DeleteZeroSentencesEnum DeleteZero = DZ_NONE;       // policy about what to do with zero sentences in "Delete Logic"
    bool                    SortHits = true;            // hits must be sorted, for example if doc IDs are chagned
    bool                    UniqueHits = false;         // hits must be unique
    bool                    StripKeys = false;          // keys will be stored in stripped format, see ConvertKeyToStrippedFormat.
    bool                    UseDirectOutput = false;
    bool                    UseDirectInput = false;

    TString                  TmpFileDir;                 // directory for temporary files
};

class YxFileWBL;
class THitIteratorHeap;

class IExternalHitWriter {
public:
    virtual ~IExternalHitWriter() {}
    virtual void StartKey(const char* key) = 0;
    virtual void FinishKey() = 0;
    virtual void AddHit(ui32 indexSrc, ui32 indexDest, const TSimpleSharedPtr<YxFileWBL>& output, const TWordPosition& hit) = 0;
};

inline void AddDocHit(TVector<ui32>* docHits, TWordPosition wp) {
    if (docHits) {
        if (wp.Doc() >= docHits->size())
            docHits->resize(3*wp.Doc()/2 + 1);
        (*docHits)[wp.Doc()]++;
//        if (wp.Doc() == 345717)
//            fprintf(stderr, "hit: %i.%i.%i\n", wp.Break(), wp.Word(), wp.GetRelevLevel());
    }
}

class TInvKeyWriterHitCount : public NIndexerCore::TInvKeyWriter {
public:
    TVector<ui32>* DocHitCounts;

public:
    TInvKeyWriterHitCount(NIndexerCore::TOutputIndexFile& file, bool hasSubIndex, TVector<ui32>* docHitCounts)
        : NIndexerCore::TInvKeyWriter(file, hasSubIndex)
        , DocHitCounts(docHitCounts)
    {
    }

    void WriteHit(SUPERLONG hit) {
        NIndexerCore::TInvKeyWriter::WriteHit(hit);
        AddDocHit(DocHitCounts, hit);
    }
};

/*
 * index merger class
 */
class TAdvancedIndexMerger {
private:
    typedef TReadKeysIteratorImpl< TAdvancedHitIterator<TBufferedHitIterator> >     TIndexIterator;
    typedef THeapOfHitIteratorsImpl< TAdvancedHitIterator<TBufferedHitIterator> >   TInputHeap;
    typedef TVector< TSimpleSharedPtr<TInvKeyWriterHitCount> > TIndexWriters;
    typedef TVector< TAutoPtr<char, TDeleteArray> >         TSorterBuffers;
    typedef TVector<TVector<ui32> >                         TFormRemap;
    typedef TVector<TVector<NIndexerCore::TOutputKey> >     TOutputKeys;

    struct TIndexFileInfo {
        const TString KeyName;
        const TString InvName;
        const TAdvancedMergeTask::TMergeInput* const MergeInput;

        TIndexFileInfo(const TAdvancedMergeTask::TMergeInput* mergeInput)
            : MergeInput(mergeInput)
        {
        }
        TIndexFileInfo(const TString& keyName, const TString& invName, const TAdvancedMergeTask::TMergeInput* mergeInput)
            : KeyName(keyName)
            , InvName(invName)
            , MergeInput(mergeInput)
        {
        }
    };

    struct TInputIndices {
        TIndexIterator* Array;
        size_t Count;

        TInputIndices(TIndexIterator* arr, size_t count)
            : Array(arr)
            , Count(count) {
        }
        ~TInputIndices() {
            if (Array)
                DestroyIndexIterators(*this);
        }
    };
    struct TCombinedKeyIterators;

    class TFormCollisionResolver : private TNonCopyable {
        TIndexWriters& Writers;
        struct THitCacheItem {
            SUPERLONG PrevHit;
            ui32 HitCount;
            TVector<ui32> Forms;
        };
        typedef TVector<THitCacheItem> THitCache;
        THitCache HitCache;
    public:
        explicit TFormCollisionResolver(TIndexWriters& writers)
            : Writers(writers)
            , HitCache(writers.size())
        {
        }
        void StoreHit(SUPERLONG hit, ui32 form, ui32 dst);
        void StorePrevHit(ui32 dst);
        void Reset();
    private:
        void SaveFormPosCollision(ui32 dst);
    };

    class TInternalMerging : private TNonCopyable {
        THitIteratorHeap& InputHeap;
        TIndexWriters& Writers;
        TAttrDeleteLogic& DeleteLogic;
        const TFinalRemapTable& FinalRemapTable;

        TVector<const char*> CurKeys;
        TFormCollisionResolver Resolver;

    public:
        TInternalMerging(THitIteratorHeap& inputHeap, TIndexWriters& writers, TAttrDeleteLogic& deleteLogic, const TFinalRemapTable& finalRemapTable)
            : InputHeap(inputHeap)
            , Writers(writers)
            , DeleteLogic(deleteLogic)
            , FinalRemapTable(finalRemapTable)
            , Resolver(writers)
        {
        }

        template<class SentWriter>
        void Merge(const TOutputKeys& outputKeys, size_t maxKeys, TFormRemap& formRemap, bool haveForms, SentWriter& sentWriter);
    };

    THolder<IHitsBufferManager>     BufferManager;                      // manages buffer for the hit iterators

    TVector<TIndexFileInfo>         InputFiles;
    static const ui32               SorterBuffersSize = 100 * 1024 * 1024; //size of one buffer (usaly we have 2 buffers (one for each output))
    static const ui32               ExternalMergeLimit = 50;            //limit of keys allowed for internal merge (if internal merge allowed at all)

    static const size_t             OutputBufferSize = 1024 * 1024;     // needed for MakeFastAccess

    TSimpleSharedPtr<const TAdvancedMergeTask>  Task;                               // task object, see above

    TAttrDeleteLogic                DeleteLogic;                        // delete logic, can delete keys and positions
    bool                            RawKeys;                            // keys have no forms
    bool                            UseRemapTables;
    bool                            UseDeleteLogic;
    bool                            HasMemoryPortions;
    bool                            UseDirectOutput;
    const std::atomic<bool>*        StopFlag;
    IExternalHitWriter*             ExternalHitWriter;

private:
    void CountWordForms(TFormCounts& formCounts, THitIteratorHeap& inputHeap);
    void ConstructKeys(TFormCounts& formCounts, TFormRemap& formRemap, TOutputKeys& outputKeys, NIndexerCore::TLemmaAndFormsProcessor& lemforms);
    void ConstructEmptyKeys(TOutputKeys& outputKeys, NIndexerCore::TLemmaAndFormsProcessor& lemforms);
    bool GetDstDocId(ui32 srcCluster, ui32 srcDocId, ui32& dstCluster, ui32& dstDocId);
    void InternalMerge(THitIteratorHeap& inputHeap, TIndexWriters& writers, const TOutputKeys& outputKeys, size_t maxKeys, TFormRemap& formRemap, bool haveForms);
    template <typename TKeyHit, class SentWriter> //template used for significant optimization of single key persts
    void ExternalMerge(THitIteratorHeap& inputHeap, TIndexWriters& writers, TSorterBuffers& sorterBuffers, TOutputKeys& outputKeys, TFormRemap& formRemap, bool haveForms, SentWriter& sentWriter);

    void CreateOutputs(TVector< TSimpleSharedPtr<YxFileWBL> >& outputs, size_t bufSize = 0x100000);

    static TInputIndices CreateIndexIterators(const TVector<TIndexFileInfo>& inputFiles, IHitsBufferManager* bufferManager, bool directInput);
    static void DestroyIndexIterators(TInputIndices& inputIndices);

    template<class SentWriter>
    bool Merge(SentWriter& sentWriter, TVector<ui32>* docHits = nullptr, size_t bufSize = 0x100000);
    bool MergeNoForms();
    template<class SentWriter>
    bool MergeNoDocIdChanges(SentWriter&, TVector<ui32>* docHits = nullptr, size_t bufSize = 0x100000);
    bool MergeMemoryPortions();

    void TryAddMergerInput(const TAdvancedMergeTask::TMergeInput& mergeInput, const TString& keySuffix, const TString& invSuffix);

public:
    TAdvancedIndexMerger(TSimpleSharedPtr<const TAdvancedMergeTask> task, const std::atomic<bool>* stopFlag = nullptr, bool rawKeys = false);
    void SetExternalHitWriter(IExternalHitWriter* externalHitWriter);
    bool Run(const char* sentlen = nullptr, TVector<ui32>* docHits = nullptr, size_t bufSize = 0x100000);
    template<class SentWriter>
    bool Run(SentWriter&, TVector<ui32>* docHits = nullptr, size_t bufSize = 0x100000);
};

void MergeTempAttributes(ui32 maxDocId, const TString& oldIndexPrefix, const TString& reindexedIndexPrefix, const TString& resultPrefix, const TString& tempFileDir);

void AddInputPortions(TAdvancedMergeTask& task, const TVector<TString>& names, IYndexStorage::FORMAT format, ui32 version = YNDEX_VERSION_PORTION_DEFAULT);
inline void AddInputPortions(TAdvancedMergeTask& task, const TVector<TString>& names, ui32 version = YNDEX_VERSION_PORTION_DEFAULT) {
    AddInputPortions(task, names, IYndexStorage::PORTION_FORMAT, version);
}
void AddInputPortions(TAdvancedMergeTask& task, const TVector<TPortionBuffers>& buffers,
    IYndexStorage::FORMAT format = IYndexStorage::PORTION_FORMAT, ui32 version = YNDEX_VERSION_PORTION_DEFAULT);
namespace NIndexerCore {
    class TMemoryPortion;
}
void AddInputPortions(TAdvancedMergeTask& task, const TVector<const NIndexerCore::TMemoryPortion*>& portions,
    IYndexStorage::FORMAT format = IYndexStorage::PORTION_FORMAT, ui32 version = YNDEX_VERSION_PORTION_DEFAULT);

//! @param sortHits     false by default because there is no remap table and hits are usually sorted
void MergePortions(NIndexerCore::TIndexStorageFactory& storageFactory, ui32 outputVersion = YNDEX_VERSION_FINAL_DEFAULT, bool outputHasSubIndex = true,
                   bool sortHits = true, const TVector<ui32>& remap = TVector<ui32>());

//! @param input                input portions, requirement: key contains one form for lemma
//! @param format               format of input portions
//! @param remap                docIDs for portions, it also can be NULL
//! @param intersectedRanges    if ranges of docIds for different portions are intersected then
//!                             merging algorithm is not so fast
void MergeMemoryPortions(const TPortionBuffers* input, size_t count, IYndexStorage::FORMAT format,
    const ui32* remap, bool intersectedRanges, NIndexerCore::TMemoryPortion& output);
void MergeMemoryPortions(const NIndexerCore::TMemoryPortion** input, size_t count, IYndexStorage::FORMAT format,
    const ui32* remap, bool intersectedRanges, NIndexerCore::TMemoryPortion& output);

class TMemoryHitIterator : public THitsHeapFields, private TNonCopyable {
    typedef DecoderFallBack<CHitDecoder, false, HIT_FMT_BLK8> THitDecoder;
    THitDecoder HitDecoder;
    const char* Data;
    size_t Size;
    const char* Cur;
    const char* End;
public:
    explicit TMemoryHitIterator(size_t)
         : Data(nullptr)
        , Size(0)
        , Cur(nullptr)
        , End(nullptr)
    {
    }
    void Init(const void* data, size_t size, EHitFormat hitFormat) {
        Data = reinterpret_cast<const char*>(data);
        Size = size;
        HitDecoder.SetFormat(hitFormat);
    }
    bool Next() {
        if (HitDecoder.Fetch())
            return true;
        if (Cur >= End)
            return false;
        HitDecoder.Next(Cur);
        return true;
    }
    SUPERLONG Current() const {
        return HitDecoder.GetCurrent();
    }
//    void Drain() {
//        Cur = End;
//    }
    void Restart(i64 offset, i32 length, i32 count) {
        Y_ASSERT(offset + length <= (i64)Size);
        Cur = Data + offset;
        End = Cur + length;
        HitDecoder.Reset();
        HitDecoder.SetSize(count);
        HitDecoder.ReadHeader(Cur);
        if (count < 8)
            HitDecoder.FallBackDecode(Cur, count);
    }
};

class TMemoryKeyIterator : private TNonCopyable {
    typedef NIndexerCore::NIndexerDetail::TInputMemoryStream TMemoryStream;
    typedef NIndexerCore::TInputIndexFileImpl<TMemoryStream> TInputMemoryIndex;
    typedef NIndexerCore::TInvKeyReaderImpl<TInputMemoryIndex> TMemoryIndexReader;

    THolder<TInputMemoryIndex> IndexFile;
    THolder<TMemoryIndexReader> IndexReader;
public:
    typedef TAdvancedHitIterator<TMemoryHitIterator> THitIterator;
    THitIterator Hits;

    void SetHitIteratorIndex(size_t i) {
        Hits.ArrIndex = i;
    }
    void Init(const TPortionBuffers& buffers, IYndexStorage::FORMAT format) {
        ui32 version = YNDEX_VERSION_CURRENT;
        if (format == IYndexStorage::FINAL_FORMAT) {
            TMemoryStream invStream(buffers.InvBuf, buffers.InvBufSize);
            NIndexerCore::TInvKeyInfo invKeyInfo; // unused, we need only version here
            ReadIndexInfoFromStream(invStream, version, invKeyInfo);
        }
        IndexFile.Reset(new TInputMemoryIndex(TMemoryStream(buffers.KeyBuf, buffers.KeyBufSize), TMemoryStream(), format, version));
        IndexReader.Reset(new TMemoryIndexReader(*IndexFile, false));
        Hits.Init(buffers.InvBuf, buffers.InvBufSize, IndexReader->GetHitFormat());
    }
    const char* Str() const {
        return IndexReader->GetKeyText();
    }
    bool NextKey() {
        return IndexReader->ReadNext();
    }
    void NextHit(bool) {
        Hits.Restart(
            IndexReader->GetOffset(),
            GetHitsLength<HIT_FMT_BLK8>(IndexReader->GetLength(), IndexReader->GetCount(), IndexReader->GetHitFormat()),
            IndexReader->GetCount());
    }
    bool operator>(const TMemoryKeyIterator& other) const {
        return strcmp(Str(), other.Str()) > 0;
    }
};

#include "advmerger-inl.h"
