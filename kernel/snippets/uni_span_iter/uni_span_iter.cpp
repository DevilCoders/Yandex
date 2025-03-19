#include "uni_span_iter.h"
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/sent_match.h>

#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_info/sentword.h>

#include <kernel/snippets/config/config.h>

namespace NSnippets {

    typedef NSnippets::TSentMultiword TWordpos;

    /**
     * Set of bool flags and stored between candidates values.
     */
    struct TState
    {
        bool ReachedEnd;                /**< reached end of unpacked text, no more candidates should be produced */
        bool SentsFilled;               /**< no more whole sents can fit current candidate */
        int SentsSeen;                  /**< how many whole sents already fits in current candidate */
        bool TerminalsFilled;           /**< candidate boundaries set to terminals or they could not
                                             be set in CheckTerminals stage */
        bool Reverted;                  /**< there were matches in sent between end of previous candidate
                                             and new start position */
        bool CheckTerminalsStage;       /**< true if CheckTerminals stage is active, otherwise CheckSpans
                                             stage is active */
        bool ProcessingSent;            /**< true if still can build candidates from current sent,
                                             if false - must switch to next one */
        bool FilledFromCurWord;         /**< candidate from current start position filled, have to
                                             move to next start position */
        bool CheckSpansFirstWordSeen;   /**< hack flag for CheckSpans stage: not to miss first word in sent
                                             in it's match */
        TWordpos TmpFirst;              /**< remember seen starting position, need for CheckTerminals revert mode
                                             and for CheckSpans */

        TState(const TWordpos& p) : TmpFirst(p)
        {
            Reset();
        }

        /**
         * Sets all flags to default values. Should be done when switching to next sent.
         */
        void Reset()
        {
            ReachedEnd = false;
            SentsFilled = false;
            SentsSeen = 0;
            TerminalsFilled = false;
            Reverted = false;
            CheckTerminalsStage = true;
            ProcessingSent = true;
            FilledFromCurWord = false;
            CheckSpansFirstWordSeen = false;
        }
    };

    // set of boundary new value functors
    /**
     *  Returns next terminal in sent or the same one, if current terminal or last word in sent
     */
    struct TGetNextTerminal
    {
        const TSentsMatchInfo& MatchInfo;

        TGetNextTerminal(const TSentsMatchInfo& mInfo) : MatchInfo(mInfo) {}

        TWordpos operator()(const TWordpos& pos) const
        {
            int nextWordId = pos.LastWordId() + 1;
            if (nextWordId >= MatchInfo.SentsInfo.WordCount()) {
                return pos;
            }
            TWordpos tmp = pos;
            if (MatchInfo.IsWordAfterTerminal(nextWordId) && !tmp.IsLastInSent()) {
                ++tmp;
            }
            nextWordId = tmp.LastWordId() + 1;
            while (!tmp.IsLastInSent() && !MatchInfo.IsWordAfterTerminal(nextWordId)) {
                ++tmp;
                nextWordId = tmp.LastWordId() + 1;
            }
            return tmp;
        }
    };

    /**
     * Returns next match or current one if it's last in sent
     */
    struct TGetNextMatch
    {
        const TSentsMatchInfo& MatchInfo;

        TGetNextMatch(const TSentsMatchInfo& mInfo) : MatchInfo(mInfo) {}

        TWordpos operator()(const TWordpos& pos) const
        {
            if (pos.IsLastInSent()) {
                return pos;
            }
            TWordpos tmp = pos;
            ++tmp;
            while (!MatchInfo.IsMatch(tmp) && !tmp.IsLastInSent()) {
                ++tmp;
            }

            if (!MatchInfo.IsMatch(tmp) && tmp.IsLastInSent()) {
                return pos;
            }
            return tmp;
        }
    };

    /**
     * Returns previous match or current one if it's first in sent
     */
    struct TGetPrevMatch
    {
        const TSentsMatchInfo& MatchInfo;

        TGetPrevMatch(const TSentsMatchInfo& mInfo) : MatchInfo(mInfo) {}

