#pragma once

#include "item_key.h"

#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/wad/wad.h>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/tuple/tuple_writer.h>

#include <util/folder/path.h>
#include <util/generic/hash_set.h>

namespace NDoom::NItemStorage {

class TWadItemStorageChunkWriter {
    using TItemsWriter = NOffroad::TTupleWriter<TItemKey, TItemKeyVectorizer, NOffroad::TD3I1Subtractor, NOffroad::TEncoder64, 1, NOffroad::PlainOldBuffer>;

public:
    TWadItemStorageChunkWriter(IOutputStream* output, IOutputStream* itemsOutput);
    TWadItemStorageChunkWriter(const TFsPath& prefix, ui32 chunk);
    TWadItemStorageChunkWriter(IWadWriter* output, IOutputStream* itemsOutput);
    void Reset();

    void RegisterItemLumpType(TStringBuf lumpId);
    IOutputStream* StartGlobalLump(TStringBuf lumpId);
    IOutputStream* StartItemLump(TItemKey item, TStringBuf lumpId);
    void Finish();

    ui32 CurrentDocId() const {
        return NumItems_ - 1;
    }

private:
    THolder<IWadWriter> WriterHolder_;
    IWadWriter* Writer_ = nullptr;
    NDoom::TMegaWadWriter ItemsOutput_;
    THolder<TItemsWriter::TTable> ItemsTable_;
    TItemsWriter Items_;
    ui32 NumItems_ = 0;
    TMaybe<TItemKey> PrevItem_;
};

class TWadItemStorageGlobalWriter {
public:
    TWadItemStorageGlobalWriter(IOutputStream* out);
    TWadItemStorageGlobalWriter(const TFsPath& prefix);

    IOutputStream* StartGlobalLump(TStringBuf lumpId);
    void Finish();

private:
    NDoom::TMegaWadWriter Writer_;
};

struct TMappingConfig {
    double LoadFactor = 0.995;
};

TVector<TItemKey> ReadItemKeys(IWad* itemsWad);

// TODO(sskvor): Sharded mapping
class TWadItemStorageMappingWriter {
public:
    TWadItemStorageMappingWriter(const TFsPath& prefix, TMappingConfig config = {});
    TWadItemStorageMappingWriter(TConstArrayRef<TBlob> inputs, IOutputStream* output, TMappingConfig config = {});

    void SetRemovedKeys(ui32 chunk, TConstArrayRef<TItemKey> keys);

    void Finish();

private:
    TMappingConfig MappingConfig_;

    TVector<THolder<IWad>> ChunkMappings_;
    TVector<THashSet<TItemKey>> RemovedKeys_;

    THolder<IOutputStream> LocalOutput_;
    TMegaWadWriter Writer_;
};

} // namespace NDoom::NItemStorage
