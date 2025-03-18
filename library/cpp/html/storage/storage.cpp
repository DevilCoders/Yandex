#include "storage.h"

namespace NHtml {
    TStorage::TStorage(bool mode)
        : Data_(64 << 10)
        , Current_(0)
        , IsThereExternalText(mode)
    {
    }

    TStorage::~TStorage() {
    }

    TSegmentedQueueIterator TStorage::Begin() const {
        return Queue_.Begin();
    }

    TSegmentedQueueIterator TStorage::End() const {
        return Queue_.End();
    }

    void TStorage::Clear() {
        Queue_.Clear();
        Data_.Clear();
    }

    THtmlChunk* TStorage::Push(const THtmlChunk& e) {
        auto chunk = new (Data_) TIntrusiveChunkQueue::TChunk(e, Current_++);

        if (e.AttrCount) {
            auto attrs = Data_.AllocateArray<NHtml::TAttribute>(e.AttrCount);

            for (size_t i = 0; i < e.AttrCount; ++i) {
                attrs[i] = e.Attrs[i];
            }

            chunk->Attrs = attrs;
        }

        if ((IsThereExternalText == CopyText) && e.leng) {
            chunk->text = Data_.Append(e.text, e.leng);
        }

        Queue_.PushBack(chunk);

        return chunk;
    }

    TParserResult::TParserResult(TStorage& storage)
        : Storage(storage)
    {
        Storage.Clear();
    }

    TParserResult::~TParserResult() {
        Storage.Push(THtmlChunk(PARSED_EOF));
    }

    THtmlChunk* TParserResult::OnHtmlChunk(const THtmlChunk& chunk) {
        return Storage.Push(chunk);
    }

}