        TWordpos operator()(TWordpos& pos) const
        {
            if (pos.IsFirstInSent()) {
                return pos;
            }
            TWordpos tmp = pos;
            --tmp;
            while(!MatchInfo.IsMatch(tmp) && !tmp.IsFirstInSent()) {
                --tmp;
            }
            if (!MatchInfo.IsMatch(tmp) && tmp.IsFirstInSent()) {
                return pos;
            }
            return tmp;
        }
    };

    /**
     * Returns previous word
     */
    struct TGetPrev
    {
        TWordpos operator()(const TWordpos& p) const
        {
            return p.Prev();
        }
    };

    /**
     * Return next word
     */
    struct TGetNext
    {
        TWordpos operator()(const TWordpos& p) const
        {
            return p.Next();
        }
    };

    class TUniSpanIter::TImpl {
    private:
        const TSentsMatchInfo& MatchInfo;       /**< Container for matches, terminals, sent boundaries */
        const TWordSpanLen& SpanLen;            /**< Candidate length calculator */
        bool BeforeFirst;                       /**< Flag for first run */
        float MaxLength;                        /**< Candidate max length */

        TWordpos First;                         /**< Candidate start boundary */
        TWordpos Last;                          /**< Candidate end boundary */
        const int StartSent;                    /**< Sent to start building cands */
        int CurrentSent;                        /**< Current processing sent number */
        bool CurSentHasMatches;                 /**< If current processing sent has matches */
        TState CurrentState;                    /**< Set of flags to manage candidate building stages */
        TVector<int> SeenMatches;               /**< Seen matches in sent */
        const int EndSent;                      /**< Last sent to build cands */

        const TGetNextTerminal GetNextTerminal; /**< Functor instance */
        const TGetNextMatch GetNextMatch;       /**< Functor instance */
        const TGetPrevMatch GetPrevMatch;       /**< Functor instance */
        const TGetNext GetNext;                 /**< Functor instance */
        const TGetPrev GetPrev;                 /**< Functor instance */
        const IRestr& Restriction;
        const IRestr& SkipRestr;

        /**
         * Boundary modifier.
         * Boundary changes until max length exceeded or stop param reached
         * @param op boundary modifier, must be one of boundary value functors
         * @param p modifyable boundary, must be First or Last
         * @param stop limits boundary change
         * @return if boundary was changed
         */
        template<typename TOp>
        bool MoveUntil(const TOp& op, TWordpos& p, const TWordpos& stop)
        {
            Y_ASSERT(&p == &First || &p == &Last);
            TWordpos initPos = p;
            while (p != stop) {
                TWordpos prevPos = p;
                p = op(p);
                if (!CurrentCandidateFitsInLength()) {
                    p = prevPos;
                    break;
                }
            }
            return p != initPos;
        }

        /**
         * @param sent
         * @return coordinate of first word in given sent
         */
        TWordpos FirstWordInSent(int sent) const;

        /**
         * @param sent
         * @return coordinate of last word in given sent
         */
        TWordpos LastWordInSent(int sent) const;

        /**
         * Checks if there are matches between given boundaries
         * @param b starting boundary
         * @param e ending boundary
         * @return true if there are matches, false otherwise
         */
        bool HasMatches(const TWordpos& b, const TWordpos& e) const;

        /**
         * Rembers matches coordinates between current First,Last boundaries in current sent.
         * @see InitSeenMatches()
         * @see CheckWords()
         */
        void PushWords();

        /**
         * Checks if there were remembered by PushWords() matches between given boundaries
         * @param beg starting boundary
         * @param end ending boundary
         * @return true if there were remembered matches, false otherwise
         * @see PushWords()
         */
        bool CheckWords(TWordpos beg, TWordpos end) const;

        /**
         * @return true if there no more unpacked sents left
         */
        bool IsReachedEnd() const;

        /**
         * Checks if there are restrictions between lastSent and the beginning of the current span
         * @return true if restriction exists, false otherwise
         */
        bool HasRestr(int lastSent) const;

        // modifying methods
        /**
         * Make attempts to build candidate from whole sents
         * @return true if attempt was successful, false if sent was too long or reached end
         */
        bool AddWholeSent();

