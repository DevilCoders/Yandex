#pragma once

#include <kernel/search_types/search_types.h>
#include <cstdio>
#include <util/system/defaults.h>
#include <util/generic/noncopyable.h>
#include <kernel/keyinv/hitlist/hits_coders.h>

#include "fatwriter.h"
#include "indexfile.h"
#include "indexreader.h"
#include "hitwriter.h"               // only for typedef TInvKeyWriter
#include "subindexwriter.h"          // only for typedef TInvKeyWriter

namespace NIndexerCore {

//! provides interface for writing index file, aggregates hit writer and subindex writer
template <typename TOutputIndex, typename THitWriter, typename TSubIndexWriter, typename TFATWriter>
class TInvKeyWriterImpl : private TNonCopyable {
    TOutputIndex& File;
    const TSubIndexInfo SubIndexInfo;
    TFATWriter FastAccessTableWriter;

    THitWriter HitWriter;
    TSubIndexWriter SubIndexWriter;

    //! do nothing by default, see specialization
    void AssignFATWriter() const {
    }

public:
    explicit TInvKeyWriterImpl(TOutputIndex& file)
        : File(file)
        , SubIndexInfo(InitSubIndexInfo(file.GetFormat(), file.GetVersion()))
        , FastAccessTableWriter(SubIndexInfo.hasSubIndex)
        , HitWriter(DetectHitFormat(file.GetVersion()))
    {
        AssignFATWriter();
    }
    TInvKeyWriterImpl(TOutputIndex& file, bool hasSubIndex)
        : File(file)
        , SubIndexInfo(InitSubIndexInfo(file.GetFormat(), file.GetVersion(), hasSubIndex))
        , FastAccessTableWriter(hasSubIndex)
        , HitWriter(DetectHitFormat(file.GetVersion()))
    {
        AssignFATWriter();
    }
    bool HasSubIndex() const {
        return SubIndexInfo.hasSubIndex;
    }
    const TSubIndexInfo& GetSubIndexInfo() const {
        return SubIndexInfo;
    }

private:
    void Reset() {
        SubIndexWriter.ClearSubIndex();
        HitWriter.Reset();
    }
    //! the last call for the current key - writes all accumulated data to file and resets all members
    //! @return false - no positions to store
    bool WriteInvData() {
        if (!HitWriter.WriteHits(File))
            return false;
        SubIndexWriter.WriteSubIndex(File, HitWriter.GetCount(), HitWriter.GetLength(), SubIndexInfo);
        return true;
    }

public:
    //! writes positions of the current key to file
    //! @note actually positions can be cached that depends on implementation of position writer
    void WriteHit(SUPERLONG hit) {
        if (Y_UNLIKELY(HitWriter.WriteHit(File, hit, SubIndexInfo.nSubIndexStep) && SubIndexInfo.hasSubIndex)) {
            SubIndexWriter.AddPerst(HitWriter.GetLastWrittenHit(), HitWriter.GetLength());
        }
    }
    //! finishes writing of key
    //! @note analogue of TYndexFile::FlushNextKey()
    //!       analogue of TYndexFile::StoreNextKey():
    //!           WriteKey(prevText);
    void WriteKey(const char* text) {
        if (!WriteInvData())
            return;

        Y_ASSERT(text && *text && strlen(text) < MAXKEY_BUF);
        int dataLen = HitWriter.GetKeyDataLength(File);
        File.WriteKey(text, dataLen);
        HitWriter.WriteKeyData(File);
        Reset();
    }
};

template <>
inline void TInvKeyWriterImpl<TOutputIndexFile, THitWriterImpl<CHitCoder, TFile>, NIndexerDetail::TSubIndexWriter, NIndexerDetail::TFastAccessTableWriter>::AssignFATWriter() const {
    File.SetFATWriter(&FastAccessTableWriter);
}

using TInvKeyWriter = TInvKeyWriterImpl<TOutputIndexFile, THitWriterImpl<CHitCoder, TFile>, NIndexerDetail::TSubIndexWriter, NIndexerDetail::TFastAccessTableWriter>;

class TRawInvKeyWriter : private TNonCopyable {
    TOutputIndexFile& IndexFile;
    const TSubIndexInfo SubIndexInfo;
    NIndexerDetail::TFastAccessTableWriter FastAccessTableWriter;

    i64 Count;
    ui32 Length;
    SUPERLONG LastHit;

private:
    void Reset() {
        Count = 0;
        Length = 0;
        LastHit = START_POINT;
    }

public:
    explicit TRawInvKeyWriter(TOutputIndexFile& indexFile)
        : IndexFile(indexFile)
        , SubIndexInfo(InitSubIndexInfo(IndexFile.GetFormat(), IndexFile.GetVersion(), false))
        , FastAccessTableWriter(false)
        , Count(0)
        , Length(0)
        , LastHit(START_POINT)
    {
        Y_ASSERT(IndexFile.GetVersion() == YNDEX_VERSION_RAW64_HITS);
        IndexFile.SetFATWriter(&FastAccessTableWriter);
    }
    bool HasSubIndex() const {
        return false;
    }
    void WriteHit(SUPERLONG hit) {
        Y_ASSERT(hit >= LastHit);
        Count++;
        Length += IndexFile.WriteInvData(hit - LastHit);
        LastHit = hit;
    }
    //! @note raw hits are not counted
    void WriteRawHit(SUPERLONG hit) {
        Length += IndexFile.WriteInvData(hit);
    }
    void WriteHits(const void* data, ui32 length, i64 count) {
        IndexFile.WriteInvData(data, length);
        Count += count;
        Length += length;
    }
    void WriteKey(const char* text) {
        if (!Length)
            return;

        Y_ASSERT(text && *text);
        Y_ASSERT(strlen(text) < MAXKEY_BUF); // in release it is supposed that length of text is OK
        const bool finalFormat = (IndexFile.GetFormat() == IYndexStorage::FINAL_FORMAT);

        int dataLen = 0;
        if (finalFormat)
            dataLen += IndexFile.GetKeyDataLen(Count);
        dataLen += IndexFile.GetKeyDataLen(Length);

        IndexFile.WriteKey(text, dataLen);

        if (finalFormat)
            IndexFile.WriteKeyData(Count);
        IndexFile.WriteKeyData(Length);

        Reset();
    }
};

} // namespace NIndexerCore
