#pragma once

#include "struct_diff_reader.h"

#include <kernel/doom/wad/wad.h>

namespace NDoom {


template <EWadIndexType indexType, class Key, class KeyVectorizer, class KeySubtractor, class Data>
class TOffroadStructDiffWadReader {
    using TReader = TStructDiffReader<Key, KeyVectorizer, KeySubtractor, Data>;
public:
    using TKey = Key;
    using TData = Data;
    using TTable = typename TReader::TTable;
    using TModel = typename TReader::TModel;

    TOffroadStructDiffWadReader() {}

    template <typename...Args>
    TOffroadStructDiffWadReader(Args&&...args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const TString& path) {
        LocalWad_ = IWad::Open(path);
        Reset(nullptr, LocalWad_.Get());
    }

    void Reset(const IWad* wad) {
        LocalWad_.Reset();
        Reset(nullptr, wad);
    }

    void Reset(const TTable* table, const IWad* wad) {
        Table_ = table;

        if (!Table_) {
            if (!LocalTable_)
                LocalTable_ = MakeHolder<TTable>();

            TModel model;
            model.Load(wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructModel)));
            LocalTable_->Reset(model);
            Table_ = LocalTable_.Get();
        }

        TBlob structSizeBlob = wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize));
        Y_ENSURE(structSizeBlob.Size() == sizeof(ui32));
        const ui32 dataSize = ReadUnaligned<ui32>(structSizeBlob.Data());
        Data_ = wad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::Struct));
        Reader_.Reset(dataSize, Table_, Data_);
    }

    bool Next(TKey* key, const TData** data) {
        return Reader_.Read(key, data);
    }

private:
    THolder<IWad> LocalWad_;
    THolder<TTable> LocalTable_;
    const TTable* Table_ = nullptr;
    TBlob Data_;
    TReader Reader_;
};


} // namespace NDoom