        /**
         * Make attempt to add next sent begin to previous whole sent candidate
         * If candidate don't have matches or begin of sent is too short (<3 words) then attempt fails.
         * @return true in case of successful attempt, false otherwise
         */
        bool AddSentBegin();

        /**
         * Sets flags and boundaries for CheckSpans stage
         */
        void SetToCheckSpans();

        /**
         * Moves boundaries in sent after previous terminals candidate was built.
         * @return always false, but changes flags and moves boundaries
         */
        bool CheckTerminalsStateFilled();

        /**
         * Checks if there were matches between new starting boundary and ending boundary of
         * previous candidate. If check was successful - attempts to build candidate from that range
         * @return true if attempt was ssuccessful, false otherwise
         */
        bool CheckTerminalsStateReverted();

        /**
         * Attempts to grow candidate right with some words after checking terminals candidate
         * @return true, if new candidate was generated, false otherwise
         */
        bool CheckTerminalsStateTerminalsFilled();

        /**
         * Check if reached end of sent. Sets proper flags if reached or attempts to build candidate.
         * @return true if candidate built, false otherwise
         */
        bool CheckTerminalsCheckEnd();

        /**
         * Attempts to build candidate with boundaries at terminals within current sent
         * in case if sent is too long and contains matches
         * @return true in case of successful attempt, false otherwise
         */
        bool CheckTerminals();

        /**
         * attempts to build candidate from sent start
         * @return true if new candidate build, false otherwise
         */
        bool AddFirstSentBeg();

        /**
         * Moves boundaries from match to match, checks if there were candidates with same matches and
         * attempts to build ones if they haven't.
         * @return true if new candidate build, false otherwise
         */
        bool CheckSpansStateFilled();

        /**
         * Build span candidates by moving starting and ending boundaires words by word.
         * @return true if new candidate built, false otherwise
         */
        bool CheckSpansStateUnfilled();

        /**
         * Attempts to build span candidates, that couldn't be built in terminals stage.
         * Moves forward from match to match from sent beginning, grows right, and checks if there
         * were candidate with same matches built in terminals stage. If wasn't - builds new candidate.
         * Also checks if sent end was reached and forces to switch to next sent.
         * @return true if new candidate built, false otherwise
         */
        bool CheckSpans();

        /**
         * Resets seen matches when swithcing to new sent
         * @see SwitchSent()
         */
        void InitSeenMatches();

        /**
         * Switches to new sent or sets flag that end of unpacked text is reached.
         */
        void SwitchSent();

        /**
         * Checks if the candidate fit in the given length
         */
        bool CandidateFitsInLength(TWordpos first, TWordpos last) const;

        /**
        * Checks if the current candidate fit in the given length
        */
        bool CurrentCandidateFitsInLength() const;

    public:
        TImpl(const TSentsMatchInfo& matchInfo, const IRestr& restr, const IRestr& skipRestr, const TWordSpanLen& wordSpanLen, int start, int stop);
        TWordpos GetFirst() const;
        TWordpos GetLast() const;
        const IRestr& GetSkipRestr() const;
        const TWordSpanLen& GetWordSpanLen() const;
        bool MoveNext();
        void Reset(float maxSize);
    };

    inline TWordpos TUniSpanIter::TImpl::FirstWordInSent(int sent) const
    {
        const TSentsInfo& sentsInfo = MatchInfo.SentsInfo;
        return TWordpos(sentsInfo.WordId2SentWord(sentsInfo.FirstWordIdInSent(sent)));
    }

    inline TWordpos TUniSpanIter::TImpl::LastWordInSent(int sent) const
    {
        const TSentsInfo& sentsInfo = MatchInfo.SentsInfo;
        return TWordpos(sentsInfo.WordId2SentWord(sentsInfo.LastWordIdInSent(sent)));
    }

    inline bool TUniSpanIter::TImpl::HasMatches(const TWordpos& b, const TWordpos& e) const
    {
        return MatchInfo.MatchesInRange(b, e) > 0;
    }

