#pragma once

#include <library/cpp/html/face/event.h>
#include <util/stream/output.h>
#include "attrs.h"

namespace NHtmlDetect {
    struct TMark {
        enum {
            Markup = 0, // markup, ignored by parser; content preserved
            Tag = 1,    // start tag, tag id must follow immediately, see tags.gperf
            EndTag = 2, // end of tag
            End = 3,    // end of markup, plain text follows immediately
            Name = 4,   // marks start of attrbibute name, that lasts until StartValue
            Value = 5,  // ends name and starts value; must follow StartAttr; value last until next StartAttr or End
            Attr = 6,   // known attribute id follows
        };
    };

    class TWriter {
    private:
        IOutputStream* OutStream;

    private:
        static bool IsAttrChar(char letter) {
            if ((letter >= 'a' && letter <= 'z') || (letter >= '0' && letter <= '9') || letter == '_' || letter == '-')
                return true;
            return false;
        }
        // is there plain ascii version in util?
        static char ToLower(char c) {
            static const char shift = 'a' - 'A';
            if (c >= 'A' && c <= 'Z')
                c += shift;
            return c;
        }

        void WriteMarkup(const char* text, size_t len) {
            if (len > 2) {
                *OutStream << char(TMark::Markup);
                WriteText(text + 1, len - 2);
                *OutStream << char(TMark::End);
            }
        }
        // write text with no control chars
        void WriteText(const char* text, size_t len) {
            // record all symbols, replacing all control chars with space
            const char* const end = text + len;
            while (text < end) {
                unsigned char letter = *text;
                if (letter < 0x20 && letter != 0x0a && letter != 0x0d)
                    letter = ' ';
                OutStream->Write(letter);
                ++text;
            }
        }
        // write proper attribute name in lowercase
        void WriteName(const char* text, size_t len) {
            const NHtml::TAttr* pattr = NHtml::FindAttrPtr(text, len);
            if (pattr) {
                OutStream->Write(TMark::Attr);
                OutStream->Write(pattr->Id);
            } else {
                const char* const end = text + len;
                while (text < end) {
                    const char letter = ToLower(*text);
                    if (IsAttrChar(letter))
                        OutStream->Write(letter);
                    ++text;
                }
            }
        }
        void WriteTagStart(const THtmlChunk& e) {
            *OutStream << char(TMark::Tag) << (unsigned char)e.Tag->id();

            for (size_t i = 0; i < e.AttrCount; ++i) {
                const NHtml::TAttribute& attr = e.Attrs[i];
                if (!attr.Name.Leng) // killed by conf rewrite
                    continue;

                *OutStream << char(TMark::Name);
                WriteName(e.text + attr.Name.Start, attr.Name.Leng);
                *OutStream << char(TMark::Value);
                WriteText(e.text + attr.Value.Start, attr.Value.Leng);
            }
            *OutStream << char(TMark::End);
        }
        void WriteTagEnd(const THtmlChunk&) {
            *OutStream << char(TMark::EndTag);
        }

    public:
        TWriter(IOutputStream* stream)
            : OutStream(stream)
        {
        }
        HTLEX_TYPE WriteEvent(const THtmlChunk& e) {
            if (e.flags.type == PARSED_TEXT) {
                WriteText(e.text, e.leng);
            } else if (e.flags.type == PARSED_MARKUP)

            {
                // regular tree markup
                if (e.Tag && e.Tag->id() && e.flags.markup != MARKUP_IGNORED) {
                    if (e.flags.apos == HTLEX_START_TAG) {
                        WriteTagStart(e);
                        return HTLEX_START_TAG;
                    } else if (e.flags.apos == HTLEX_END_TAG) {
                        WriteTagEnd(e);
                        return HTLEX_END_TAG;
                    } else {
                        WriteTagStart(e);
                        WriteTagEnd(e);
                    }
                }
                // ignored markup with known tag name, needed to support irreg microforms
                else if (e.Tag && e.Tag->id() && e.flags.markup == MARKUP_IGNORED) {
                    if (e.flags.apos == HTLEX_START_TAG)
                        WriteTagStart(e);
                    else if (e.flags.apos == HTLEX_END_TAG)
                        WriteTagEnd(e);
                    else {
                        WriteTagStart(e);
                        WriteTagEnd(e);
                    }
                }
                // other markup
                else {
                    WriteMarkup(e.text, e.leng);
                }
            }
            return HTLEX_EOF;
        }
    };

}
