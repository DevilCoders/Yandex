#include "sanitize.h"

#include <library/cpp/html/entity/htmlentity.h>

namespace NHtmlXPath {

    void CompressWhiteSpace(TString& text) {
        // at the beginning of the string, we pretend to have written the space
        // in order to eliminate it altogether
        bool writtenWhiteSpace = true;

        TString::iterator target = text.begin();

        for (TString::const_iterator source = text.begin();
             source != text.end();
             ++source)
        {
            switch (ui8(*source)) {
                case 0xC2: {
                        if (ui8(*(source + 1)) == 0xA0) { //< UTF-8 0xC2A0 NO-BREAK SPACE
                            ++source;
                        } else {
                            *target++ = *source;
                            writtenWhiteSpace = false;
                            break; //< don't fall through
                        }

                        [[fallthrough]];
                    }
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    {
                        if (! writtenWhiteSpace) {
                            *target++ = ' ';
                            writtenWhiteSpace = true;
                        }
                        break;
                    }
                default: {
                        *target++ = *source;
                        writtenWhiteSpace = false;
                    }
            }
        }

        // remove the space at the end, if any
        if (target != text.begin() && writtenWhiteSpace) {
            --target;
        }

        text.resize(target - text.begin());
    }

    void DecodeHTMLEntities(TString& str) {
        // Convert HTML entities, if any
        if (str.find('&') != TString::npos) {
            TString recoded = TString::Uninitialized(str.size() * 3); //< recoded entities take space
            TStringBuf decoded = HtTryEntDecodeAsciiCompat(str, recoded.begin(), recoded.size());
            if (decoded.begin() != str.begin()) { //< replaced something
                recoded.resize(decoded.size());
                str = recoded;
            }
        }
    }

} // namespace NHtmlXPath

