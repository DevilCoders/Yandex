#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/hash.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/charset/wide.h>
#include <util/folder/dirut.h>
#include <library/cpp/charset/recyr.hh>

#include "matchers/ref_charmap.h"

namespace NRefSequences {

    struct TWhitespaceFilter {
        bool operator()(char c) const {
            return c != ' ' && c != '-';
        }
    };

    struct TAlphaNumericFilter {
        bool operator()(char c) const {
            return csYandex.IsAlnum(c);
        }
    };

    struct TRefLetterFilter {
        bool operator()(char c) const {
            return NRefSequences::IsLetter(c);
        }
    };

    template<typename TTokenFilter>
    class TTrigramIterator {
    public:
        explicit TTrigramIterator(TStringBuf yndBuf)
            : YndBuf(yndBuf)
        {
            while (Pos < YndBuf.size() && !Fltr(YndBuf[Pos])) {
                ++Pos;
            }
            AdvanceRight();
        }

        bool AtEnd() const {
            return Pos >= YndBuf.size();
        }

        void Next() {
            Prev = YndBuf[Pos];
            Pos = Right;
            AdvanceRight();
        }

        ui32 Get() const {
            Y_ASSERT(!AtEnd());
            char next = Right >= YndBuf.size() ? 'E' : YndBuf[Right];
            return MakeUiGramm(Prev, YndBuf[Pos], next);
        }
    private:
        void AdvanceRight() {
            if (AtEnd()) {
                return;
            }
            for (Right = Pos + 1; Right < YndBuf.size(); ++Right) {
                if (Fltr(YndBuf[Right])) {
                    break;
                }
            }
        }

        ui32 MakeUiGramm(ui8 a, ui8 b, ui8 c) const {
            using NRefSequences::NPrivate::charmap;
            using NRefSequences::IsLetter;
            Y_ASSERT(IsLetter(a) && IsLetter(b) && IsLetter(c));
            ui32 res = charmap[a] * 71 * 71 + charmap[b] * 71 + charmap[c];
            Y_ASSERT(res < 71 * 71 * 71);
            return res;
        }
    private:
        TTokenFilter Fltr;
        TStringBuf YndBuf;
        ui32 Pos = 0;
        ui32 Right = 0;
        char Prev = 'S';
    };


} // namespace NRefSequences


