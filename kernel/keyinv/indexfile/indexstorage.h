#pragma once

#include "indexstorageface.h"
#include "indexfile.h"
#include "indexstorageimpl.h"
#include "indexreader.h"
#include "indexwriter.h"

namespace NIndexerCore {

    class TIndexStorage : public IYndexStorage, private TNonCopyable {
        TOutputIndexFile IndexFile;
        TInvKeyWriter IndexWriter;
        typedef NIndexerDetail::TIndexStorageImpl<TOutputIndexFile, TInvKeyWriter> TStorageImpl;
        TStorageImpl Impl;
    public:
        TIndexStorage(const char* keyName, const char* invName, FORMAT format = PORTION_FORMAT,
            ui32 version = YNDEX_VERSION_CURRENT, bool alwaysRemoveDuplicateHits = false)
            : IndexFile(keyName, invName, format, version)
            , IndexWriter(IndexFile, (format == FINAL_FORMAT))
            , Impl(IndexFile, IndexWriter, alwaysRemoveDuplicateHits)
        {
        }
        void Close() {
            Impl.Flush();
            IndexFile.CloseEx(); // writes FAT if needed
        }
        void StorePositions(const char* textPointer, SUPERLONG* filePositions, size_t filePosCount) override {
            Impl.StorePositions(textPointer, filePositions, filePosCount);
        }
    };

}
