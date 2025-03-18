#pragma once

#include "phone.h"
#include "phone_proc.h"
#include "tel_schemes.h"
#include "telephonoid.h"
#include "token.h"

#include <library/cpp/containers/comptrie/search_iterator.h>
#include <util/charset/wide.h>
#include <util/generic/yexception.h>
#include <util/generic/map.h>
#include <util/string/util.h>

////
//  TTelFinder
////

class TTelFinder {
protected:
    static const size_t MAX_SEQUENTIAL_TELEPHONEIDS = 100;

    TPhoneSchemes TelSchemes;
    struct TSearchState {
        TTelephonoid Id;
        TSearchIterator<TPhoneSchemes::TSchemeTrie> Iter;
        ui16 CommulativeSpacesTypes;
    };
    typedef std::pair<TTelephonoid, TAreaScheme> TResultState;

    static const ui16 WEAK_SEP = TTelephonoidChunk::DBadChar | TTelephonoidChunk::DOpeningParen | TTelephonoidChunk::DPlus;
    static const ui16 NON_SP = ~ui16(TTelephonoidChunk::DSpace | TTelephonoidChunk::DBadChar | TTelephonoidChunk::DBad);

protected:
    bool IsNumber(TWtringBuf token) const;

    // Check states against next telephonoid chunk. Updates positions in the scheme trie for matched states
    // and remove unmatched states
    template <bool ignoreSchemes>
    static void ShiftStates(const TTelephonoidChunk& tc, TVector<TSearchState>& states, TString& buff);

    // Stores intermediate results for search states, which have full matched schemes.
    // In case of cleanup == true, resolves ambiguity for all intermediate results and calls phoneProcessor for them,
    // and clears all search states after that.
    template <bool ignoreSchemes>
    static bool CheckStates(TVector<TSearchState>& states, TVector<TResultState>& res, ui16 spaces);

    static void FlushResults(TVector<TResultState>& res, IPhoneProcessor& phoneProcessor);

    template <class TIterator, bool ignoreSchemes>
    void FindPhonesImpl(TIterator it, IPhoneProcessor& phoneProcessor) const {
        TVector<TSearchState> curStates;
        TVector<TResultState> results;
        TString buff;
        try {
            ui16 currentSpacesTypes = 0;
            size_t pos = 0;
            bool phoneBoundary = true;
            for (; it.Ok(); ++it, ++pos) {
                TToken token = *it;
                if (IsNumber(token.Word)) {
                    if (phoneBoundary) {
                        TSearchState newState = {TTelephonoid(), MakeSearchIterator(TelSchemes.GetSchemes()), currentSpacesTypes};
                        curStates.push_back(newState);
                        phoneBoundary = false;
                    }
                    TTelephonoidChunk tc = {token.Word, pos, currentSpacesTypes};
                    ShiftStates<ignoreSchemes>(tc, curStates, buff);
                    //spaces
                    currentSpacesTypes = TTelephonoidChunk::GetLeftPhoneSpacesTypesUntilError(token.Punctuation);
                    phoneBoundary = CheckStates<ignoreSchemes>(curStates, results, currentSpacesTypes);
                    if (currentSpacesTypes & TTelephonoidChunk::DBadChar) {
                        phoneBoundary = true;
                        currentSpacesTypes = TTelephonoidChunk::GetRightPhoneSpacesTypes(token.Punctuation);
                        if (!results.empty()) {
                            FlushResults(results, phoneProcessor);
                        }
                        curStates.clear();
                    }

                } else {
                    currentSpacesTypes &= ~TTelephonoidChunk::DBad;
                    if (token.Word) {
                        ui16 nextSpacesTypes = TTelephonoidChunk::GetLeftPhoneSpacesTypesUntilError(token.Word);
                        static const ui16 badIfRepeated = TTelephonoidChunk::DPlus | TTelephonoidChunk::DDash | TTelephonoidChunk::DOpeningParen | TTelephonoidChunk::DClosingParen;
                        if (currentSpacesTypes & nextSpacesTypes & badIfRepeated)
                            currentSpacesTypes |= TTelephonoidChunk::DBad;
                        currentSpacesTypes |= nextSpacesTypes;
                        currentSpacesTypes |= TTelephonoidChunk::GetLeftPhoneSpacesTypesUntilError(token.Punctuation);
                    } else {
                        // empty word means sentence break
                        currentSpacesTypes |= TTelephonoidChunk::DBadChar;
                    }
                    phoneBoundary = phoneBoundary || CheckStates<ignoreSchemes>(curStates, results, currentSpacesTypes);
                    if (currentSpacesTypes & TTelephonoidChunk::DBadChar) {
                        phoneBoundary = true;
                        currentSpacesTypes = TTelephonoidChunk::GetRightPhoneSpacesTypes(token.Punctuation);
                        if (!results.empty()) {
                            FlushResults(results, phoneProcessor);
                        }
                        curStates.clear();
                    }
                    // Don't allow extract phone inside a multi-token mark (<word><num>)
                    if (!token.Word.Empty() && (IsAlpha(token.Word.back()) || token.Word.back() == '$' || token.Word.back() == '#') && token.Punctuation.Empty()) {
                        currentSpacesTypes |= TTelephonoidChunk::DBad;
                    }
                }
            }
            if (!curStates.empty()) {
                CheckStates<ignoreSchemes>(curStates, results, TTelephonoidChunk::DBadChar);
                curStates.clear();
            }
            if (!results.empty()) {
                FlushResults(results, phoneProcessor);
            }
        } catch (...) {
            ythrow yexception() << "Unknown exception in \"TTelFinder::FindPhones\".";
        }
    }

public:
    TTelFinder() {
    }

    TTelFinder(const TPhoneSchemes& telSchemes)
        : TelSchemes(telSchemes)
    {
    }

    static TPhone CreatePhone(const TTelephonoid& telephonoid, const TAreaScheme& scheme);
    static TPhoneLocation CreateLocation(const TTelephonoid& telephonoid, const TAreaScheme& scheme);
    static TFoundPhone CreateFoundPhone(const TTelephonoid& telephonoid, const TAreaScheme& scheme);

    inline const TPhoneSchemes& GetSchemes() const {
        return TelSchemes;
    }

    template <class TIterator>
    inline void FindPhones(TIterator it, IPhoneProcessor& phoneProcessor) const {
        FindPhonesImpl<TIterator, false>(it, phoneProcessor);
    }

    // Technical method for mining new phone schemes. Don't use in real phone extraction
    template <class TIterator>
    inline void FindPhonesIgnoreSchemes(TIterator it, IPhoneProcessor& phoneProcessor) const {
        FindPhonesImpl<TIterator, true>(it, phoneProcessor);
    }
};
