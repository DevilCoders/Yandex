#pragma once

/////////////////////////////////////////////////////////////////////
// Clusterer for big groups but not big enough to exceed memory limits
template <template <typename> class TSortingAlgorithm, template <typename> class TSearchingAlgorithm>
class TOrdinarClusterer: private TNonCopyable {
public:
    TOrdinarClusterer(TClustererConfig& config, THandler* handler)
        : Config(config)
        , Handler(handler)
    {
    }

public:
    void OnHypothesis(const TVector<TDoc>& hyp) {
        TVector<const TDoc*> hypothesis;
        MakeVectorOfPointers(hyp, &hypothesis);
        OnHypothesis(hypothesis);
    }

    void OnHypothesis(TVector<const TDoc*>& hypothesis) {
        if (Y_UNLIKELY(hypothesis.size() == 0)) {
            ythrow TWithBackTrace<yexception>() << "Empty hypothesis";
        }

        TVector<bool> processed(hypothesis.size(), false);

        THolder<TSearchingAlgorithm<TDoc>> search(new TSearchingAlgorithm<TDoc>(Config, hypothesis, processed));

        TSortingAlgorithm<TDoc>::Sort(*search.Get(), &hypothesis);
        search->PostLoad();

        size_t mainIndex = 0;
        const size_t hypothesisSize = hypothesis.size();

        TVector<size_t> groupIndexes;
        TVector<size_t> processedIndexes;

        while (true) {
            if (Y_UNLIKELY(search->Count() == 0)) {
                break;
            }

            while ((mainIndex < hypothesisSize) &&
                   (processed[mainIndex])) {
                ++mainIndex;
            }
            if (Y_UNLIKELY(mainIndex >= hypothesisSize)) {
                break;
            } else {
                if (Y_UNLIKELY(processed[mainIndex])) {
                    ythrow TWithBackTrace<yexception>() << "Found allready processed main";
                }
            }

            groupIndexes.clear();
            processedIndexes.clear();

            if ((TSortingAlgorithm<TDoc>::CorrectNeighborCount == 0) || (hypothesis[mainIndex]->Temprank > 0)) {
                search->FindNeighbors(mainIndex, &groupIndexes);

                if (TSortingAlgorithm<TDoc>::CorrectNeighborCount == 1) {
                    if (Y_UNLIKELY(groupIndexes.size() > hypothesis[mainIndex]->Temprank)) {
                        ythrow TWithBackTrace<yexception>() << "Group size is bigger than neighbor count";
                    }
                }
            }

            if (groupIndexes.size() > 0) {
                size_t newMainIndex = mainIndex;
                bool needRankFilterring = (TSortingAlgorithm<TDoc>::NeedRegroupByMainrank == 1);
                if (needRankFilterring && (search->Count() <= Config.RegroupingConfig.MaxRegroupableHypothesisSize)) {
                    for (size_t i = 0;
                         i < Config.RegroupingConfig.GetSteps(groupIndexes.size());
                         ++i) {
                        if (NeedRegroupByMainrank(hypothesis, newMainIndex, groupIndexes, &newMainIndex)) {
                            groupIndexes.clear();
                            search->FindNeighbors(newMainIndex, &groupIndexes);
                        } else {
                            needRankFilterring = false;
                            break;
                        }
                    }
                }
                const size_t s = groupIndexes.size();
                if (needRankFilterring) {
                    for (size_t i = 0; i < s; ++i) {
                        if (hypothesis[groupIndexes[i]]->Mainrank <= hypothesis[newMainIndex]->Mainrank) {
                            Handler->OnDocument(*hypothesis[groupIndexes[i]], hypothesis[newMainIndex]);
                            processedIndexes.push_back(groupIndexes[i]);
                        }
                    }
                    groupIndexes.clear();
                    Handler->OnDocument(*hypothesis[newMainIndex], nullptr);
                } else {
                    for (size_t i = 0; i < s; ++i) {
                        Handler->OnDocument(*hypothesis[groupIndexes[i]], hypothesis[newMainIndex]);
                    }
                    processedIndexes.swap(groupIndexes);
                    Handler->OnDocument(*hypothesis[newMainIndex], nullptr);
                }
                processedIndexes.push_back(newMainIndex);
            } else {
                Handler->OnDocument(*hypothesis[mainIndex], nullptr);
                processedIndexes.push_back(mainIndex);
            }

            search->Remove(processedIndexes);
            const size_t s = processedIndexes.size();
            for (size_t i = 0; i < s; ++i) {
                processed[processedIndexes[i]] = true;
            }
        }

        if (Y_UNLIKELY(search->Count() > 0)) {
            ythrow TWithBackTrace<yexception>() << "Some docs are remain";
        }
    }

private:
    void MakeVectorOfPointers(
        const TVector<TDoc>& hypothesis,
        TVector<const TDoc*>* phypothesis) const {
        const size_t s = hypothesis.size();
        phypothesis->resize(s, nullptr);
        for (size_t i = 0; i < s; ++i) {
            phypothesis->operator[](i) = &hypothesis[i];
        }
    }

    bool NeedRegroupByMainrank(
        const TVector<const TDoc*>& hypothesis,
        const size_t mainIndex,
        const TVector<size_t>& group,
        size_t* newMainIndex) const {
        size_t res = mainIndex;
        const size_t s = group.size();
        for (size_t i = 0; i < s; ++i) {
            if (hypothesis[res]->Mainrank < hypothesis[group[i]]->Mainrank) {
                res = group[i];
            }
        }
        if (res == mainIndex) {
            return false;
        } else {
            *newMainIndex = res;
            return true;
        }
    }

    void FilterUnprocessed(
        const TVector<const TDoc*>& hypothesis,
        const TVector<bool>& processed,
        TVector<const TDoc*>* unprocessed) const {
        const size_t s = hypothesis.size();
        for (size_t i = 0; i < s; ++i) {
            if (!processed[i]) {
                unprocessed->push_back(hypothesis[i]);
            }
        }
    }

private:
    TClustererConfig& Config;
    THandler* Handler;
};
//
/////////////////////////////////////////////////////////////////////
