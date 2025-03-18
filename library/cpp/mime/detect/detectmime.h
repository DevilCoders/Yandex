#pragma once

#include <library/cpp/mime/types/mime.h>

#include <util/generic/buffer.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>
#include <util/generic/noncopyable.h>

class TMimeDetector : TNonCopyable {
public:
    TMimeDetector() {
        Reset();
        MaxBytesToRead = MAX_BYTES_TO_READ_DEF;
        Utf8ErrorsRatio = UTF8_ERRORS_RATIO_DEF;
        NonPrintableRatio = NON_PRINTABLE_RATIO_DEF;
        DelimitersRatio = DELIMITERS_RATIO_DEF;
        HtmltagRatio = HTMLTAG_RATIO_DEF;
    }

    void Reset();

    bool Detect(const void* buf, const size_t length) {
        if (!length)
            return NeedMore;
        return FeedDetector((const unsigned char*)buf, ((const unsigned char*)buf) + length);
    }

    /*
       Returns detected mime-type.
       When called with default argument, returns mime-type, detected by magic numbers (MIME_TEXT is never returned).
       It's possible to set a hint (e.g. a mime-type from http headers) for better detection.
    */
    MimeTypes Mime(MimeTypes m = MIME_UNKNOWN) const {
        if (!BytesRead)
            return m;
        bool is_text = IsText();
        switch (m) {
            case MIME_HTML:
                if (NumTags * HtmltagRatio > BytesRead)
                    return MIME_HTML;
                if (MimeState != MIME_UNKNOWN)
                    return MimeState;
                if (is_text)
                    return MIME_HTML;
                return MIME_UNKNOWN;
            case MIME_TEXT:
                if (is_text)
                    return MIME_TEXT;
                return MimeState;
            case MIME_DOC:
            case MIME_XLS:
            case MIME_PPT:
                if (MimeState == MIME_DOC || MimeState == MIME_XLS || MimeState == MIME_PPT)
                    return m;
                return MimeState;
            case MIME_DOCX:
            case MIME_XLSX:
            case MIME_PPTX:
                if (MimeState == MIME_DOCX || MimeState == MIME_XLSX || MimeState == MIME_PPTX)
                    return m;
                return MimeState;
            case MIME_JAVASCRIPT:
            case MIME_JSON:
            case MIME_XML:
                if (MimeState != MIME_UNKNOWN)
                    return MimeState;
                return m;
            case MIME_APK:
                if (MimeState == MIME_ARCHIVE)
                    return m;
                return MimeState;
            case MIME_TEX:
                if (TexKnownCounter > 1 || TexKnownCounter > 0 && TexUnknownCounter > 1)
                    return m;
                if (MimeState == MIME_UNKNOWN && is_text)
                    return MIME_TEXT;
                return MimeState;
            case MIME_IMAGE_SVG:
                return MimeState == MIME_XML ? MIME_IMAGE_SVG : MimeState;
            case MIME_IMAGE_PNM:
                return (MimeState == MIME_UNKNOWN || MimeState == MIME_TEXT) && CanBePNM ? MIME_IMAGE_PNM : MimeState;
            default:
                return MimeState;
        };
    }

    inline size_t ReadSize() const {
        return BytesRead;
    }

    /*
       Detects "binary garbage", analyzing frequencies of non-printable characters, delimiters and broken utf-8 characters.
       Returns true if data consists of readable characters, grouped in words, that are not too long (plaintext, html, xml, css, js, etc.)
    */
    bool IsText() const;

    size_t GetNumHtmlTags() const {
        return NumTags;
    }

    void PrintByteStat() const;

    void SetParameters(size_t max_bytes, unsigned utf_errors, unsigned non_printable, unsigned delimiters, unsigned htmltags) {
        MaxBytesToRead = max_bytes;
        Utf8ErrorsRatio = utf_errors;
        NonPrintableRatio = non_printable;
        DelimitersRatio = delimiters;
        HtmltagRatio = htmltags;
    }

protected:
    bool FeedDetector(const unsigned char* p, const unsigned char* pe);
    bool FeedBinDetector(const unsigned char* p, const unsigned char* pe);
    void FeedTextDetector(const unsigned char* p, const unsigned char* pe);
    void Result(const MimeTypes t) {
        MimeState = t;
    }

protected:
    size_t MaxBytesToRead;
    unsigned Utf8ErrorsRatio;
    unsigned NonPrintableRatio;
    unsigned DelimitersRatio;
    unsigned HtmltagRatio;

    static const size_t MAX_BYTES_TO_READ_DEF = 30 << 10;
    static const unsigned UTF8_ERRORS_RATIO_DEF = 150;
    static const unsigned NON_PRINTABLE_RATIO_DEF = 100;
    static const unsigned DELIMITERS_RATIO_DEF = 80;
    static const unsigned HTMLTAG_RATIO_DEF = 200;

    int CsBin, CsText; // FSM state
    MimeTypes MimeState;
    size_t BytesRead;
    bool NeedMore, NeedMoreBin;

    int Cnt;
    size_t Utf8Bytes, Utf8Chars;
    size_t NumTags;
    int Bom;
    int ByteStat[4], ByteStat16[2 * 4];
    unsigned char Ch;
    int TexKnownCounter = 0;
    int TexUnknownCounter = 0;
    bool CanBePNM = false;
};