    void TUniSpanIter::TImpl::PushWords()
    {
        TWordpos tmpf = First;
        TWordpos tmpl = Last;
        if (tmpl < tmpf) {
            DoSwap(tmpf, tmpl);
        }
        if (!MatchInfo.IsMatch(tmpf)) {
            tmpf = GetNextMatch(tmpf);
        }
        if (!MatchInfo.IsMatch(tmpl)) {
            tmpl = GetPrevMatch(tmpl);
        }
        int wordOfs = tmpl.GetFirst().GetWordOfs();
        while (tmpf != tmpl) {
            if (SeenMatches[tmpf.GetFirst().GetWordOfs()] < wordOfs) {
                SeenMatches[tmpf.GetFirst().GetWordOfs()] = wordOfs;
            }

            tmpf = GetNextMatch(tmpf);
        }
        if (tmpf == tmpl) {
            if (SeenMatches[tmpf.GetFirst().GetWordOfs()] < wordOfs) {
                SeenMatches[tmpf.GetFirst().GetWordOfs()] = wordOfs;
            }
        }
    }

    bool TUniSpanIter::TImpl::CheckWords(TWordpos beg, TWordpos end) const
    {
        if (!MatchInfo.IsMatch(beg)) {
            beg = GetNextMatch(beg);
        }
        if (!MatchInfo.IsMatch(end)) {
            end = GetPrevMatch(end);
        }
        return SeenMatches[beg.GetFirst().GetWordOfs()] >= end.GetFirst().GetWordOfs();
    }

    inline bool TUniSpanIter::TImpl::IsReachedEnd() const
    {
        return CurrentState.ReachedEnd;
    }

    inline bool TUniSpanIter::TImpl::HasRestr(int lastSent) const
    {
        if (First.GetSent() == lastSent) {
            return true;
        }
        return !Restriction(lastSent - 1);
    }

    bool TUniSpanIter::TImpl::AddWholeSent()
    {
        // check whether zero or more whole sentences can be added
        int nextSent = First.GetSent() + CurrentState.SentsSeen;
        CurrentState.ReachedEnd = (nextSent > EndSent);
        if (CurrentState.ReachedEnd || !HasRestr(nextSent)) {
            CurrentState.SentsFilled = true; // SentsFilled & Filled set up this way to avoid another attempts
            CurrentState.FilledFromCurWord = true; // to build whole sent cands and add next sent begin
            CurrentState.ProcessingSent = false; // forces to switch current sent
            return false;
        } else {
            Last = LastWordInSent(nextSent);
            // check if fits length & contains matches - return true (have a candidate!) & update SentsSeen
            // if sent too long - go to next stage & set some flags
            if (CurrentCandidateFitsInLength()) {
                ++CurrentState.SentsSeen;
                if (HasMatches(First, Last)) {
                    return true;
                }
            } else {
                // no other tries to build whole sents candidates
                CurrentState.SentsFilled = true;
                // if current sent is long & without matches - avoid building other candidates
                if (CurrentState.SentsSeen == 0 && !CurSentHasMatches) {
                    CurrentState.ProcessingSent = false;
                }
                // setup ending boundary
                Last = FirstWordInSent(nextSent);
                // still try to add next sent begin
            }
            return false;
        }
    }

    bool TUniSpanIter::TImpl::AddSentBegin()
    {
        // stupid check if candidate is too long
        if (!CurrentCandidateFitsInLength() || Last.IsLastInSent()) {
            CurrentState.ProcessingSent = false; // forces to switch current sent
            return false;
        }
        // check if AddSentBegin() has already attempted to build candidate ending with terminals
        if (!CurrentState.TerminalsFilled) {
            // try to find most distant terminal
            CurrentState.TerminalsFilled = true; // there was attempt to build candidate ending with terminal
            if (!MoveUntil(GetNextTerminal, Last, Last.LastInSameSent()) ||
                Last.GetLast().GetWordOfs() < 3 || Last.IsLastInSent()) {
                // attempt failed (too long or too short candidate), reset Last boundary to sent start
                Last = FirstWordInSent(Last.GetSent());
                return false;
            } else {
                // attempt succeded, got a candidate!
                if (HasMatches(First, Last)) {
                    return true;
                }
            }
        } else {
            CurrentState.FilledFromCurWord = true;
            CurrentState.ProcessingSent = false;
            if (!MoveUntil(GetNext, Last, Last.LastInSameSent())) {
                return false;
            }
            if (Last.GetLast().GetWordOfs() >= 3) {
                if (HasMatches(First, Last)) {
                    return true;
                }
            }
            return false;
        }
        return false;
    }

