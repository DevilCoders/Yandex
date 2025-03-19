#pragma once

#include "indexstorageimpl.h"
#include "indexwriter.h" // for TOutputIndexFile

namespace NIndexerCore {
    namespace NIndexerDetail {

        //! represents old interface of index file for writing (old class TYndexFile)
        template <typename TDerived, typename TIndexWriter>
        class TOldIndexFileImpl : private TNonCopyable {
            TOutputIndexFile IndexFile;
            THolder<TIndexWriter> IndexWriter;
            typedef TIndexStorageImpl<TOutputIndexFile, TIndexWriter> TStorageImpl;
            THolder<TStorageImpl> StorageImpl;
        private:
            void CreateIndexWriter() {
                Y_ASSERT(!IndexWriter);
                IndexWriter.Reset(static_cast<const TDerived*>(this)->CreateIndexWriter(IndexFile));
            }
            void CreateStorageImpl() {
                StorageImpl.Reset(new TStorageImpl(IndexFile, *IndexWriter));
            }
        public:
            //! @param alwaysRemoveDuplicateHits    works for StorePositions() only
            TOldIndexFileImpl(IYndexStorage::FORMAT format, ui32 version)
                : IndexFile(format, version)
            {
            }
            void Open(const char* prefix, int keyBufSize = INIT_FILE_BUF_SIZE, int invBufSize = INIT_FILE_BUF_SIZE, bool directIO = false) {
                IndexFile.Open(prefix, keyBufSize, invBufSize, directIO);
                CreateIndexWriter();
                CreateStorageImpl();
            }
            void Open(const char* keyName, const char* invName, int keyBufSize = INIT_FILE_BUF_SIZE, int invBufSize = INIT_FILE_BUF_SIZE, bool directIO = false) {
                IndexFile.Open(keyName, invName, keyBufSize, invBufSize, directIO);
                CreateIndexWriter();
                CreateStorageImpl();
            }
            IYndexStorage::FORMAT GetFormat() const {
                return IndexFile.GetFormat();
            }
            ui32 GetVersion() const {
                return IndexFile.GetVersion();
            }
            void StoreNextHit(SUPERLONG hit) {
                StorageImpl->StoreHit(hit);
            }
            void StoreRawHit(SUPERLONG hit) {
                StorageImpl->StoreRawHit(hit);
            }
            void StorePositions(const char* keyText, const SUPERLONG* positions, size_t posCount) {
                StorageImpl->StorePositions(keyText, positions, posCount);
            }
            void StoreHits(const void* data, ui32 length, i64 count) {
                StorageImpl->StoreHits(data, length, count);
            }
            void StoreNextKey(const char* key) {
                StorageImpl->StoreKey(key);
            }
            void FlushNextKey(const char* key) {
                StorageImpl->FlushKey(key);
            }
            void Flush() {
                StorageImpl->Flush();
            }
            void CloseEx() {
                Flush();
                IndexFile.CloseEx(); // writes FAT if needed
                IndexWriter.Reset(nullptr);
            }
        };

    } // NIndexerDetail
} // NIndexerCore

