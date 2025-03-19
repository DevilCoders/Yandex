#pragma once

#include <kernel/search_types/search_types.h>
#include "indexreader.h"
#include "oldindexfileimpl.h"

namespace NIndexerCore {

    //! writes all version of index files
    class TOldIndexFile : public NIndexerDetail::TOldIndexFileImpl<TOldIndexFile, TInvKeyWriter> {
        typedef NIndexerDetail::TOldIndexFileImpl<TOldIndexFile, TInvKeyWriter> TBase;
        const bool HasSubIndex;
    public:
        explicit TOldIndexFile(IYndexStorage::FORMAT format = IYndexStorage::PORTION_FORMAT, ui32 version = YNDEX_VERSION_CURRENT)
            : TBase(format, version)
            , HasSubIndex(format == IYndexStorage::FINAL_FORMAT)
        {
        }
        explicit TOldIndexFile(IYndexStorage::FORMAT format, ui32 version, bool hasSubIndex)
            : TBase(format, version)
            , HasSubIndex(hasSubIndex)
        {
        }
        TInvKeyWriter* CreateIndexWriter(TOutputIndexFile& indexFile) const {
            return new TInvKeyWriter(indexFile, HasSubIndex);
        }
    };

    //! writes YNDEX_VERSION_RAW64_HITS index files
    class TRawIndexFile : public NIndexerDetail::TOldIndexFileImpl<TRawIndexFile, TRawInvKeyWriter> {
        typedef NIndexerDetail::TOldIndexFileImpl<TRawIndexFile, TRawInvKeyWriter> TBase;
    public:
        TRawIndexFile()
            : TBase(IYndexStorage::FINAL_FORMAT, YNDEX_VERSION_RAW64_HITS)
        {
        }
        TRawInvKeyWriter* CreateIndexWriter(TOutputIndexFile& indexFile) const {
            return new TRawInvKeyWriter(indexFile);
        }
        //! for index-based containers, not really indices
        //! @note TBufferedHitIterator::ReadPackedI64() can be used to retrieve this value;
        //!       files with such hits must not be merged using TAdvancedIndexMerger
        void StoreRaw(SUPERLONG data) {
            TBase::StoreRawHit(data);
        }
    };

    //! writes key-file only (version, FAT and 12-bytes information block is written to inv-file)
    class TOnlyKeysIndexFile : public TNonCopyable {
        TOutputIndexFile IndexFile;
        const TSubIndexInfo SubIndexInfo;
        NIndexerDetail::TFastAccessTableWriter FastAccessTableWriter;
    public:
        TOnlyKeysIndexFile()
            : IndexFile(IYndexStorage::FINAL_FORMAT, YNDEX_VERSION_CURRENT) // actually version does not make sence
            , SubIndexInfo(InitSubIndexInfo(IndexFile.GetFormat(), IndexFile.GetVersion(), false))
            , FastAccessTableWriter(false)
        {
            IndexFile.SetFATWriter(&FastAccessTableWriter);
        }
        void Open(const char* prefix) {
            IndexFile.Open(prefix);
        }
        void Open(const char* keyName, const char* invName) {
            IndexFile.Open(keyName, invName);
        }
        void CloseEx() {
            IndexFile.FlushKeyBlock();
            IndexFile.CloseEx();
        }
        void WriteKey(const char* text, i64 count, ui32 length) {
            Y_ASSERT(strlen(text) < MAXKEY_BUF); // in release it is supposed that length of text is OK
            Y_ASSERT(IndexFile.GetFormat() == IYndexStorage::FINAL_FORMAT);

            int dataLen = IndexFile.GetKeyDataLen(count);
            dataLen += IndexFile.GetKeyDataLen(length);

            IndexFile.WriteKey(text, dataLen);

            IndexFile.WriteKeyData(count);
            IndexFile.WriteKeyData(length);
        }
    };

    class TIndexReader : public TNonCopyable {
        NIndexerCore::TInputIndexFile IndexFile;
        THolder<TMemoryMap> InvMapping;
        THolder<NIndexerCore::TInvKeyReader> IndexReader;
    private:
        void Initialize() {
            Y_ASSERT(IndexFile.IsOpen());
            InvMapping.Reset(new TMemoryMap(IndexFile.CreateInvMapping()));
            IndexReader.Reset(new NIndexerCore::TInvKeyReader(IndexFile));
        }
    public:
        explicit TIndexReader(const char* prefix, IYndexStorage::FORMAT format = IYndexStorage::FINAL_FORMAT)
            : IndexFile(format)
        {
            IndexFile.Open(prefix);
            Initialize();
        }
        TIndexReader(const char* keyName, const char* invName, IYndexStorage::FORMAT format = IYndexStorage::FINAL_FORMAT)
            : IndexFile(format)
        {
            IndexFile.Open(keyName, invName);
            Initialize();
        }
        ui32 GetVersion() const {
            return IndexFile.GetVersion();
        }
        bool ReadKey() {
            return IndexReader->ReadNext();
        }
        template <typename TDecoder>
        void InitPosIterator(TPosIterator<TDecoder>& it, i64 offset, ui32 length, i64 count) const {
            IndexReader->InitPosIterator(*InvMapping, it, offset, length, count);
        }
        template <typename TDecoder>
        void InitPosIterator(TPosIterator<TDecoder>& it) const {
            IndexReader->InitPosIterator(*InvMapping, it);
        }
        const char* GetKeyText() const {
            return IndexReader->GetKeyText();
        }
        i64 GetCount() const {
            return IndexReader->GetCount();
        }
        ui32 GetLength() const {
            return IndexReader->GetLength();
        }
        i64 GetOffset() const {
            return IndexReader->GetOffset();
        }
        ui32 GetSizeOfHits() const {
            return IndexReader->GetSizeOfHits();
        }
        i64 GetInvFileLength() const {
            return IndexFile.GetInvFileLength();
        }
    };

} // NIndexerCore