    inline void TUniSpanIter::TImpl::SetToCheckSpans()
    {
        // set boundaries to sent start
        First = FirstWordInSent(CurrentSent);
        Last = First;
        // switch to next stage
        CurrentState.CheckTerminalsStage = false;
        CurrentState.FilledFromCurWord = true; // start condition for CheckSpans stage
    }

    bool TUniSpanIter::TImpl::CheckTerminalsStateFilled()
    {
        TWordpos end = LastWordInSent(CurrentSent);
        // reached end of current sent
        if (Last == end) {
            SetToCheckSpans(); // go to next stage
            return false;
        }

        // check if previous candidate was reverted (ending with terminal instead of starting with one)
        if (CurrentState.Reverted) {
            // restore boundaries & reset reverted flag
            First = CurrentState.TmpFirst;
            Last = First;
            CurrentState.Reverted = false;
        } else {
            // switch to next terminal in sent
            First = GetNextTerminal(First);
            if (First != end) {
                // was set before terminal - let's set boundary just after it
                ++First;
            } else {
                SetToCheckSpans(); // go to next stage
                return false;
            }
        }

        // if don't have matches further in sentence - switch to stage2
        // else continue moving through sentence
        if (HasMatches(Last, end)) {
            // reset flags to build candidate in next iteration
            CurrentState.FilledFromCurWord = false;
            CurrentState.TerminalsFilled = false;
        } else {
            SetToCheckSpans(); // go to next stage
        }
        return false;
    }

    bool TUniSpanIter::TImpl::CheckTerminalsStateReverted()
    {
        if (HasMatches(Last, First)) {
            CurrentState.TmpFirst = First;
            Last = First;
            First = GetPrevMatch(First);

            if (CurrentCandidateFitsInLength()) {
                TWordpos begin = FirstWordInSent(CurrentSent);
                MoveUntil(GetPrev, First, begin);
                PushWords();
                CurrentState.Reverted = true; // restore remembered starting position in next iteration
                CurrentState.FilledFromCurWord = true; // candidate was filled - go to proper branch of checks
                return true;
            } else {
                // candidate was too long, restore boundaries
                First = CurrentState.TmpFirst;
                Last = First;
                return false;
            }
        } else {
            // set ending boundary to new position
            Last = First;
            return false;
        }
    }

    bool TUniSpanIter::TImpl::CheckTerminalsStateTerminalsFilled()
    {
        TWordpos end = LastWordInSent(CurrentSent);
        CurrentState.TerminalsFilled = true;
        if (MoveUntil(GetNextTerminal, Last, end) && HasMatches(First, Last)) {
            PushWords();
            return true;
        }
        return false;
    }

    bool TUniSpanIter::TImpl::CheckTerminalsCheckEnd()
    {
        TWordpos end = LastWordInSent(CurrentSent);
        if (Last != end) {
            // try to move boundary
            if (!MoveUntil(GetNext, Last, end)) {
                CurrentState.FilledFromCurWord = true;
                return false;
            }
            // reached maximum length
            if (Last.IsLast() || !CandidateFitsInLength(First, Last.Next())) {
                CurrentState.FilledFromCurWord = true;
            }
            // check if got candidate with matches
            if (HasMatches(First, Last)) {
                PushWords();
                return true;
            }
            return false;
        } else {
            // check if candidate filled
            if (CurrentState.FilledFromCurWord) {
                SetToCheckSpans(); // go to next stage
            } else {
                // try to fill candidate length by moving starting boundary
                CurrentState.FilledFromCurWord = true;
                TWordpos begin = FirstWordInSent(CurrentSent);
                if (MoveUntil(GetPrev, First, begin) && HasMatches(First, Last)) {
                    PushWords();
                    return true;
                }
            }
            return false;
        }
    }

