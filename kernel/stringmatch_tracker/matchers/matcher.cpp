#include "matcher.h"

#include <util/string/split.h>

namespace NSequences {

// ======================= LCSMatcher ================================

void TLCSMatcher::ExtendSA(const ui8 ch) {
    const size_t currentStateNum = Size++;

    States[currentStateNum].Clear();
    States[currentStateNum].Length = States[LastStateNum].Length + 1;

    size_t previous = LastStateNum;
    size_t prevPrevious = previous;
    bool noMatch = false;
    while (!(noMatch || States[previous].Next[ch])) {
        States[previous].Next[ch] = currentStateNum;
        prevPrevious = previous;
        previous = States[previous].Link;
        noMatch = (previous == prevPrevious);
    }

    if (noMatch) {
        States[currentStateNum].Link = 0;
        LastStateNum = currentStateNum;
        return;
    }

    const size_t linkDest = States[previous].Next[ch];

    if (States[previous].Length + 1 == States[linkDest].Length) {
        States[currentStateNum].Link = linkDest;
    } else {
        const size_t cloneStateNum = Size++;

        States[cloneStateNum] = States[linkDest];
        States[cloneStateNum].Length = States[previous].Length + 1;

        noMatch = false;
        while (!noMatch && States[previous].Next[ch] == linkDest) {
            States[previous].Next[ch] = cloneStateNum;
            prevPrevious = previous;
            previous = States[previous].Link;
            noMatch = (prevPrevious == previous);
        }
        States[linkDest].Link = States[currentStateNum].Link = cloneStateNum;
    }
    LastStateNum = currentStateNum;
}

TLCSMatcher::TLCSMatcher(const TString& request, size_t threshold /* = 1 */)
    : Threshold(threshold)
{
    Y_ASSERT(threshold >= 1);

    if (request.size() < threshold) {
        return;
    }

    States.resize(request.size() * 2);
    States[0].Clear();

    for (size_t pos = request.size(); pos != 0; --pos) {
        const ui8 ch = NPrivate::charmap[ui8(request[pos - 1])];
        if (ch && ch <= AlphabetSize) {
            ExtendSA(ch - 1); // 0-based indexing
        }
    }
}

void TLCSMatcher::ExtendMatch(char c) {
    if (Size <= Threshold)
        return;

    ui8 ch = NPrivate::charmap[ui8(c)];
    if (!ch || ch > AlphabetSize)
        return;

    --ch; // switching to 0-based indexing

    while (CurState && !States[CurState].Next[ch]) {
        CurState = States[CurState].Link;
        CurLength = States[CurState].Length;
    }
    if (States[CurState].Next[ch]) {
        CurState = States[CurState].Next[ch];
        ++CurLength;
    }
    if (CurLength > BestLength) {
        BestLength = CurLength;
    }
}

// ======================= TrigramMatcher ================================

void TTrigramMatcher::DoFill(const TStringBuf& str) {
    if (str.size() < 3)
        return;

    ui32 hashValue = 0;
    for (const char symbol : str) {
        const ui8 ch = NPrivate::charmap[ui8(symbol)];
        if (!ch)
            continue;

        hashValue <<= 20;
        hashValue >>= 14;
        hashValue += ch;

        if (hashValue < (1 << 12))
            continue;

        SetBit(hashValue);
    }
}

void TTrigramMatcher::FillFromRequest(const TString& request, bool fillByWords) {
    TStringBuf words(request);

    if (!fillByWords) {
        DoFill(words);
        return;
    }

    while (words) {
        DoFill(words.NextTok(' '));
    }
}

// ======================= TExtendedTrigramMatcher =============================

void TExtendedTrigramMatcher::InitFromRequest(const char* const beg, const char* const end) {
    QueryUniqTrigrams = 0;
    Clear();

    for (const char* pos = end - 1; pos >= beg; --pos) {
        ExtendMatch(*pos);
        if (!FindMatch()) {
            SetBit(GetCurrentMatch());
            ++QueryUniqTrigrams;
        }
    }
}

void TExtendedTrigramMatcher::AddText(const char* const beg, const char* const end, ui32* const trigramMatch) {
    ResetMatch();
    const char* lastMatchPos = end + 10;

    *trigramMatch = 0;

    for (const char* pos = end - 1; pos >= beg; --pos) {
        ExtendMatch(*pos);
        ui32 currentMatch = GetCurrentMatch();
        bool inQuery = false;
        if (FindMatch()) {
            *trigramMatch += ::Min<ui32>(ui32(3), lastMatchPos - pos);
            lastMatchPos = pos;

            inQuery = true;
        }

        if (!TextMatcher.GetBit(currentMatch)) {
            ++TextUniqTrigrams;
            TextMatcher.SetBit(currentMatch);

            if (inQuery) {
                ++MatchedTrigrams;
            }
        }
    }
}

} // namespace NSequences
