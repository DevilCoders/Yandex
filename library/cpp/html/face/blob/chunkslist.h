#pragma once

#include <library/cpp/html/face/onchunk.h>

#include <util/generic/buffer.h>
#include <util/generic/ptr.h>

namespace NHtml {
    /**
     * Reference to memory block with serialized parser chunks.
     */
    class TChunksRef: public TStringBuf {
    public:
        inline explicit TChunksRef(const TBuffer& buf)
            : TStringBuf(buf.Data(), buf.Size())
        {
        }

        inline TChunksRef(const char* data, size_t len)
            : TStringBuf(data, len)
        {
        }
    };

    class THtmlChunksWriter: public IParserResult {
    public:
        THtmlChunksWriter(size_t docLen = 64 << 10);
        ~THtmlChunksWriter() override;

        TBuffer CreateResultBuffer();

    public:
        /* IParserResult */
        THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override;

    private:
        class TImpl;
        THolder<TImpl> Impl_;
    };

    /**
     * @param chunks сериализованный результат html-парсера.
     * @param result интерфейс получения THtmlChunk.
     *
     * @note Ссылки на строки в THmlChunk проставляются на память в data.
     */
    bool NumerateHtmlChunks(const TChunksRef& chunks, IParserResult* result);

}
