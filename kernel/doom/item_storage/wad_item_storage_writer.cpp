#include "wad_item_storage_writer.h"
#include "item_chunk_mapper.h"

#include <util/string/builder.h>

namespace NDoom::NItemStorage {

TWadItemStorageChunkWriter::TWadItemStorageChunkWriter(IOutputStream* output, IOutputStream* itemsOutput)
    : WriterHolder_(MakeHolder<TMegaWadWriter>(output))
    , Writer_(WriterHolder_.Get())
    , ItemsOutput_{ itemsOutput }
{
    Reset();
}

TWadItemStorageChunkWriter::TWadItemStorageChunkWriter(const TFsPath& prefix, ui32 chunk)
    : WriterHolder_(MakeHolder<TMegaWadWriter>(TStringBuilder{} << prefix.GetPath() << '.' << chunk << ".wad"))
    , Writer_(WriterHolder_.Get())
    , ItemsOutput_{ TStringBuilder{} << prefix.GetPath() << '.' << chunk << ".items.wad" }
{
    Reset();
}

TWadItemStorageChunkWriter::TWadItemStorageChunkWriter(IWadWriter* output, IOutputStream* itemsOutput)
    : Writer_(output)
    , ItemsOutput_{ itemsOutput }
{
    Reset();
}

void TWadItemStorageChunkWriter::Reset() {
    ItemsTable_ = MakeHolder<TItemsWriter::TTable>(TItemsWriter::TModel{});
    Items_.Reset(ItemsTable_.Get(), ItemsOutput_.StartGlobalLump("item_storage_items"));
    NumItems_ = 0;
}

IOutputStream* TWadItemStorageChunkWriter::StartGlobalLump(TStringBuf lumpId) {
    return Writer_->StartGlobalLump(lumpId);
}

void TWadItemStorageChunkWriter::RegisterItemLumpType(TStringBuf lumpId) {
    return Writer_->RegisterDocLumpType(lumpId);
}

IOutputStream* TWadItemStorageChunkWriter::StartItemLump(TItemKey item, TStringBuf lumpId) {
    if (PrevItem_ != item) {
        PrevItem_ = item;
        Items_.WriteHit(item);
        ++NumItems_;
    }
    return Writer_->StartDocLump(CurrentDocId(), lumpId);
}

void TWadItemStorageChunkWriter::Finish() {
    Items_.Finish();

    ItemsOutput_.WriteGlobalLump("item_storage_num_items", ToString(NumItems_));

    ItemsOutput_.Finish();
    Writer_->Finish();
}

TWadItemStorageGlobalWriter::TWadItemStorageGlobalWriter(IOutputStream* out)
    : Writer_{ out }
{
}

TWadItemStorageGlobalWriter::TWadItemStorageGlobalWriter(const TFsPath& prefix)
    : Writer_{ prefix.GetPath() + ".global.wad" }
{
}

IOutputStream* TWadItemStorageGlobalWriter::StartGlobalLump(TStringBuf lumpId) {
    return Writer_.StartGlobalLump(lumpId);
}

void TWadItemStorageGlobalWriter::Finish() {
    Writer_.Finish();
}

using TItemsReader = NOffroad::TTupleReader<TItemKey, TItemKeyVectorizer, NOffroad::TD3I1Subtractor, NOffroad::TDecoder64, 1, NOffroad::PlainOldBuffer>;

class TItemKeyReader {
public:
    TItemKeyReader(IWad* itemsWad)
        : ItemsWad_(itemsWad)
        , Model_(MakeHolder<TItemsReader::TTable>(TItemsReader::TModel{}))
    {
        TBlob numItemsBlob = ItemsWad_->LoadGlobalLump("item_storage_num_items");
        NumItems_ = FromString(TStringBuf{ numItemsBlob.AsCharPtr(), numItemsBlob.Size() });
        if (NumItems_ == 0) {
            return;
        }
        ItemsBlob_ = ItemsWad_->LoadGlobalLump("item_storage_items");
        Reader_.Reset(Model_.Get(), TArrayRef{ ItemsBlob_.AsCharPtr(), ItemsBlob_.Size() });
        Y_ENSURE(Reader_.ReadHit(&ItemKey_));
    }

