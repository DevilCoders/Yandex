#include "text_normalize.h"

#include <library/cpp/charset/doccodes.h>
#include <contrib/libs/libiconv/iconv.h>

namespace NHtml {
    namespace {
        bool CheckUtf16(const ui8* buf, int size, bool* be, bool* bom) {
            if (size < 2) {
                return false;
            }
            const ui8* p = buf;
            int len = (size < 1024) ? size : 1024;
            if ((*p == 0xFE) || (*p == 0xFF)) {
                *be = (*p == 0xFE);
                *bom = true;
                return true;
            } else {
                *be = (*p == 0x00);
                *bom = false;
            }
            for (; len >= 4; len--, p++) {
                const ui8* pSpec = p;
                const ui8* pSpec1 = p + 1;
                const ui8* pSpec2 = p + 2;

                if ((*pSpec == 'h') && (*pSpec1 == 0) && (*pSpec2 == 't')) {
                    return true;
                }
            }
            return false;
        }

        bool CheckBOMinUtf8(const ui8* buf, int size) {
            if (size > 2)
                return buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF;
            return false;
        }

        // Определяет, что текст в CODES_UTF8 или CODES_UTF_16(BE/LE).
        // В остальный случаях возвращяет CODES_ASCII.
        ECharset CheckEncoding(const ui8* data, size_t len, bool* bom) {
            bool be = false;
            if (CheckUtf16(data, len, &be, bom)) {
                return be ? CODES_UTF_16BE : CODES_UTF_16LE;
            }
            if (CheckBOMinUtf8(data, len)) {
                *bom = true;
                return CODES_UTF8;
            }
            return CODES_ASCII;
        }

        inline void NulToSpace(char* buf, int size) {
            char* p;
            for (p = buf; size--; p++)
                if (!*p)
                    *p = ' ';
        }

    }

    void NormalizeUtfInput(TBuffer* buf, bool replaceNulls) {
        ui8* p = reinterpret_cast<ui8*>(buf->Data());
        size_t size = buf->Size();
        bool bom = false;
        switch (const ECharset charset = CheckEncoding(p, size, &bom)) {
            case CODES_UTF_16BE:
            case CODES_UTF_16LE: {
                size_t len = size;
                if (bom) {
                    *p = ' ';
                    p++;
                    *p = ' ';
                    p++;
                    len -= 2;
                }
                libiconv_t iconver;
                if (charset == CODES_UTF_16BE)
                    iconver = libiconv_open("utf-8", "utf-16BE");
                else
                    iconver = libiconv_open("utf-8", "utf-16LE");
                size_t normSize = 2 * len;
                size_t written = normSize;
                TBuffer normBuffer(normSize);
                char* normPointer = normBuffer.Data();
                libiconv(iconver, (char**)&p, &len, &normPointer, &written);
                normBuffer.Resize(normSize - written);
                buf->Swap(normBuffer);
                libiconv_close(iconver);
            } break;
            case CODES_UTF8:
                Y_ASSERT(bom == true);
                // Erase UTF-8 BOM.
                p[0] = ' ';
                p[1] = ' ';
                p[2] = ' ';
                break;
            case CODES_ASCII:
                break;
            default:
                Y_ASSERT(false);
                break;
        }
        if (replaceNulls) {
            NulToSpace(buf->Data(), buf->Size());
        }
    }

}
