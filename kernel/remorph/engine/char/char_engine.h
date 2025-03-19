#pragma once

#include "char_iterator.h"
#include "char_result.h"
#include "literal_table.h"

#include <kernel/remorph/common/verbose.h>
#include <kernel/remorph/core/core.h>
#include <kernel/remorph/engine/engine.h>
#include <kernel/remorph/proc_base/matcher_base.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/system/defaults.h>

namespace NReMorph {

class TCharEngine;
typedef TIntrusivePtr<TCharEngine> TCharEnginePtr;

class TCharEngine: public NMatcher::TMatcherBase {
public:
    typedef TCharResult TResult;
    typedef TCharResultPtr TResultPtr;
    typedef TCharResults TResults;

private:
    NPrivate::TLiteralTablePtr LiteralTable;
    TVector<std::pair<TString, double>> RuleNames;
    NRemorph::TDFAPtr Dfa;
    size_t MaxSearchStates; // Maximum number of search states when traversing DFA

private:
    template <class TResultHolder, bool search>
    class TResultCollector: public NRemorph::TMatchTrack {
    private:
        const TCharEngine& Engine;
        TResultHolder& ResultHolder;
        NSorted::TSimpleMap<size_t, size_t> SymbBreaks;

    private:
        template <class TSymbolPtr>
        inline TUtf16String GetText(const NRemorph::TInput<TSymbolPtr>& inputSequence) {
            const NRemorph::TInput<TSymbolPtr>* current = inputSequence.GetNext() ? &inputSequence.GetNext().front() : nullptr;
            TUtf16String text;
            size_t i = 0;
            for (; nullptr != current; ++i) {
                SymbBreaks.push_back(std::make_pair(text.length(), i));
                if (i > 0 && current->GetSymbol()->GetProperties().Test(NSymbol::PROP_SPACE_BEFORE)) {
                    text.append(wchar16(' '));
                    SymbBreaks.push_back(std::make_pair(text.length(), i));
                }
                text.append(current->GetSymbol()->GetText());
                current = current->GetNext() ? &current->GetNext().front() : nullptr;
            }
            SymbBreaks.push_back(std::make_pair(text.length(), i));
            NRemorph::TMatchTrack::InputTrack.assign(i, 0);
            NRemorph::TMatchTrack::Start = 0;
            NRemorph::TMatchTrack::End = i;
            return text;
        }

        template <class TSymbolPtr>
        inline TUtf16String GetText(const TVector<TSymbolPtr>& inputSequence) {
            TUtf16String text;
            size_t i = 0;
            for (; i < inputSequence.size(); ++i) {
                SymbBreaks.push_back(std::make_pair(text.length(), i));
                if (i > 0 && inputSequence[i]->GetProperties().Test(NSymbol::PROP_SPACE_BEFORE)) {
                    text.append(wchar16(' '));
                    SymbBreaks.push_back(std::make_pair(text.length(), i));
                }
                text.append(inputSequence[i]->GetText());
            }
            SymbBreaks.push_back(std::make_pair(text.length(), i));
            NRemorph::TMatchTrack::Start = 0;
            NRemorph::TMatchTrack::End = inputSequence.size();
            return text;
        }

        bool CharPosToSymbolPos(size_t& pos) const {
            NSorted::TSimpleMap<size_t, size_t>::const_iterator i = SymbBreaks.Find(pos);
            if (i == SymbBreaks.end())
                return false;
            pos = i->second;
            return true;
        }

        bool CharPosToSymbolPos(std::pair<size_t, size_t>& pos) const {
            return CharPosToSymbolPos(pos.first) && CharPosToSymbolPos(pos.second);
        }

        void ConvertNamedMatchPos(NRemorph::TNamedSubmatches& namedSubmatches, const NRemorph::TMatchInfo& info, size_t offset) const {
            for (NRemorph::TNamedSubmatches::const_iterator i = info.NamedSubmatches.begin(); i != info.NamedSubmatches.end(); ++i) {
                NRemorph::TNamedSubmatches::value_type v = *i;
                v.second += offset;
                if (CharPosToSymbolPos(v.second)) {
                    v.second -= NRemorph::TMatchTrack::Start;
                    namedSubmatches.insert(v);
                }
            }
        }

        inline bool IsAcceptable(const TUtf16String& text, size_t offset = 0) const {
            Y_ASSERT(offset <= text.length());
            return offset < text.length()
                && (0 == offset || Y_LIKELY(0 == (Engine.Dfa->Flags & NRemorph::DFAFLG_STARTS_WITH_ANCHOR)))
                && (Engine.Dfa->MinimalPathLength < 2 || text.length() - offset >= Engine.Dfa->MinimalPathLength);
        }

    public:
        TResultCollector(const TCharEngine& e, TResultHolder& resHolder)
            : NRemorph::TMatchTrack(0, 0)
            , Engine(e)
            , ResultHolder(resHolder)
        {
            Y_ASSERT(Engine.Dfa);
        }

        // Controls the search stop point
        Y_FORCE_INLINE operator bool() const {
            return ResultHolder.AcceptMore();
        }