    bool IsValid() const {
        return RowIndex_ < NumItems_;
    }
    ui32 Size() const {
        return NumItems_;
    }
    ui32 RowIndex() const {
        return RowIndex_;
    }
    TItemKey ItemKey() const {
        return ItemKey_;
    }

    void Next() {
        if (++RowIndex_ >= NumItems_) {
            return;
        }
        Y_ENSURE(Reader_.ReadHit(&ItemKey_));
    }

private:
    IWad* ItemsWad_ = nullptr;
    THolder<TItemsReader::TTable> Model_;
    TBlob ItemsBlob_;
    TItemsReader Reader_;
    TItemKey ItemKey_;
    ui32 NumItems_ = 0;
    ui32 RowIndex_ = 0;
};

TVector<TItemKey> ReadItemKeys(IWad* itemsWad) {
    TItemKeyReader reader(itemsWad);
    TVector<TItemKey> keys(Reserve(reader.Size()));
    for (; reader.IsValid(); reader.Next()) {
        keys.push_back(reader.ItemKey());
    }
    return keys;
}

TWadItemStorageMappingWriter::TWadItemStorageMappingWriter(const TFsPath& prefix, TMappingConfig config)
    : MappingConfig_{std::move(config)}
{
    for (ui32 i = 0;; ++i) {
        TFsPath items = TStringBuilder{} << prefix.GetPath() << "." << i << ".items.wad";
        if (!items.Exists()) {
            Y_ENSURE(i > 0, "No chunks found");
            break;
        }

        ChunkMappings_.push_back(IWad::Open(items));
    }

    LocalOutput_ = MakeHolder<TBufferedFileOutputEx>(prefix.GetPath() + ".mapping.wad", WrOnly | CreateAlways | Direct | DirectAligned | Seq, 1 << 17);
    Writer_.Reset(LocalOutput_.Get());
}

TWadItemStorageMappingWriter::TWadItemStorageMappingWriter(TConstArrayRef<TBlob> inputs, IOutputStream* output, TMappingConfig config)
    : MappingConfig_{std::move(config)}
    , Writer_{output}
{
    ChunkMappings_.reserve(inputs.size());
    for (const TBlob& input : inputs) {
        ChunkMappings_.push_back(IWad::Open(input));
    }
}

void TWadItemStorageMappingWriter::SetRemovedKeys(ui32 chunk, TConstArrayRef<TItemKey> keys) {
    if (RemovedKeys_.size() <= chunk) {
        RemovedKeys_.resize(chunk + 1);
    }
    RemovedKeys_[chunk].insert(keys.begin(), keys.end());
}

void TWadItemStorageMappingWriter::Finish() {
    TItemChunkMappingIo::TWriter writer{{}, &Writer_};

    for (auto&& [chunk, wad] : Enumerate(ChunkMappings_)) {
        THashSet<TItemKey>* removedKeys = chunk < RemovedKeys_.size() ? &RemovedKeys_[chunk] : nullptr;

        TItemKeyReader reader(wad.Get());
        for (; reader.IsValid(); reader.Next()) {
            TItemKey item = reader.ItemKey();

            if (removedKeys && removedKeys->contains(item)) {
                continue;
            }

            writer.WriteKey(item, TChunkLocalId{
                .Chunk = static_cast<ui32>(chunk),
                .LocalId = reader.RowIndex(),
            });
        }

        wad.Reset();
    }

    NOffroad::NMinHash::TMinHashParams params {
        .LoadFactor = MappingConfig_.LoadFactor,
    };

    writer.Finish(params);
    Writer_.Finish();
    if (LocalOutput_) {
        LocalOutput_->Finish();
    }
}

} // namespace NDoom::NItemStorage
