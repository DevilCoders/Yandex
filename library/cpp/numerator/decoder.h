#pragma once

#include <library/cpp/html/face/zoneconf.h>
#include <library/cpp/charset/wide.h>

#ifndef MAX_ALLOCA
#define MAX_ALLOCA 8192
#endif

class IDecoder {
public:
    virtual size_t DecodeText(const char* text, size_t len, wchar16* buffer) const = 0;
    virtual size_t DecodeEvent(const THtmlChunk& chunk, wchar16* buffer) const;
    virtual size_t GetDecodeBufferSize(size_t sourceBytes) const = 0;
    virtual ~IDecoder() {
    }
};

class TNlpInputDecoder: public IDecoder {
public:
    explicit TNlpInputDecoder(ECharset charset)
        : Charset(charset)
    {
    }
    size_t DecodeText(const char* text, size_t len, wchar16* buffer) const override;
    size_t GetDecodeBufferSize(size_t sourceBytes) const override;

protected:
    const ECharset Charset;
};

inline size_t GetDecodeBufferSize(const TAttrEntry* attrs, size_t attrCount, const IDecoder* decoder) {
    size_t size = 0;
    for (size_t i = 0; i < attrCount; ++i)
        size += decoder->GetDecodeBufferSize(+attrs[i].Value);
    return size;
}

inline void DecodeAttrValues(TAttrEntry* attrs, size_t attrCount, wchar16* buffer, const IDecoder* decoder) {
    wchar16* p = buffer;
    for (size_t i = 0; i < attrCount; ++i) {
        TAttrEntry& attr = attrs[i];
        size_t n = +attr.Value;
        if (attr.Type == ATTR_URL || attr.Type == ATTR_UNICODE_URL || attr.Type == ATTR_NOFOLLOW_URL) {
            CharToWide(~attr.Value, n, p, csYandex);
        } else if (n) {
            n = decoder->DecodeText(~attr.Value, n, p);
            Y_ASSERT(n <= +attr.Value);
        }
        attr.DecodedValue = TWtringBuf(p, n);
        p[n] = 0;
        p += n + 1;
    }
}
