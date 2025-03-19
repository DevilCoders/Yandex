#pragma once

#include "sentence.h"
#include "base.h"

#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/charset/wide.h>

namespace NRemorphAPI {

namespace NImpl {

class TResults: public TBase, public IResults {
private:
    TVector<TIntrusivePtr<TSentenceData>> Sentences;
    TLocker Lock; // We need to lock TProcessor object in order to keep protobuf types in the memory

public:
    TResults(const IBase* parent);

    void Add(const TUtf16String& text, TVector<NFact::TFactPtr>& facts, NText::TWordSymbols& tokens);

    unsigned long GetSentenceCount() const override;
    ISentence* GetSentence(unsigned long num) const override;
};


} // NImpl

} // NRemorphAPI
