#include "strip.h"

#include <library/cpp/html/pcdata/pcdata.h>
#include <util/string/strip.h>

#include <library/cpp/html/face/onchunk.h>
#include <library/cpp/html/html5/parse.h>
#include <library/cpp/html/spec/lextype.h>

namespace {
    struct THtmlStripperHandler: public IParserResult {
        THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) final {
            if (chunk.Tag) {
                OnTag(chunk);
            } else {
                OnText(chunk);
            }

            return nullptr;
        }

        void OnText(const THtmlChunk& chunk) {
            Part += TStringBuf(chunk.text, chunk.leng);
        }

        void OnTag(const THtmlChunk&) {
            AddPart();
        }

        void AddPart() {
            Part = StripString(Part);

            if (Part.size()) {
                if (Result.size()) {
                    Result.append(" ", 1);
                }

                Result.append(Part);
                Part.clear();
            }
        }

        static inline TString StripHtml(const TStringBuf& src) {
            THtmlStripperHandler handler;

            NHtml5::ParseHtml(src, &handler);
            handler.AddPart();

            return DecodeHtmlPcdata(handler.Result);
        }

        TString Part;
        TString Result;
    };
}

TString NHtml::StripHtml(const TStringBuf& input) {
    return THtmlStripperHandler::StripHtml(input);
}
