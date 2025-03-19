#pragma once

#include <library/cpp/numerator/numerate.h>
#include <library/cpp/charset/wide.h>

#include "date_recognizer.h"

class TDateRecognizerHandler : public INumeratorHandler
{
private:
    TStreamDateRecognizer* Recognizer;
    static const size_t BUFFER_SIZE = 65536;
    char Buffer[BUFFER_SIZE];

    void Push(const wchar16* token, size_t len) {
        const size_t LOCAL_BUFFER = BUFFER_SIZE;

        if (Recognizer) {
            len = Min(len, LOCAL_BUFFER);
            WideToChar(token, len, Buffer, CODES_YANDEX);
            Recognizer->Push(Buffer, len);
        }
    }

public:
    TDateRecognizerHandler()
        : Recognizer(nullptr)
    {
    }

    void SetRecognizer(TStreamDateRecognizer* recognizer) {
        Recognizer = recognizer;
    }

    void OnTokenStart(const TWideToken& tok, const TNumerStat&) override {
        Push(tok.Token, tok.Leng);
    }

    void OnSpaces(TBreakType , const wchar16 *token, unsigned len, const TNumerStat&) override {
        Push(token, len);
    }
};
