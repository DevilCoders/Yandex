#pragma once

#include <util/generic/ptr.h>

namespace NSnippets {

    class IRestr;
    class TSentsMatchInfo;
    class TSentMultiword;
    class TWordSpanLen;

    /**
     * Universal candidate generator
     *
     * Works in 4 stages by iterating sents from unpacked text:
     * 1. Build candidate from whole sent (whole sent candidates)
     * 2. Add next sent begin if 1. was successful
     * 3. Candidates with boundaries on terminals (terminal candidates)
     * 4. Candidates with boundaries on words (span candidates)
     *
     * Stages 3 and 4 works only if 1 and 2 failed (sentence too long)
     * Stage 4 build only candidates with matches, that wasn't built on stage 3.
     */
    class TUniSpanIter {
    public:
        typedef NSnippets::TSentMultiword TWordpos;

    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        TUniSpanIter(const TSentsMatchInfo& matchInfo, const IRestr& restr, const IRestr& skipRestr, const TWordSpanLen& wordSpanLen, int start = -1, int stop = -1);
        ~TUniSpanIter();
        TWordpos GetFirst() const;
        TWordpos GetLast() const;
        const IRestr& GetSkipRestr() const;
        const TWordSpanLen& GetWordSpanLen() const;
        bool MoveNext();
        void Reset(float maxSize);
    };
}

