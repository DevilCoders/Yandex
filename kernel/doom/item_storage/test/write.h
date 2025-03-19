#pragma once

#include "index.h"

#include <kernel/doom/item_storage/wad_item_storage_writer.h>

#include <util/folder/path.h>

namespace NDoom::NItemStorage::NTest {

class IItemWriterFactory {
public:
    virtual ~IItemWriterFactory() = default;

    virtual TWadItemStorageGlobalWriter MakeGlobalWriter() = 0;
    virtual TWadItemStorageMappingWriter MakeMappingWriter() = 0;
    virtual TWadItemStorageChunkWriter MakeChunkWriter(TChunkId chunk, TString uuid) = 0;
    virtual TVector<TFsPath> ListChunkItemWads() = 0;
    virtual void FinishChunks() {}
};

class IWriterFactory {
public:
    virtual ~IWriterFactory() = default;

    virtual THolder<IItemWriterFactory> MakeItemWriter(TItemType itemType) = 0;
    virtual void Finish() {}
};

THolder<IWriterFactory> MakeLocalWadWriterFactory(const TFsPath& path);

void WriteIndex(const TIndexData& index, IWriterFactory* writerFactory);

} // namespace NDoom::NItemStorage::NTest
