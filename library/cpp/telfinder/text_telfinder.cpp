#include "text_telfinder.h"
#include "simple_tokenizer.h"
#include "phone_collect.h"

#include <library/cpp/deprecated/iter/vector.h>

TTextTelProcessor::TTextTelProcessor(const TTelFinder* telFinder)
    : TelFinder(telFinder)
{
}

TTextTelProcessor::~TTextTelProcessor() {
}

TPhone TTextTelProcessor::NormalizePhone(const TUtf16String& wtext) {
    ProcessText(wtext);

    if (FoundPhones.empty())
        return TPhone();

    TPhone phone = FoundPhones.front().Phone;

    FoundPhones.clear();
    Tokens.clear();

    return phone;
}

void TTextTelProcessor::DeletePhones() {
    FoundPhones.clear();
    Tokens.clear();
}

void TTextTelProcessor::GetFoundPhones(TVector<TFoundPhone>& phones) {
    phones = FoundPhones;
}

void TTextTelProcessor::ProcessText(const TUtf16String& text) {
    Tokens = TSimpleTokenizer::BuildTokens(text);
    TPhoneCollector coltr(FoundPhones);
    TelFinder->FindPhones(NIter::TVectorIterator<TToken>(Tokens), coltr);
}
