#include "key_encoder.h"

#include <util/generic/algorithm.h>

namespace NDoom {

bool TKeyEncoder::Encode(const TDecodedKey& src, TString* dst) const {
    Y_ASSERT(dst);
    dst->clear();

    *dst += src.Lemma();

    if (src.FormCount() == 0) {
        return true;
    }
    if (src.FormCount() == 1) {
        TDecodedFormRef form = src.Form(0);
        if (!form.Flags() && form.Text() == src.Lemma()) {
            return true;
        }
    }

    TString lastFormBuf;
    lastFormBuf.reserve(dst->size() + 1);
    lastFormBuf += *dst;
    lastFormBuf += '\x00';

    TString curFormBuf;
    curFormBuf.reserve(lastFormBuf.size());

    *dst += '\x01';

    for (size_t i = 0; i < src.FormCount(); ++i) {
        TDecodedFormRef form = src.Form(i);
        curFormBuf.clear();
        curFormBuf += form.Text();
        if (form.Flags() & FORM_HAS_LANG) {
            curFormBuf += static_cast<char>(form.Language());
        }
        curFormBuf += static_cast<char>(form.Flags());
        size_t same = 0;
        while (same < lastFormBuf.size() && same < curFormBuf.size() && lastFormBuf[same] == curFormBuf[same]) {
            ++same;
        }
        size_t diff = curFormBuf.size() - same;
        if (same < 8 && diff < 16) {
            *dst += static_cast<char>((same << 4) | diff);
        } else {
            if (same >= 128) {
                diff += same - 127;
                same = 127;
            }
            if (diff >= 256) {
                return false;
            }
            *dst += static_cast<char>(128 | same);
            *dst += static_cast<char>(diff);
        }
        *dst += TStringBuf(curFormBuf.data() + same, diff);
        lastFormBuf.swap(curFormBuf);
    }
    return true;
}

} // namespace NDoom
