#pragma once

#include <util/generic/algorithm.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/charset/recyr.hh>

#include <kernel/lemmer/dictlib/grammar_index.h>

typedef TUtf16String TWtringType;
typedef TString TStringType;

namespace NDictBuild {
    void DetachGrammar(TWtringType& w, TStringType& grammar, bool verbose) {
        char tokenBuffer[256];
        char grammarBuffer[256];

        char* tokenPtr = tokenBuffer;
        char* grammarPtr = grammarBuffer;

        TWtringType::const_iterator it = w.begin();
        TWtringType::const_iterator jt = w.end();

        // Извлекаем граммемы.
        for (/**/; it != jt; ++it) {
            char c = WideCharToYandex[*it];

            if ((c == ',' || c == '=' || c == ' ') && tokenPtr != tokenBuffer) {
                *tokenPtr = 0;

                EGrammar grammarCode = TGrammarIndex::GetCode(tokenBuffer);

                if (grammarCode != gInvalid) {
                    *grammarPtr++ = grammarCode;
                } else if (verbose) {
                    Cerr << "Invalid grammar feature \"" << CharToWide(TStringBuf(tokenBuffer, tokenPtr), csYandex) << "\"\n";
                }

                tokenPtr = tokenBuffer;
            } else {
                *tokenPtr++ = c;

                if (size_t(tokenPtr - tokenBuffer) >= size_t(Y_ARRAY_SIZE(tokenBuffer) - 1) && verbose) {
                    Cerr << "Too long grammar feature \"" << CharToWide(TStringBuf(tokenBuffer, tokenPtr), csYandex) << "...\"\n";
                }
            }
        }

        // Извлекаем граммемы: осталась часть строки от последнего разделителя до конца.
        if (tokenPtr != tokenBuffer) {
            *tokenPtr = 0;

            EGrammar grammarCode = TGrammarIndex::GetCode(tokenBuffer);

            if (grammarCode != gInvalid) {
                *grammarPtr++ = grammarCode;
            } else if (verbose) {
                Cerr << "Invalid grammar feature \"" << CharToWide(TStringBuf(tokenBuffer, tokenPtr), csYandex) << "\"\n";
            }

            tokenPtr = tokenBuffer;
        }

        // Грамматика должна быть непустой.
        if (grammarPtr == grammarBuffer && verbose) {
            Cerr << "No grammar categories assigned to one of the forms; entry \"\n";
        }

        // Сортируем, удаляем ненужное.
        Sort(grammarBuffer, grammarPtr, std::less<unsigned char>());
        grammarPtr = Unique(grammarBuffer, grammarPtr);

        for (size_t i = 0, j = grammarPtr - grammarBuffer; i < j; ++i) {
            unsigned char grammeme = grammarBuffer[i];

            if (grammeme <= gBefore || grammeme >= gMax) {
                char hex[3];
                hex[0] = (grammeme / 16 < 10) ? (grammeme / 16 + '0') : (grammeme / 16 - 10 + 'a');
                hex[1] = (grammeme % 16 < 10) ? (grammeme % 16 + '0') : (grammeme % 16 - 10 + 'a');
                hex[2] = 0;

                if (verbose) {
                    Cerr << "Invalid grammar code 0x" << hex << "; entry \"\n";
                }
            }
        }

        grammar = TStringType(grammarBuffer, grammarPtr - grammarBuffer);
    }
}
