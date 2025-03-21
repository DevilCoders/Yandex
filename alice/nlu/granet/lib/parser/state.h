#pragma once

#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/grammar/grammar_data.h>
#include <alice/nlu/granet/lib/grammar/token_id.h>
#include <alice/nlu/libs/interval/interval.h>
#include <util/generic/deque.h>

namespace NGranet {

// Let's we have grammar with rules:
//   A: B C w5 w6 D
//   B: w0 w1
//   C: w2 w3 w4
//   D: w7 w8
//
// Legend:
//   A, B, C, D - nonterminals (elements);
//   w0, w1, ... - terminals (words)
//
// Then this grammar can be matched to sample in such a way:
//
//   Sample:                    |  w0 |  w1 |  w2 |  w3 |  w4 |  w5 |  w6 |  w7 |  w8 |
//   A:                         |<-A-----A-----A-----A-----A-----A-----A-----A-----A->|
//   Children:                  |<-B-----B->|<-C-----C-----C->|  w5 |  w6 |<-D-----D->|
//   Chart:                    [0]   [1]   [2]   [3]   [4]   [5]   [6]   [7]   [8]   [9]
//   States of A:              SA0<--------SA1<--------------SA2<--SA3<--SA4<--------SA5
//                                          v                 |                       |
//   States of B:              SB0<--SB1<--SB2                |                       |
//                                                            v                       v
//   States of C and D:                    SC0<--SC1<--SC2<--SC3         SD0<--SD1<--SD2
//
// Horizontal arrows between states - TParserState::Prev.
// Vertical arrows between states - TParserState::PassedChild.
//
// Actually diagram is simplified. Some states are pairs of states (connected by Prev):
//   - states SB2, SC3, SD2, SA5 are pairs of states: incomplete and complete state,
//   - states SA0, SA1, SA4 are pairs of states: state before normal state and state of waiting for child.

// ~~~~ Constants ~~~~

// TODO(samoylovboris) Move these constants to compiler (in rule logprob).

// ln(-10) is 22 000 possible words hidden behind wildcard
constexpr float WILDCARD_LOG_PROB = -10;

// Penalty for filler generation.
constexpr float FILLER_LOG_PROB = -2;

// Penalty for filler generation inside %cover_filler (lower than FILLER_LOG_PROB)
constexpr float COVERED_FILLER_LOG_PROB = -1;

// Penalty for match by lemma.
constexpr float LEMMA_LOG_PROB = -1;

// ~~~~ EParserStateKeyFlag ~~~~

enum EParserStateKeyFlag : ui8 {
    // Is interval [position of last restart or begin, current position) not empty.
    // Used to prevent restarting of element if last iteration has matched empty interval
    // (protection from infinite loop).
    PSKF_HAS_WORDS_IN_CURRENT_ITERATION = FLAG8(0),

    // State was generated by complete filler.
    PSKF_PASSED_FILLER = FLAG8(1),

    // Waiting for predicted child.
    PSKF_WAITING_FOR_CHILD = FLAG8(2),

    // Final state of one element occurrence.
    PSKF_COMPLETE = FLAG8(3),
};

Y_DECLARE_FLAGS(EParserStateKeyFlags, EParserStateKeyFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(EParserStateKeyFlags);

// ~~~~ TParserStateKey ~~~~

// Each parser state describes one moment of one parsing branch. While moving forward algorithm
// generates new states and doesn't change or delete existed ones.
class TParserStateKey {
public:
    // Position in sample.
    // End point is always equal to index of TParserStateList in array Chart.
    NNlu::TIntInterval Interval;

    // Iterated grammar element.
    // Always defined.
    const TGrammarElement* Element = nullptr;

    // Position in TGrammarElement::Rules tree.
    // Always defined.
    TSearchIterator<TRuleTrie> TrieIterator;

    // TODO(samoylovboris) comments
    ui32 SetOfCompleteRulesOfBag = 0;
    ui8 ConstrainedIterationCount = 1;

    EParserStateKeyFlags Flags = 0;

    ui16 TrieIteratorDepth = 0;

public:
    bool operator==(const TParserStateKey& other) const;
    bool operator!=(const TParserStateKey& other) const;

    size_t GetHash() const;

    bool HasFlags(const EParserStateKeyFlags& flags) const {
        return Flags.HasFlags(flags);
    }
    bool HasAnyFlag(const EParserStateKeyFlags& flags) const {
        return Flags & flags;
    }

    bool HasWordsInCurrentIteration() const {
        return Flags.HasFlags(PSKF_HAS_WORDS_IN_CURRENT_ITERATION);
    }
    bool IsPassedFiller() const {
        return Flags.HasFlags(PSKF_PASSED_FILLER);
    }
    bool IsWaitingForChild() const {
        return Flags.HasFlags(PSKF_WAITING_FOR_CHILD);
    }
    bool IsComplete() const {
        return Flags.HasFlags(PSKF_COMPLETE);
    }
};

// ~~~~ TParserStateKey for global namespace ~~~~

} // namespace NGranet

template <>
struct THash<NGranet::TParserStateKey> {
    inline size_t operator()(const NGranet::TParserStateKey& state) const {
        return state.GetHash();
    }
};

namespace NGranet {

// ~~~~ EParserStateEventType ~~~~

enum EParserStateEventType : ui32 {
    PSET_UNDEFINED,
    PSET_BEGIN,
    PSET_SKIP_CHILD,
    PSET_WAIT_CHILD,
    PSET_PASS_TOKEN,
    PSET_PASS_CHILD,
    PSET_PASS_FILLER,
    PSET_RESTART,
    PSET_COMPLETE_ENTITY,
    PSET_COMPLETE,
};

// ~~~~ TParserState ~~~~

class TParserState : public TParserStateKey {
public:
    TParserState(const TParserStateKey& key, EParserStateEventType event, int index)
        : TParserStateKey(key)
        , EventType(event)
        , Index(index)
    {
    }