    bool TUniSpanIter::TImpl::CheckTerminals()
    {
        // previously was filled candidate on current stage
        if (CurrentState.FilledFromCurWord) {
            return CheckTerminalsStateFilled();
        }

        // starting boundary moved forward too far - check words in missed range
        if (Last < First) {
            return CheckTerminalsStateReverted();
        }

        // check terminals
        if (!CurrentState.TerminalsFilled) {
            return CheckTerminalsStateTerminalsFilled();
        }

        // check if reached end of sentence
        return CheckTerminalsCheckEnd();
    }

    bool TUniSpanIter::TImpl::AddFirstSentBeg() {
        Y_ASSERT(Last == First);
        CurrentState.CheckSpansFirstWordSeen = true; // yeap, this function runs only once
        TWordpos end = LastWordInSent(CurrentSent);
        if (Last == end) { // in case of superlong the first word
            CurrentState.ProcessingSent = false;
            return false;
        }

        if (First == FirstWordInSent(CurrentSent) && MatchInfo.IsMatch(First)) {
            if (!CurrentCandidateFitsInLength()) {
                return false;
            }
            MoveUntil(GetNext, Last, Last.LastInSameSent());
            if (CheckWords(First, Last)) {
                return false;
            } else {
                PushWords();
                return true;
            }
        }
        return false;
    }

    bool TUniSpanIter::TImpl::CheckSpansStateFilled()
    {
        TWordpos end = LastWordInSent(CurrentSent);
        // reached end
        if (Last == end) {
            CurrentState.ProcessingSent = false;
            return false;
        }

        TWordpos lastMatch = end;
        if (!MatchInfo.IsMatch(lastMatch)) {
            lastMatch = GetPrevMatch(lastMatch);
        }

        // moves start boundary to next match and ending one to max distant match
        First = GetNextMatch(First);
        Last = First;
        MoveUntil(GetNextMatch, Last, lastMatch);
        // check if there were candidate with same matches
        if (CheckWords(First, Last)) {
            // no new candidates but make some checks
            if (Last == lastMatch) {
                CurrentState.ProcessingSent = false; // reached end of sent
            }
            return false;
        } else {
            // got new candidate!
            PushWords();
            CurrentState.TmpFirst = First;
            TWordpos begin = FirstWordInSent(CurrentSent);
            if (CurrentCandidateFitsInLength() && !MoveUntil(GetPrev, First, begin)) {
                return true;
            } else {
                // forces move span boundaries word by word
                CurrentState.FilledFromCurWord = false;
                return false;
            }
        }
    }

    bool TUniSpanIter::TImpl::CheckSpansStateUnfilled()
    {
        if (First == CurrentState.TmpFirst) {
            // first match == start of span, switch starting boundary to next match
            CurrentState.FilledFromCurWord = true;
            return false;
        } else {
            // moving span to next word
            ++First;
            TWordpos end = LastWordInSent(CurrentSent);
            MoveUntil(GetNext, Last, end);
            return true;
        }
    }

    inline bool TUniSpanIter::TImpl::CheckSpans()
    {
        if (CurrentState.FilledFromCurWord) {
            return CheckSpansStateFilled();
        } else {
            return CheckSpansStateUnfilled();
        }
    }

    inline void TUniSpanIter::TImpl::InitSeenMatches()
    {
        const int sentLen = MatchInfo.SentsInfo.GetSentLengthInWords(CurrentSent);
        SeenMatches.clear();
        SeenMatches.resize(sentLen, -1);
    }

    inline void TUniSpanIter::TImpl::SwitchSent()
    {
        CurrentState.Reset();
        int nextSent = BeforeFirst ? StartSent : CurrentSent + 1;
        BeforeFirst = false;

        if (nextSent > EndSent) {
            CurrentState.ReachedEnd = true;
            return;
        }

        CurrentSent = nextSent;

        CurSentHasMatches = MatchInfo.SentHasMatches(CurrentSent);
        if (CurSentHasMatches) {
            InitSeenMatches();
        }
        First = FirstWordInSent(CurrentSent);
        Last = LastWordInSent(CurrentSent);
    }

    bool TUniSpanIter::TImpl::CandidateFitsInLength(TWordpos first, TWordpos last) const
    {
        return SpanLen.CalcLength(TSingleSnip(first, last, MatchInfo)) <= MaxLength;
    }

