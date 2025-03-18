#include "decoder.h"

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/face/event.h>

#include <util/system/defaults.h>
#include <library/cpp/charset/recyr.hh>
#include <util/memory/tempbuf.h>
#include <util/charset/wide.h>

size_t IDecoder::DecodeEvent(const THtmlChunk& chunk, wchar16* buffer) const {
    if (!chunk.text || !chunk.leng)
        return 0;

    return DecodeText(chunk.text, chunk.leng, buffer);
}

size_t TNlpInputDecoder::DecodeText(const char* text, size_t len, wchar16* buffer) const {
    return HtEntDecodeToChar(Charset, text, len, buffer);
}

size_t TNlpInputDecoder::GetDecodeBufferSize(size_t sourceBytes) const {
    return (sourceBytes + 1) * 2;
}
