#pragma once

#include "base.h"

#include <kernel/remorph/text/textprocessor.h>
#include <kernel/remorph/tokenizer/tokenizer.h>

namespace NRemorphAPI {

namespace NImpl {

class TProcessor: public TBase, public IProcessor {
private:
    NText::TTextFactProcessorPtr Processor;
    NToken::TTokenizeOptions Opts;
    TLangMask Lang;
public:

    TProcessor(const NText::TTextFactProcessorPtr& pr, const NToken::TTokenizeOptions& opts, const TLangMask& lng);

    IResults* Process(const char* text) const override;
};


} // NImpl

} // NRemorphAPI