        template <class TInputSource>
        void Search(const TInputSource& inputSequence) {
            static_assert(search, "expect search");
            TUtf16String text = GetText(inputSequence);
            if (!IsAcceptable(text))
                return;

            bool full = true;
            for (NSorted::TSimpleMap<size_t, size_t>::const_iterator i = SymbBreaks.begin();
                i != SymbBreaks.end() && ResultHolder.AcceptMore(); ++i) {
                if (IsAcceptable(text, i->first)) {
                    full = NRemorph::SearchFrom(*Engine.LiteralTable, *Engine.Dfa,
                        NPrivate::TWcharIterator(TWtringBuf(text), i->first),
                        *this, Engine.MaxSearchStates) && full;
                }
            }
            Engine.WarnIfLostSearchResults(full);
        }

        template <class TInputSource>
        void Match(const TInputSource& inputSequence) {
            static_assert(!search, "expect !search");
            TUtf16String text = GetText(inputSequence);
            if (!IsAcceptable(text))
                return;

            const bool full = NRemorph::SearchFrom(*Engine.LiteralTable, *Engine.Dfa,
                NPrivate::TWcharIterator(TWtringBuf(text)),
                *this, Engine.MaxSearchStates);
            Engine.WarnIfLostSearchResults(full);
        }

        // Functor for checking match result
        void operator()(const NRemorph::TMatchState<NPrivate::TWcharIterator>& state) {
            if (!search && !state.GetIter().AtEnd())
                return;

            std::pair<size_t, size_t> pos = state.GetIter().GetPosRange();
            size_t startOffset = pos.first;
            if (!CharPosToSymbolPos(pos) || (pos.first == pos.second))
                return;

            NRemorph::TMatchTrack::Start = pos.first;
            NRemorph::TMatchTrack::End = pos.second;

            TVector<NRemorph::TMatchInfoPtr> matchInfos = state.CreateMatchInfos();
            for (TVector<NRemorph::TMatchInfoPtr>::const_iterator i = matchInfos.begin();
                i != matchInfos.end() && ResultHolder.AcceptMore(); ++i) {

                TCharResultPtr res = new TCharResult(Engine.RuleNames[i->Get()->MatchedId], pos, *this);
                ConvertNamedMatchPos(res->NamedSubmatches, **i, startOffset);
                res->MatchTrack.GetContexts().resize(pos.second - pos.first);
                ResultHolder.Put(res);
            }
        }
    };

private:
    inline void WarnIfLostSearchResults(const bool full) const {
        if (Y_UNLIKELY(!full)) {
            REPORT(WARN, "The limit " << int(MaxSearchStates)
                << " for number of search states has been exceeded. Some of search results could be lost");
        }
    }

protected:
    TCharEngine()
        : NMatcher::TMatcherBase(NMatcher::MT_CHAR)
        , MaxSearchStates(1000)
    {
    }

    void LoadFromFile(const TString& filePath, const NGzt::TGazetteer*) override;
    void LoadFromStream(IInputStream& in, bool signature) override;

    void ParseFromString(const TString& rules);

public:
    void SetMaxSearchStates(size_t states) {
        MaxSearchStates = states;
    }

    size_t GetMaxSearchStates() const {
        return MaxSearchStates;
    }

    void CollectUsedGztItems(THashSet<TUtf16String>&) const override;
    void SaveToStream(IOutputStream& out) const override;

    static TCharEnginePtr Load(const TString& filePath);
    // Load compiled char-engine from the stream
    static TCharEnginePtr Load(IInputStream& in);
    // Parse char-engine from the specified string
    static TCharEnginePtr Parse(const TString& rules);

public:
    template <class TInputSource>
    inline TCharResultPtr Match(const TInputSource& inputSource, NRemorph::OperationCounter* = nullptr) const {
        NRemorph::TSingleResultHolder<TCharResultPtr> holder;
        TResultCollector<NRemorph::TSingleResultHolder<TCharResultPtr>, false> collector(*this, holder);
        collector.Match(inputSource);
        return holder.Result;
    }

    template <class TInputSource>
    inline void MatchAll(const TInputSource& inputSource, TCharResults& results, NRemorph::OperationCounter* = nullptr) const {
        NRemorph::TMultiResultHolder<TCharResults> holder(results);
        TResultCollector<NRemorph::TMultiResultHolder<TCharResults>, false> collector(*this, holder);
        collector.Match(inputSource);
    }

    template <class TInputSource>
    inline TCharResultPtr Search(const TInputSource& inputSource, NRemorph::OperationCounter* = nullptr) const {
        NRemorph::TSingleResultHolder<TCharResultPtr> holder;
        TResultCollector<NRemorph::TSingleResultHolder<TCharResultPtr>, true> collector(*this, holder);
        collector.Search(inputSource);
        return holder.Result;
    }

    template <class TInputSource>
    inline void SearchAll(const TInputSource& inputSource, TCharResults& results, NRemorph::OperationCounter* = nullptr) const {
        NRemorph::TMultiResultHolder<TCharResults> holder(results);
        TResultCollector<NRemorph::TMultiResultHolder<TCharResults>, true> collector(*this, holder);
        collector.Search(inputSource);
    }

    template <class TSymbolPtr>
    inline void SearchAllCascaded(const NRemorph::TInput<TSymbolPtr>& input, TCharResults& results, NRemorph::OperationCounter* = nullptr) const {
        SearchAll(input, results);
    }

private:
    void Parse(const TStreamParseOptions& options);
};

} // NReMorph
