#include "write.h"

namespace NDoom::NItemStorage::NTest {

class TLocalWadItemWriterFactory final : public IItemWriterFactory {
public:
    TLocalWadItemWriterFactory(TFsPath path)
        : Prefix_{std::move(path)}
    {
    }

    TWadItemStorageGlobalWriter MakeGlobalWriter() override {
        return TWadItemStorageGlobalWriter{Prefix_};
    }

    TWadItemStorageMappingWriter MakeMappingWriter() override {
        return TWadItemStorageMappingWriter{Prefix_};
    }

    TWadItemStorageChunkWriter MakeChunkWriter(TChunkId chunk, TString /*uuid*/) override {
        return TWadItemStorageChunkWriter{Prefix_, chunk.Id};
    }

    TVector<TFsPath> ListChunkItemWads() override {
        TVector<TFsPath> wads;
        for (ui32 i = 0;; ++i) {
            TFsPath items = TStringBuilder{} << Prefix_.GetPath() << "." << i << ".items.wad";
            if (!items.Exists()) {
                break;
            }
            wads.push_back(items);
        }
        return wads;
    }

private:
    TFsPath Prefix_;
};

class TLocalWadWriterFactory final : public IWriterFactory {
public:
    TLocalWadWriterFactory(TFsPath prefix)
        : Prefix_{std::move(prefix)}
    {
    }

    THolder<IItemWriterFactory> MakeItemWriter(TItemType itemType) override {
        return MakeHolder<TLocalWadItemWriterFactory>(Prefix_.Child("item_" + ToString(itemType)));
    }

private:
    TFsPath Prefix_;
};

THolder<IWriterFactory> MakeLocalWadWriterFactory(const TFsPath& path) {
    return MakeHolder<TLocalWadWriterFactory>(path);
}

void WriteItemIndex(const TItemIndex& data, IItemWriterFactory* writers) {
    {
        TWadItemStorageGlobalWriter writer = writers->MakeGlobalWriter();
        for (auto&& [name, data] : data.GlobalLumps) {
            writer.StartGlobalLump(name)->Write(data);
        }
        writer.Finish();
    }

    for (const TChunk& chunk : data.Chunks) {
        TWadItemStorageChunkWriter writer = writers->MakeChunkWriter(chunk.Id, chunk.Uuid);

        for (auto&& [name, data] : chunk.ChunkGlobalLumps) {
            writer.StartGlobalLump(name)->Write(data);
        }

        for (TStringBuf name : chunk.ItemLumpsList) {
            writer.RegisterItemLumpType(name);
        }
        for (auto&& [item, lumps] : chunk.ItemLumps) {
            for (auto&& [name, data] : lumps) {
                writer.StartItemLump(item, name)->Write(data);
            }
        }

        writer.Finish();
    }

    writers->FinishChunks();

    {
        TWadItemStorageMappingWriter writer = writers->MakeMappingWriter();
        writer.Finish();
    }

    {
        auto wads = writers->ListChunkItemWads();
        for (auto&& wad : wads) {
            wad.DeleteIfExists();
        }
    }
}

void WriteIndex(const TIndexData& index, IWriterFactory* writerFactory) {
    for (const TItemIndex& item : index.Items) {
        WriteItemIndex(item, writerFactory->MakeItemWriter(item.ItemType).Get());
    }
    writerFactory->Finish();
}

} // namespace NDoom::NItemStorage::NTest
