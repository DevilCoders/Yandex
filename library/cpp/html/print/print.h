#pragma once

#include <library/cpp/html/face/onchunk.h>

#include <util/generic/ptr.h>
#include <util/stream/output.h>

namespace NHtml {
    class TStorage;

    struct TPrintConfig {
        int Indent = 1;
        char Char = '\t';
        bool Format = true;
        // Strip original spaces. Applied only if Format == false.
        bool Strip = true;
        // output comments
        bool Comments = false;
    };

    class THtmlPrinter: public IParserResult {
    public:
        THtmlPrinter(IOutputStream& out, const TPrintConfig& config = TPrintConfig());
        ~THtmlPrinter() override;

        THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override;

    private:
        class TImpl;
        THolder<TImpl> Impl_;
    };

    TString PrintHtml(const TStorage& html, const TPrintConfig& config = TPrintConfig());

}