    // For debug output.
    EParserStateEventType EventType = PSET_UNDEFINED;

    // Index in TParserStateList.
    int Index = 0;

    // Log probability of state. It includes:
    // - Sum of log probability of each item in rule:
    //     - 0 for exact words,
    //     - LEMMA_LOG_PROB for lemmas,
    //     - likely not big negative value for nonterminals (children elements),
    //     - WILDCARD_LOG_PROB - big negative value for wildcards.
    // - TGrammarElement::RulesLogProbs - log probability of completed rule relative to whole trie.
    float LogProb = 0;

    // Upper bound of log probability of final solution if this solution uses this state.
    // = LogProb + Max{parent.SolutionLogProbUpperBound for parent from WaitingParentsList}
    // Heuristic for StateLimit processing.
    float SolutionLogProbUpperBound = 0;

    // Complete rules of this state.
    // Note: state of bag element may be not complete, but have complete rules.
    TRuleIndexes CompleteRules;

    // See comment in the beginning of file.
    const TParserState* Prev = nullptr;
    const TParserState* PassedChild = nullptr;
    const TParserState* PredictedChild = nullptr;
    const TParserState* BeginningState = nullptr;
    const TParserState* WaitingParentsList = nullptr;

    // States with equal keys arranged into the list of alternatives sorted by LogProb:
    //   TParserStateList::BestStates[key] - starting point of that list (best state).
    //   TParserState::WorseAlternative, BetterAlternative - connections.
    TParserState* WorseAlternative = nullptr;
    TParserState* BetterAlternative = nullptr;

    // Parser should ignore this state. Possible reasons:
    //   - State has better alternative (BetterAlternative != nullptr).
    //   - State was marked as disabled to avoid combinatorial explosion.
    bool IsDisabled = false;

public:
    // Best complete rule of this state is forced. Which means that it is better (win in case
    // of ambiguity) than any not forced rule.
    bool IsForced() const {
        return CompleteRules.RuleCount > 0 && Element->ForcedRules.Get(CompleteRules.RuleIndex);
    }

    // Used to compare two complete states of same element to resolve ambiguity.
    bool IsBetterThan(const TParserState& other) const {
        if (IsForced() != other.IsForced()) {
            return IsForced();
        }
        return LogProb > other.LogProb;
    }

    // Best complete rule of this state is negative.
    // If parser choose 'negative' rule on some interval (rule wins by probability), then do not
    // generate completed element on this interval.
    bool IsNegative() const {
        return CompleteRules.RuleCount > 0 && Element->NegativeRules.Get(CompleteRules.RuleIndex);
    }
};

// ~~~~ TParserStateList ~~~~

// States of parser for one position in sequence of tokens.
class TParserStateList : public TMoveOnly {
public:
    void Init(size_t levelCount, size_t stateLimit);

    size_t GetStateCount() const {
        return StateCount;
    }
    size_t GetStateLimit() const {
        return StateLimit;
    }
    bool GetStateLimitHasBeenReached() const {
        return StateLimitHasBeenReached;
    }

    // Add beginning state.
    const TParserState* AddBeginningState(const TParserStateKey& key, float solutionLogProbUpperBound);

    // Add other types of state.
    void AddNextState(const TParserStateKey& key, EParserStateEventType event,
        const TParserState& prev, float logProbIncrement = 0, const TParserState* predictedChild = nullptr,
        const TParserState* passedChild = nullptr, TRuleIndexes completeRules = {});

    // Element level -> states of elements of that level.
    const TVector<TDeque<TParserState>>& GetLevels() const {
        return StatesOfLevels;
    }

private:
    TParserState& AddStateCommon(const TParserStateKey& key, EParserStateEventType event);
    TParserState& GetWritableState(const TParserState& state);
    void TrackBest(TParserState& state);
    bool ProcessStateLimit();
    void DisableState(TParserState& state);
    bool Check() const;

private:
    // All states in this position.
    TVector<TDeque<TParserState>> StatesOfLevels;

    // Best state (by LogProb) for each TParserStateKey.
    THashMap<TParserStateKey, TParserState*> BestStates;

    size_t StateCount = 0;
    size_t EnabledStateCount = 0;
    size_t StateLimit = Max<size_t>();
    bool StateLimitHasBeenReached = false;
};

} // namespace NGranet
