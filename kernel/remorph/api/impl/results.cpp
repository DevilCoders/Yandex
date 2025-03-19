#include "results.h"
#include "sentence.h"

namespace NRemorphAPI {

namespace NImpl {

TResults::TResults(const IBase* parent)
    : Lock(parent)
{
}

void TResults::Add(const TUtf16String& text, TVector<NFact::TFactPtr>& facts, NText::TWordSymbols& tokens) {
    Sentences.push_back(new TSentenceData());
    Sentences.back()->Text = WideToUTF8(text);
    DoSwap(Sentences.back()->Facts, facts);
    DoSwap(Sentences.back()->Tokens, tokens);
}

unsigned long TResults::GetSentenceCount() const {
    return Sentences.size();
}

ISentence* TResults::GetSentence(unsigned long num) const {
    return num < Sentences.size() ? new TSentence(this, *Sentences[num]) : nullptr;
}

} // NImpl

} // NRemorphAPI