    bool TUniSpanIter::TImpl::CurrentCandidateFitsInLength() const
    {
        return CandidateFitsInLength(First, Last);
    }

    bool TUniSpanIter::TImpl::MoveNext()
    {
        if (BeforeFirst) {
            SwitchSent();
        }
        while (true) {
            if (IsReachedEnd()) {
                return false;
            }

            if (!CurrentState.SentsFilled) {
                if (AddWholeSent()) {
                    return true;
                }
            } else {
                if (CurrentState.SentsSeen > 0) {
                    if (!CurrentState.FilledFromCurWord && AddSentBegin()) {
                        return true;
                    }
                } else { // We're working now on the one big sentence (CurrentState)
                    if (CurSentHasMatches) {
                        if (CurrentState.CheckTerminalsStage) {
                            if (CheckTerminals()) {
                                return true;
                            }
                        } else {
                            if (CurrentState.CheckSpansFirstWordSeen == false) {
                                if (AddFirstSentBeg()) {
                                    return true;
                                }
                            }
                            if (CheckSpans()) {
                                return true;
                            }
                        }
                    }
                }
            }

            if (!CurrentState.ProcessingSent) {
                if (IsReachedEnd()) {
                    return false;
                } else {
                    SwitchSent();
                }
            }
        }
        return false;
    }

    TUniSpanIter::TImpl::TImpl(const TSentsMatchInfo& matchInfo, const IRestr& restr, const IRestr& skipRestr, const TWordSpanLen& wordSpanLen, int start, int stop)
        : MatchInfo(matchInfo)
        , SpanLen(wordSpanLen)
        , BeforeFirst(true)
        , MaxLength()
        , First(matchInfo.SentsInfo.Begin<TWordpos>())
        , Last(matchInfo.SentsInfo.Begin<TWordpos>())
        , StartSent(start < 0 ? 0 : start)
        , CurrentSent(StartSent)
        , CurSentHasMatches(false)
        , CurrentState(First)
        , SeenMatches()
        , EndSent(stop < 0 ? matchInfo.SentsInfo.SentencesCount() - 1 : stop)
        , GetNextTerminal(MatchInfo)
        , GetNextMatch(MatchInfo)
        , GetPrevMatch(MatchInfo)
        , GetNext()
        , GetPrev()
        , Restriction(restr)
        , SkipRestr(skipRestr)
    {
    }

    TWordpos TUniSpanIter::TImpl::GetFirst() const
    {
        Y_ASSERT(CurrentCandidateFitsInLength());
        return First;
    }

    TWordpos TUniSpanIter::TImpl::GetLast() const
    {
        Y_ASSERT(CurrentCandidateFitsInLength());
        return Last;
    }

    const IRestr& TUniSpanIter::TImpl::GetSkipRestr() const
    {
        return SkipRestr;
    }

    const TWordSpanLen& TUniSpanIter::TImpl::GetWordSpanLen() const
    {
        return SpanLen;
    }

    void TUniSpanIter::TImpl::Reset(float maxSize)
    {
        MaxLength = maxSize;
        CurrentSent = StartSent;
        BeforeFirst = true;
    }

    TUniSpanIter::TUniSpanIter(const TSentsMatchInfo& matchInfo, const IRestr& restr, const IRestr& skipRestr, const TWordSpanLen& wordSpanLen, int start, int stop)
        : Impl(new TImpl(matchInfo, restr, skipRestr, wordSpanLen, start, stop))
    {
    }

    TUniSpanIter::~TUniSpanIter() {
    }

    TWordpos TUniSpanIter::GetFirst() const {
        return Impl->GetFirst();
    }

    TWordpos TUniSpanIter::GetLast() const {
        return Impl->GetLast();
    }

    const IRestr& TUniSpanIter::GetSkipRestr() const {
        return Impl->GetSkipRestr();
    }

    const TWordSpanLen& TUniSpanIter::GetWordSpanLen() const {
        return Impl->GetWordSpanLen();
    }

    bool TUniSpanIter::MoveNext() {
        return Impl->MoveNext();
    }

    void TUniSpanIter::Reset(float maxSize) {
        Impl->Reset(maxSize);
    }
}
