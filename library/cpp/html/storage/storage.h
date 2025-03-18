#pragma once

#include "queue.h"

#include <library/cpp/html/face/onchunk.h>

namespace NHtml {
    class TStorage {
    public:
        enum : bool {
            CopyText = false,
            ExternalText = true
        };

        explicit TStorage(bool mode = CopyText);
        ~TStorage();

        TSegmentedQueueIterator Begin() const;

        TSegmentedQueueIterator End() const;

        void Clear();

        THtmlChunk* Push(const THtmlChunk& e);

        void SetPeerMode(bool mode) {
            IsThereExternalText = mode;
        }

    private:
        TMemoryPool Data_;
        TIntrusiveChunkQueue Queue_;
        size_t Current_;
        bool IsThereExternalText;
    };

    class TParserResult: public IParserResult {
    public:
        TParserResult(TStorage& storage);
        ~TParserResult() override;

        THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override;

    private:
        TStorage& Storage;
    };

}
