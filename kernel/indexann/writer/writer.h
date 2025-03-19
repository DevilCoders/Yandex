#pragma once

/// author@ vvp@ Victor Ploshikhin
/// created: Sep 26, 2013 8:41:17 PM
/// see: ROBOT-2993

#include <kernel/indexann/interface/writer.h>

#include <kernel/indexer/directindex/directindex.h>
#include <kernel/indexann/protos/portion.pb.h>

#include <kernel/keyinv/indexfile/indexstorageface.h>

#include <util/generic/ptr.h>
#include <util/folder/path.h>
#include <util/generic/maybe.h>
#include <util/generic/array_ref.h>

namespace NIndexerCore {
    class IDirectTextCallback2;
    class TMemoryPortionFactory;
} // namespace NIndexerCore

struct IYndexStorageFactory;

namespace NIndexAnn {


struct TWriterConfig
{
    TFsPath RecognizerDataPath;
    size_t InvIndexMem = 50ULL * 1024 * 1024;
    size_t InvIndexDocCount = 500 * 1024;
    bool AutoIncrementBreaks = true;
    bool Verbose = true;
    NIndexerCore::TDTCreatorConfig DtcCfg;

    // if you are not provide recognizerDataPath you should set TSentenceParams::TextLanguage for each sentence manually
    explicit TWriterConfig(const TFsPath& recognizerDataPath = TFsPath(), NIndexerCore::TDTCreatorConfig dtcCfg = NIndexerCore::TDTCreatorConfig())
        : RecognizerDataPath(recognizerDataPath)
        , DtcCfg(dtcCfg)
    {}
};

struct TArchiveWriterConfig
{
    bool UseHeader;
    ui32 ArchiveVersion;
    IOutputStream* ArchiveStream;

    TArchiveWriterConfig(IOutputStream* archiveStream = nullptr, bool useHeader = true, ui32 archiveVersion = ARCVERSION)
        : UseHeader(useHeader)
        , ArchiveVersion(archiveVersion)
        , ArchiveStream(archiveStream)
    {}
};

struct TSentenceParams
{
    // disables language recognition if set
    TMaybe<ELanguage> TextLanguage;
    // use this mask if set
    TLangMask LangMask;

    NIndexerCore::TDirectIndex::TAttributes Attrs;
    RelevLevel RLevel = MID_RELEV;
    bool IncBreak = true;
};

static constexpr size_t FIRST_ANN_BREAK_ID = 1;

class TWriter {
public:
     // If autoIncrementBreaks is set to false (the default is true) the text passed to StartSentence() will always be stored
     // as a single sentence in the resulting index/archive. Note that the text may be silently truncated if it contains
     // more than WORD_LEVEL_Max words.
    TWriter(const TString& indexPath, const TString& tmpDir, const TWriterConfig& cfg, TAtomicSharedPtr<IDocDataWriter> dataWriter, bool needArc = false);
    TWriter(TSimpleSharedPtr<IYndexStorageFactory> storage, TAtomicSharedPtr<IDocDataWriter> dataWriter, const TWriterConfig& cfg, const TArchiveWriterConfig& archiveCfg = TArchiveWriterConfig());
    virtual ~TWriter();

    void AddDirectTextCallback(NIndexerCore::IDirectTextCallback2* obj);
    void AddDirectTextCallback(NIndexerCore::IDirectTextCallback5* obj);

    // Note: documents should be sorted by docId!
    // Note: max valid docId is DOC_LEVEL_Max == (1<<26)-1.
    void StartDoc(ui32 docId, ui64 feedId = 0);
    void FinishDoc(bool makePortion = false, TFullDocAttrs* docAttrs = nullptr);

    /**
     * Starts new sentence.
     * Returns false in one the following cases:
     *  - exceptions of string utf8 to wide conversion
     *  - too much data
     */
    bool StartSentence(const TString& text, const TSentenceParams& sentParams = TSentenceParams(), const void* callbackV5Data = nullptr);
    bool StartSentence(const TUtf16String& text, const TSentenceParams& sentParams = TSentenceParams(), const void* callbackV5Data = nullptr);

    void AddData(size_t region, size_t dataKey, const TArrayRef<const char>& data);

    // this library issues some warnings, i.e. "too many breaks for document", "adding sentence created multiple breaks", etc.
    // you can reroute them to this stream (defaults to stderr)
    // does _not_ take ownership, you have to ensure that your stream is valid thoughout the lifetime of TWriter instance
    void SetWarningStream(IOutputStream& newCerr);
    // it is possible to change Indexer behavior during indexing
    void SetAutoIncrementBreaks(bool f);
protected:
    TWriter();

protected:
    THolder<class TWriterImplBase> Impl;
};

class TMemoryWriter: public TWriter {
public:
    TMemoryWriter(const TWriterConfig& cfg, IYndexStorage::FORMAT format = IYndexStorage::PORTION_FORMAT, bool needArc = false);

    void StartPortion();

    // you can modify it until next call of StartPortion
    TPortionPB& DonePortion();
};


// With cfg we are able to make portions of certain number of documents and exact size of portion.
// Here is an implementation which gives us full access to MemoryFactory and ArchBuffer. So, we are able to minimize number of copy operations.
class TFullAccessMemoryWriter: public TMemoryWriter {
public:
    TFullAccessMemoryWriter(const TWriterConfig& cfg, IYndexStorage::FORMAT format = IYndexStorage::PORTION_FORMAT, bool needArc = false);
    NIndexerCore::TMemoryPortionFactory& GetStorageFactory() const;
    TBuffer& GetArchBuffer() const;
    void MakePortion();

private:
    bool NeedArc = false;
};


} // namespace NIndexAnn
