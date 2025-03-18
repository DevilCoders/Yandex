#pragma once

#include "phone.h"
#include "telfinder.h"

////
//  TTextTelProcessor
////

class TTextTelProcessor {
private:
    TVector<TToken> Tokens;
    TFoundPhones FoundPhones;
    const TTelFinder* TelFinder;

public:
    TTextTelProcessor(const TTelFinder* telFinder);
    virtual ~TTextTelProcessor();

    TPhone NormalizePhone(const TUtf16String& wtext);

    void DeletePhones();
    void GetFoundPhones(TVector<TFoundPhone>& phones);

    const TVector<TToken>& GetTokens() const {
        return Tokens;
    }

    void ProcessText(const TUtf16String& text);
};
