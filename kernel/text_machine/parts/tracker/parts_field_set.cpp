#include "parts_field_set.h"

#include <kernel/text_machine/module/module_def.inc>


namespace NTextMachine::NCore {

    MACHINE_PARTS(Tracker) {

        using TStateRef =
            TFieldSetForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19Unit::TStreamwiseState;
        using TResultRef =
             IDataExtractorImpl<EDataExtractorType::DssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19>::TStreamCounters;

        static void FillDataImpl(
            const TStateRef& in,
            TResultRef& out)
        {
            const size_t queryWordsNum = in.WordCounters.Size();
            out.WordCounters.resize(queryWordsNum);

            for(size_t queryWordId : xrange(queryWordsNum)) {
                auto& dst = out.WordCounters[queryWordId];
                auto& src = in.WordCounters[queryWordId];
                dst.TermFrequencyAny = src.TermFrequence;
                dst.TermFrequencyExact = src.ExactTermFrequence;
                dst.MaxQueryWordsInOneSentenceFoundWithCurrent = src.MaxQueryCooucrance;

                TArrayRef<const float> coOccuranceInfo = in.InvDistances.GetRange(queryWordId);
                for(float f : coOccuranceInfo) {
                    dst.QueryWordsFoundWithCurrent += size_t(f > 0);
                    dst.AvgMaxInverseDistance += f;
                }
                dst.AvgMaxInverseDistance /= queryWordsNum;
            }

            if (queryWordsNum > 1) {
                out.BigramCounters.resize(queryWordsNum - 1);

                for(size_t queryBigramId : xrange(queryWordsNum - 1)) {
                    auto& dst = out.BigramCounters[queryBigramId];
                    auto& src = in.BigramCounters[queryBigramId];

                    dst.SentecesWithBothWords = src.SentecesWithBothWords;
                    dst.SentencesWithBothWordsAnyFormNeighboring = src.SentencesWithBothWordsAnyFormNeighboring;
                    dst.SentencesWithBothWordsExactNeighboring = src.SentencesWithBothWordsExactNeighboring;
                    dst.MaxQueryWordsInOneSentenceFoundWithCurrentWords = src.MaxQueryWordsInOneSentenceFoundWithCurrentWords;
                    dst.MaxQueryWordsInOneSentenceFoundWithCurrentWordsNeighboring = src.MaxQueryWordsInOneSentenceFoundWithCurrentWordsNeighboring;

                    dst.InverseDistanceSum = src.InverseDistanceSum;
                }
            }
        }

        TForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19
            TFieldSetForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19Unit::TState::DoExtractData() const
        {
            Y_ENSURE(UrlState);
            Y_ENSURE(TitleState);
            Y_ENSURE(BodyState);
            Y_ENSURE(Query);

            TForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19 result;

            result.Query = Query;
            FillDataImpl(*UrlState, result.CountersByUrl);
            FillDataImpl(*TitleState, result.CountersByTitle);
            FillDataImpl(*BodyState, result.CountersByBody);

            Y_ASSERT(result.CountersByUrl.WordCounters.size() == result.CountersByTitle.WordCounters.size());
            Y_ASSERT(result.CountersByUrl.WordCounters.size() == result.CountersByBody.WordCounters.size());

            Y_ASSERT(result.CountersByUrl.BigramCounters.size() == result.CountersByTitle.BigramCounters.size());
            Y_ASSERT(result.CountersByUrl.BigramCounters.size() == result.CountersByBody.BigramCounters.size());

            return result;
        }


        TCountersForDssmSSHardWordWeights TFieldSetForDssmSSHardWordWeightsFeaturesUnit::TState::DoExtractData() const {
            Y_ASSERT(Query);
            Y_ASSERT(TitleState);
            Y_ASSERT(QueryDwellTimeState);
            Y_ASSERT(TitleState->WeightedAcc.Size() == Query->Words.size());
            Y_ASSERT(QueryDwellTimeState->WeightedAcc.Size() == Query->Words.size());

            TCountersForDssmSSHardWordWeights result;

            result.Query = Query;
            result.WordCounters.resize(Query->Words.size());

            for(auto queryWordId : xrange(Query->Words.size())) {
                auto& dst = result.WordCounters[queryWordId];
                dst.FoundInTitle = TitleState->WeightedAcc.Get(queryWordId) > 0;
                dst.FoundInQueryDwellTimeStream = QueryDwellTimeState->WeightedAcc.Get(queryWordId) > 0;
            }

            return result;
        }

        TFullSplitBertCounters TFieldSetForFullSplitBertCountersUnit::TState::DoExtractData() const {
            Y_ASSERT(Query->GetNumWords() == FieldsAcc.Size());

            TFullSplitBertCounters result;

            size_t numWords = Query->GetNumWords();
            result.resize(numWords);
            for (size_t i : xrange(numWords)) {
                auto& counters = result[i];
                counters.ExactHitsInBody = FieldsAcc[i];
                counters.Idfs = Query->GetIdf(i);
            }
            return result;
        }

    } // MACHINE_PARTS(Tracker)
} //  NTextMachine::NCore

#include <kernel/text_machine/module/module_undef.inc>

