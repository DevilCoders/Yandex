#pragma once

#include "check.h"
#include <kernel/dups/proto/pessimized_clones.pb.h>
#include <kernel/search_daemon_iface/relevance_type.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <search/idl/events.ev.pb.h>
#include <util/string/cast.h>
#include <util/generic/refcount.h>
#include <util/generic/ptr.h>
#include <util/generic/maybe.h>

namespace NDups {
    const size_t NO_REARRANGER_SURVIVORS = 1;

    struct TGluedDoc: public TCheckedDoc {
        /// resulting position in output
        size_t RearrWithDocOnPosition;

        /// true if doc looses in it's best dup group
        bool IsRemoved;

        /// true if the relevance of document needs to be decreased
        bool NeedPessimization;
        TMaybe<TString> OriginalDocumentUrl;

        /// true if resulting position differs from source position
        bool GetIsRearranged() const {
            return DocPosition != RearrWithDocOnPosition;
        }

        TGluedDoc(const TCheckedDoc& doc)
            : TCheckedDoc(doc)
            , RearrWithDocOnPosition(doc.DocPosition)
            , IsRemoved(false)
            , NeedPessimization(false)
        {}

        /// apriory less
        bool operator <(const TGluedDoc& other) const {
            if (IsRemoved != other.IsRemoved)
                return !IsRemoved;

            return false;
        }

        /// apriory equal
        bool operator ==(const TGluedDoc& other) const {
            return IsRemoved == other.IsRemoved;
        }

        bool operator !=(const TGluedDoc& other) const {
            return !(*this == other);
        }
    };

    struct TDupPessimizationMultiplier {
        bool Enabled = false;
        TMaybe<float> Value = Nothing(); // Nothing equals 1.f

        TDupPessimizationMultiplier() = default;

        TDupPessimizationMultiplier(TMaybe<float> value)
            : Enabled(true)
            , Value(value)
        {
        }
    };

    class TDupsGroup {
    private:
        /// indexes of docs in group
        TVector<size_t> DocIndexes;

        /// output group positions before rearranging
        TVector<size_t> OldPositions;

        TCheckedDoc MainDoc;
        EDupType DupType;

        size_t MainDocIdx;

    public:
        typedef TVector<size_t>::iterator iterator;

        TDupsGroup(const TCheckResult& checkResult, size_t mainDocIdx)
            : MainDoc(checkResult.Doc)
            , DupType(checkResult.DupType)
            , MainDocIdx(mainDocIdx)
        {
            DocIndexes.push_back(mainDocIdx);
        }

        EDupType GetDupType() const {
            return DupType;
        }

        const TCheckedDoc& GetMainDoc() const {
            return MainDoc;
        }

        size_t GetMainDocIdx() const {
            return MainDocIdx;
        }

        iterator begin() {
            return DocIndexes.begin();
        }

        iterator end() {
            return DocIndexes.end();
        }

        void Add(size_t index) {
            DocIndexes.push_back(index);
        }

        size_t operator [](size_t index) const {
            return DocIndexes[index];
        }

        size_t Size() const {
            return DocIndexes.size();
        }

        void SavePositions() {
            OldPositions = DocIndexes;
        }

        size_t GetOldPosition(size_t index) const {
            return OldPositions[index];
        }

        static bool ByPositionAsc (const TDupsGroup& v, const TDupsGroup& other) {
            if (v.MainDoc.DocPosition == other.MainDoc.DocPosition) {
                return v.DupType < other.DupType;
            }
            return v.MainDoc.DocPosition < other.MainDoc.DocPosition;
        }

        static bool ByPositionDesc (const TDupsGroup& v, const TDupsGroup& other) {
            if (v.MainDoc.DocPosition == other.MainDoc.DocPosition) {
                return v.DupType < other.DupType;
            }
            return v.MainDoc.DocPosition > other.MainDoc.DocPosition;
        }
    };

    /// rearranger interface
    class IRearrange : public TSimpleRefCount<IRearrange> {
    public:
        virtual ~IRearrange() {}

        /// just sort the [begin, end) range
        virtual void Rearrange(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs) = 0;

        /// number of docs to survive after rearr
        virtual size_t GetSurvivorCount() const = 0;
        virtual void CalcSurvivorCount(TDupsGroup::iterator /*begin*/, TDupsGroup::iterator /*end*/, TVector<TGluedDoc>& /*docs*/, const TMaybe<size_t>& /*maxRemovedDocs*/) = 0;
    };

    using TRearrangePtr = TIntrusivePtr<IRearrange>;

    /// Helper for TRearrangeDupsImpl
    struct TRelevFactors: public TMap<TString, double> {
        TRelevFactors()
        {}

        void Add(const TString& name) {
            insert(std::make_pair(name, 0));
        }

        // This evil thingie (pointer to the factor value) is required by muParser
        double* Get(const TString& name) {
            return &(*this)[name];
        }

        double Get(const TString& name) const {
            const_iterator it = find(name);
            return it == end() ? 0 : it->second;
        }
    };

    /// Formula interface, helper for TRearrangeDupsImpl
    class IRearrFml {
    public:
        IRearrFml() {}
        virtual ~IRearrFml() {}
        virtual double Eval() const = 0;
        virtual TRelevFactors& GetVariables() = 0;
    };

    /// Helper for TRearrangeDupsImpl
    class TDynamicRearrFml: public IRearrFml {
    public:
        TDynamicRearrFml(const TString& fml)
            : Formula(fml)
        {}

        ~TDynamicRearrFml() override
        {}

        double Eval() const override {
            return Formula.Eval();
        }

        TRelevFactors& GetVariables() override {
            return Formula.GetVariables();
        }

    private:
        TFormula<TRelevFactors> Formula;
    };

    /// Doc with fml score, helper for TRearrangeDupsImpl
    struct TScoredDoc {
        size_t DocIndex;
        size_t GroupIndex;
        double Score;

        TScoredDoc(size_t docIndex, size_t groupIndex, double score)
            : DocIndex(docIndex)
            , GroupIndex(groupIndex)
            , Score(score)
        {}
    };

    struct TRearrangeDupsTriggers {
        bool GlueNotShopRegUrls;
        bool IsWikipediaUp;
        bool IsCatalogueCardDown;
        bool IsRepostDown;
    };


    class TSurvivorCountConstraints {
    public:
        static size_t Calc(size_t minSurvivorCount, size_t dupsInGroup, const TMaybe<size_t>& maxRemovedDocs, size_t alreadyCheckedDocsCount = 0) {

            // If maxRemovedDocs is empty its values treated as 'dupsInGroup'
            size_t minSurvivorByMaxRemovedCodition = dupsInGroup - std::min(maxRemovedDocs.GetOrElse(dupsInGroup), dupsInGroup);

            auto c = std::max(minSurvivorCount, std::max(alreadyCheckedDocsCount, minSurvivorByMaxRemovedCodition));

            return std::min(c, dupsInGroup);
        }
    };

    template <typename TParams>
        class TRearrangerAddCheckedDocumentsToSurvivors : public IRearrange {
    private:
        TParams& Params;
        size_t SurvivorCount = 0;

    protected:
        TRearrangerAddCheckedDocumentsToSurvivors(TParams& params)
            : Params(params)
        {}

        TParams& GetParams() { return Params; }
        const TParams& GetParams() const { return Params; }

        void ApplyConstraintsAndSaveSurvivorCount(size_t minSurvivorCount, TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs,
                                                  const TMaybe<size_t>& maxRemovedDocs) {

            auto checkedSources = TCheckedSourceFrequencies::Count(begin, end, [this, &docs](const auto& v) { return Params.GetCheckedSourceType(docs[v].DocPosition); });

            SurvivorCount = TSurvivorCountConstraints::Calc(minSurvivorCount, std::distance(begin, end), maxRemovedDocs, checkedSources.GetMostFrequentCheckedSourceCount());
        }

    public:
        size_t GetSurvivorCount() const override {
            return SurvivorCount;
        }
    };

    template <typename TParams>
        class TRearrangeSurviveSingleNonCheckedImpl : public TRearrangerAddCheckedDocumentsToSurvivors<TParams> {
    public:
        explicit TRearrangeSurviveSingleNonCheckedImpl(TParams& params)
            : TRearrangerAddCheckedDocumentsToSurvivors<TParams>(params)
        {}

        void CalcSurvivorCount(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs, const TMaybe<size_t>& maxRemovedDocs) override {
            this->ApplyConstraintsAndSaveSurvivorCount(NO_REARRANGER_SURVIVORS, begin, end, docs, maxRemovedDocs);
        }

        void Rearrange(TDupsGroup::iterator, TDupsGroup::iterator, TVector<TGluedDoc>&) override {}
    };


    template <typename TParams>
        class TRearrangeDupsImpl: public TRearrangerAddCheckedDocumentsToSurvivors<TParams> {
    private:
        typedef typename TParams::TRelevance TRelevance;

    public:
        TRearrangeDupsImpl(TParams& params, double relevBound, IRearrFml* fml, const TRearrangeDupsTriggers& triggers)
            : TRearrangerAddCheckedDocumentsToSurvivors<TParams>(params)
            , RelevBound(relevBound)
            , Formula(fml)
            , Triggers(triggers)
        {}

        ~TRearrangeDupsImpl() override
        {}

        void CalcSurvivorCount(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs, const TMaybe<size_t>& maxRemovedDocs) override {
            size_t survivorCount = 0;
            const TRelevance topRelev = this->GetParams().GetRelevance(docs[*begin].DocPosition);
            TVector<TScoredDoc> scores;
            scores.reserve(end - begin);

            bool regUrlFound = false;

            for (TDupsGroup::iterator current = begin; current != end; ++current) {
                const TRelevance currentRelev(this->GetParams().GetRelevance(docs[*current].DocPosition));

                if (this->GetParams().DontGlue(docs[*current].DocPosition)) {
                    ++survivorCount;
                } else if (this->GetParams().IsQueryRegionInUrl(docs[*current].DocPosition)) {
                    if (current != begin && (!regUrlFound || !Triggers.GlueNotShopRegUrls || this->GetParams().IsShop(docs[*current].DocPosition))) {
                        ++survivorCount;
                    }
                    regUrlFound = true;
                }

                if (PassesRelevBound(currentRelev, topRelev)) {
                    double score = 0;
                    if (!this->GetParams().IsDupCateg(docs[*current].DocPosition))
                        score = GetScore(docs[*current].DocPosition);
                    scores.push_back(TScoredDoc(*current, current - begin, score));
                }
            }

            if (scores.size() == 0)
                return;

            TVector<TScoredDoc>::iterator bestDoc = MinElement(
                scores.begin(),
                scores.end(),
                TDupsLess(this->GetParams(), docs, Triggers)
            );

            if (!this->GetParams().DontGlue(docs[bestDoc->DocIndex].DocPosition)) {
                ++survivorCount;
            }

            this->ApplyConstraintsAndSaveSurvivorCount(survivorCount, begin, end, docs, maxRemovedDocs);
        }

        void Rearrange(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs) override {
            const TRelevance topRelev = this->GetParams().GetRelevance(docs[*begin].DocPosition);
            TVector<TScoredDoc> scores;
            scores.reserve(end - begin);

            for (TDupsGroup::iterator current = begin; current != end; ++current) {
                const TRelevance currentRelev(this->GetParams().GetRelevance(docs[*current].DocPosition));

                if (!PassesRelevBound(currentRelev, topRelev))
                    break;

                double score = 0;
                if (!this->GetParams().IsDupCateg(docs[*current].DocPosition))
                    score = GetScore(docs[*current].DocPosition);

                scores.push_back(TScoredDoc(*current, current - begin, score));
            }

            if (scores.size() == 0)
                return;

            TVector<TScoredDoc>::iterator bestDoc = MinElement(
                scores.begin(),
                scores.end(),
                TDupsLess(this->GetParams(), docs, Triggers)
            );

            DoSwap(*begin, *(begin + bestDoc->GroupIndex));

            bool regUrlFound = false;

            if (this->GetSurvivorCount() > 1) {
                TDupsGroup::iterator socialIt = begin;
                ++socialIt;

                for (TDupsGroup::iterator current = begin; current != end; ++current) {
                    bool needSwap = false;
                    if (current != begin && this->GetParams().DontGlue(docs[*current].DocPosition)) {
                        needSwap = true;
                    } else if (this->GetParams().IsQueryRegionInUrl(docs[*current].DocPosition)) {
                        if (current != begin && (!regUrlFound || !Triggers.GlueNotShopRegUrls || this->GetParams().IsShop(docs[*current].DocPosition))) {
                            needSwap = true;
                        }
                        regUrlFound = true;
                    }
                    if (needSwap) {
                        DoSwap(*socialIt, *current);
                        ++socialIt;
                    }
                }
            }
        }

    private:
        class TDupsLess {
        private:
            const TParams& Params;
            const TVector<TGluedDoc>& Docs;
            const TRearrangeDupsTriggers& Triggers;

        public:
            TDupsLess(const TParams& params, const TVector<TGluedDoc>& docs, const TRearrangeDupsTriggers& triggers)
                : Params(params)
                , Docs(docs)
                , Triggers(triggers)
            {}

            bool operator ()(const TScoredDoc& first, const TScoredDoc& second) const {
                const TGluedDoc& dupOne = Docs[first.DocIndex];
                const TGluedDoc& dupTwo = Docs[second.DocIndex];

                // apriory comparison
                if (dupOne != dupTwo)
                    return dupOne < dupTwo;

                if (Triggers.IsWikipediaUp) {
                    bool firstIsWikipedia = Params.IsWikipedia(dupOne.DocPosition);
                    bool secondIsWikipedia = Params.IsWikipedia(dupTwo.DocPosition);
                    if (firstIsWikipedia != secondIsWikipedia)
                        return firstIsWikipedia;
                }

                if (Triggers.IsCatalogueCardDown) {
                    bool firstIsCatalogueCard = Params.IsCatalogueCard(dupOne.DocPosition);
                    bool secondIsCatalogueCard = Params.IsCatalogueCard(dupTwo.DocPosition);
                    if (firstIsCatalogueCard != secondIsCatalogueCard)
                        return !firstIsCatalogueCard;
                }

                if (first.Score != second.Score)
                    return first.Score > second.Score;

                if (Triggers.IsRepostDown) {
                    bool firstIsRepost = Params.IsRepost(dupOne.DocPosition);
                    bool secondIsRepost = Params.IsRepost(dupTwo.DocPosition);
                    if (firstIsRepost != secondIsRepost)
                        return !firstIsRepost;
                }

                return dupOne.DocPosition < dupTwo.DocPosition;
            }
        };

        bool PassesRelevBound(const TRelevance& currentRelev, const TRelevance& topRelev) const {
            return (topRelev - currentRelev) <= RelevBound * topRelev;
        }

        double GetScore(size_t idx) const {
            if (!Formula)
                return 0;
            this->GetParams().LoadFactors(Formula->GetVariables(), idx);
            return Formula->Eval();
        }

    private:
        const double RelevBound;
        IRearrFml* Formula;
        TRearrangeDupsTriggers Triggers;
    };


    template <typename TParams>
        class TRearrangeClonesImpl: public TRearrangerAddCheckedDocumentsToSurvivors<TParams> {
    private:
        size_t DefaultUnglue;
        bool SpecialUnglueNavLogic;
        bool ForceNavUnglue;
        bool PessimizeAttrDups;

    private:
        size_t CalcUnglueCount(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs) const {
            if (!SpecialUnglueNavLogic)
                return DefaultUnglue;

            for (TDupsGroup::iterator current = begin; current != end; ++current) {
                if (this->GetParams().IsCatalogueCard(docs[*current].DocPosition)
                    || this->GetParams().IsLiveJournal(docs[*current].DocPosition)) {
                        return 2;
                }
            }

            if (ForceNavUnglue) {
                for (TDupsGroup::iterator current = begin; current != end; ++current) {
                    if (this->GetParams().IsNavGroupResult(docs[*current].DocPosition)) {
                        return 2;
                    }
                }
                return 1;
            }

            return 1;
        }

    public:
        TRearrangeClonesImpl(TParams& params, size_t unglue, bool specialUnglueNavLogic, bool forceNavUnglue, bool pessimizeAttrDups)
            : TRearrangerAddCheckedDocumentsToSurvivors<TParams>(params)
            , DefaultUnglue(unglue)
            , SpecialUnglueNavLogic(specialUnglueNavLogic)
            , ForceNavUnglue(forceNavUnglue)
            , PessimizeAttrDups(pessimizeAttrDups)
        {}

        ~TRearrangeClonesImpl() override
        {}

        void Rearrange(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs) override {
            auto dupsCount = std::distance(begin, end);
            if (!SpecialUnglueNavLogic) {
                TDupsGroup::iterator middle = begin + Min<size_t>(this->GetSurvivorCount(), dupsCount);
                std::partial_sort(begin, middle, end, TClonesLess(this->GetParams(), docs));
                return;
            }

            if (dupsCount <= 1)
                return;

            auto bestDoc = MinElement(begin, end, TClonesLess(this->GetParams(), docs));
            DoSwap(*begin, *bestDoc);

            auto secondBest = MinElement(begin + 1, end, TNavClonesLess(this->GetParams(), docs));
            DoSwap(*(begin + 1), *secondBest);
        }

        void CalcSurvivorCount(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs, const TMaybe<size_t>& maxRemovedDocs) override {
            TMaybe<size_t> realMaxRemovedDocs = maxRemovedDocs;
            // In case we want to pessimize attribute duplicates no deletion is allowed
            if (PessimizeAttrDups) {
                realMaxRemovedDocs = 0;
            }
            TRearrangerAddCheckedDocumentsToSurvivors<TParams>::ApplyConstraintsAndSaveSurvivorCount(
                CalcUnglueCount(begin, end, docs),
                begin, end, docs, realMaxRemovedDocs);

        }

        class TNavClonesLess {
        private:
            typedef typename TParams::TRelevance TRelevance;
            const TParams& Params;
            const TVector<TGluedDoc>& Docs;

        public:
            TNavClonesLess(const TParams& params, const TVector<TGluedDoc>& docs)
                : Params(params)
                , Docs(docs)
            {}

            bool operator ()(size_t first, size_t second) const {
                const TGluedDoc& cloneOne = Docs[first];
                const TGluedDoc& cloneTwo = Docs[second];

                bool firstIsGood = Params.IsCatalogueCard(cloneOne.DocPosition) || Params.IsLiveJournal(cloneOne.DocPosition);
                bool secondIsGood = Params.IsCatalogueCard(cloneOne.DocPosition) || Params.IsLiveJournal(cloneOne.DocPosition);

                if (firstIsGood != secondIsGood)
                    return firstIsGood;

                return cloneOne.DocPosition < cloneTwo.DocPosition;
            }
        };

        class TClonesLess {
        private:
            typedef typename TParams::TRelevance TRelevance;
            const TParams& Params;
            const TVector<TGluedDoc>& Docs;

        public:
            TClonesLess(const TParams& params, const TVector<TGluedDoc>& docs)
                : Params(params)
                , Docs(docs)
            {}

            bool operator ()(size_t first, size_t second) const {
                const TGluedDoc& cloneOne = Docs[first];
                const TGluedDoc& cloneTwo = Docs[second];

                // apriory comparison
                if (cloneOne != cloneTwo)
                    return cloneOne < cloneTwo;

                bool firstIsWikipedia = Params.IsWikipedia(cloneOne.DocPosition);
                bool secondIsWikipedia = Params.IsWikipedia(cloneTwo.DocPosition);
                if (firstIsWikipedia != secondIsWikipedia)
                    return firstIsWikipedia;

                bool firstCloneBadness = GetIsCloneBad(cloneOne);
                bool secondCloneBadness = GetIsCloneBad(cloneTwo);
                if (firstCloneBadness != secondCloneBadness)
                    return !firstCloneBadness;

                return cloneOne.DocPosition < cloneTwo.DocPosition;
            }

        private:
            bool GetIsCloneBad(const TGluedDoc& cloneDoc) const {
                return Params.GetRelevance(cloneDoc.DocPosition) < NAVIGATE_RELEVANCE &&
                       (Params.IsFake(cloneDoc.DocPosition) && !Params.IsFakeForBan(cloneDoc.DocPosition) ||
                        Params.IsCatalogueCard(cloneDoc.DocPosition));
            }
        };
    };

    template <typename TParams>
        class TRearrangeByRegUrlImpl: public TRearrangerAddCheckedDocumentsToSurvivors<TParams> {
    public:
        TRearrangeByRegUrlImpl(TParams& params, NDups::TUrlZoneManager* regUrlManager)
            : TRearrangerAddCheckedDocumentsToSurvivors<TParams>(params)
            , RegUrlManager(regUrlManager)
        {}

        ~TRearrangeByRegUrlImpl() override {}

        bool RegUrlSurvivor(size_t idx) const {
            return (this->GetParams().IsRegionalUrl(idx, RegUrlManager) || this->GetParams().GetRelevance(idx) >= NAVIGATE_RELEVANCE);
        }

        void CalcSurvivorCount(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs, const TMaybe<size_t>& maxRemovedDocs) override {
            size_t survivorCount = 0;

            for (TDupsGroup::iterator current = begin; current != end; ++current) {
                if (RegUrlSurvivor(docs[*current].DocPosition))
                    ++survivorCount;
            }

            if (survivorCount == 0)
                survivorCount = 1;

            this->ApplyConstraintsAndSaveSurvivorCount(survivorCount, begin, end, docs, maxRemovedDocs);
        }

        void Rearrange(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs) override {
            TDupsGroup::iterator surviveIt = begin;
            for (TDupsGroup::iterator current = begin; current != end; ++current) {
                if (RegUrlSurvivor(docs[*current].DocPosition)) {
                    this->GetParams().ResetDocMarker(docs[*current].DocPosition);
                    DoSwap(*surviveIt, *current);
                    ++surviveIt;
                    if (static_cast<size_t>(std::distance(begin, surviveIt)) == this->GetSurvivorCount())
                        break;
                }
            }
        }
    private:
        NDups::TUrlZoneManager* RegUrlManager = nullptr;
    };

    template <typename TParams>
    class TRearrangeByRedirectUrlImpl: public TRearrangerAddCheckedDocumentsToSurvivors<TParams> {
    public:
        TRearrangeByRedirectUrlImpl(TParams& params)
            : TRearrangerAddCheckedDocumentsToSurvivors<TParams>(params)
        {}

        void CalcSurvivorCount(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs, const TMaybe<size_t>& maxRemovedDocs) override {
            TRearrangerAddCheckedDocumentsToSurvivors<TParams>::ApplyConstraintsAndSaveSurvivorCount(1, begin, end, docs, maxRemovedDocs);
        }

        void Rearrange(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs) override {
            // Move first non-fake doc to the first position
            TDupsGroup::iterator nonFake = end, nonErfFake = end, navDoc = end;
            for (TDupsGroup::iterator it = begin; it != end; ++it) {
                int idx = docs[*it].DocPosition;
                if (this->GetParams().GetRelevance(idx) >= NAVIGATE_RELEVANCE && navDoc == end) {
                    navDoc = it;
                    break;
                }
                if (!this->GetParams().IsErfFake(idx) && nonErfFake == end)
                    nonErfFake = it;
                if (!this->GetParams().IsFake(idx) && nonFake == end)
                    nonFake = it;
            }
            TDupsGroup::iterator best = navDoc != end ? navDoc :
                                        nonErfFake != end ? nonErfFake : nonFake;
            if (best != begin && best != end) {
                this->GetParams().ResetDocMarker(docs[*best].DocPosition);
                DoSwap(*begin, *best);
            }
        }
    };

    template <typename TParams>
    class TRearrangeByRelCanonicalUrlImpl: public TRearrangerAddCheckedDocumentsToSurvivors<TParams> {
    public:
        TRearrangeByRelCanonicalUrlImpl(TParams& params)
            : TRearrangerAddCheckedDocumentsToSurvivors<TParams>(params)
        {}

        void CalcSurvivorCount(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs, const TMaybe<size_t>& maxRemovedDocs) override {
            TRearrangerAddCheckedDocumentsToSurvivors<TParams>::ApplyConstraintsAndSaveSurvivorCount(1, begin, end, docs, maxRemovedDocs);
        }

        void Rearrange(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs) override {

            std::sort(begin, end, [this, &docs](size_t first, size_t second) {
                bool firstHasCanonical = this->GetParams().HasRelCanonicalTarget(docs[first].DocPosition);
                bool secondHasCanonical = this->GetParams().HasRelCanonicalTarget(docs[second].DocPosition);

                if (!(this->GetParams().IsWikipedia(docs[first].DocPosition) && this->GetParams().IsWikipedia(docs[second].DocPosition))) {
                    if (firstHasCanonical != secondHasCanonical)
                        return secondHasCanonical;
                }

                return docs[first].DocPosition < docs[second].DocPosition;
            });
        }
    };

    template <typename TParams>
    class TRearrangeByOriginalUrlImpl: public TRearrangerAddCheckedDocumentsToSurvivors<TParams> {
    public:
        TRearrangeByOriginalUrlImpl(TParams& params, IBannedInfo& originalsChecker)
            : TRearrangerAddCheckedDocumentsToSurvivors<TParams>(params)
            , OriginalsChecker(originalsChecker)
        {}

        void CalcSurvivorCount(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs, const TMaybe<size_t>& maxRemovedDocs) override {
            TRearrangerAddCheckedDocumentsToSurvivors<TParams>::ApplyConstraintsAndSaveSurvivorCount(1, begin, end, docs, maxRemovedDocs);
        }

        void Rearrange(TDupsGroup::iterator begin, TDupsGroup::iterator end, TVector<TGluedDoc>& docs) override {
            TDupsGroup::iterator best = begin;
            for (int attempts = 0; attempts <= end - begin; ++attempts) {
                bool changed = false;
                for (TDupsGroup::iterator current = begin; current != end; ++current) {
                    if (current != best && OriginalsChecker.CheckOriginality(docs[*current].Url, docs[*best].Url) == IBannedInfo::EOriginality::FirstIsOriginal) {
                       best = current;
                       changed = true;
                    }
                }
                if (!changed) {
                    if (best != begin)
                        DoSwap(*best, *begin);
                    return;
                }
            }
            // if we are here, there is a loop somewhere in OriginalsChecker data;
            // do nothing, this fallbacks to keeping most-relevant document
        }

    private:
        IBannedInfo& OriginalsChecker;
    };

    /// perform rearranges in groups and commit results
    class TRearrange {
    private:
        /// used at Commit()
        struct TGluedDocLess {
            bool operator ()(const TGluedDoc& a, const TGluedDoc& b) const {
                return a.RearrWithDocOnPosition < b.RearrWithDocOnPosition;
            }
        };

    private:
        typedef THashMap<EDupType, TRearrangePtr> TRearrangers;
        typedef TMap<TCheckResult, size_t> TGroupMap;

    private:
        TRearrangers Rearrangers;
        TRearrangePtr DefaultRearranger;

        TVector<TGluedDoc> Docs;
        THashMap<size_t, size_t> DocPositionToIdxMap;

        TVector<TDupsGroup> Groups;
        TGroupMap GroupMap;

        TDupPessimizationMultiplier AttrClonesPessimizationMultiplier = TDupPessimizationMultiplier();


    public:
        TRearrange(size_t size, TDupPessimizationMultiplier attrDupRelevanceMultiplier = TDupPessimizationMultiplier())
            : AttrClonesPessimizationMultiplier(attrDupRelevanceMultiplier)
        {
            Docs.reserve(size);
        }

        void AddRearranger(const TVector<EDupType>& types, TRearrangePtr rearranger) {
            if (rearranger) {
                for (auto type : types) {
                    Rearrangers.insert(std::make_pair(type, rearranger));
                }
            }
        }

        void AddDefaultRearranger(TRearrangePtr rearranger) {
            DefaultRearranger = rearranger;
        }

        void AddDoc(const TCheckedDoc& doc, const TVector<TCheckResult>& checkResults) {
            size_t documentIndex = Docs.size();
            Docs.push_back(TGluedDoc(doc));
            DocPositionToIdxMap[doc.DocPosition] = Docs.size() - 1;

            for (const auto& checkResult : checkResults) {
                if (auto groupIdxPtr = GroupMap.FindPtr(checkResult)) {
                    Groups[*groupIdxPtr].Add(documentIndex);
                } else {
                    if (auto mainDocIdxPtr = DocPositionToIdxMap.FindPtr(checkResult.Doc.DocPosition)) {
                        TDupsGroup newGroup(checkResult, *mainDocIdxPtr);
                        newGroup.Add(documentIndex);
                        Groups.push_back(newGroup);
                        GroupMap[checkResult] = Groups.size() - 1;
                    }
                }
            }
        }

        template <typename TFeedback>
        void Rearrange(TFeedback& feedback, bool reverseRemoveOrder, TMaybe<size_t> maxRemovedDocs = TMaybe<size_t>()) {
            // without rearranger only first element from group survives
            Sort(Groups.begin(), Groups.end(), reverseRemoveOrder ? TDupsGroup::ByPositionDesc : TDupsGroup::ByPositionAsc);

            for (size_t groupIndex = 0; groupIndex < Groups.size(); ++groupIndex) {
                TDupsGroup& group = Groups[groupIndex];

                auto survivorCount = RearrangeSingleGroup(SearchRearranger(group.GetDupType()), group, maxRemovedDocs);

                SaveGroupRearrangeResults(group, survivorCount, Docs, feedback);
                if (maxRemovedDocs)
                    maxRemovedDocs = *maxRemovedDocs - Min(*maxRemovedDocs, group.Size() - survivorCount);
            }
        }

        size_t GetDocsTotal() const {
            return Docs.size();
        }

        size_t GetSurvivorsCount() const {
            size_t cnt = 0;
            for (auto curDoc = Docs.begin(); curDoc != Docs.end(); ++curDoc)
                if (!curDoc->IsRemoved)
                    ++cnt;
            return cnt;
        }

        // Used for clone pessimization
        static bool IsAttributeDuplicate(EDupType dupType) {
            return dupType == EDupType::BY_ATTR_CLONE || dupType == EDupType::BY_ATTR_CLONE_TEST || dupType == EDupType::BY_ATTR_OFFLINE_DUPS;
        }

        static TRelevance MultiplyRelevance(TRelevance relev, TMaybe<float> x) {
            if (x.Defined()) {
                return ((relev - 1e8) / 1e7 * x.GetRef()) * 1e7 + 1e8;
            }
            return relev;
        }

        template <typename TGroup, typename TGrouping, typename TFixRelevance, typename TRearrangeParams>
        void Commit(TGrouping& grouping, TFixRelevance& fixRelevance, TRearrangeParams& rearrangeParams, bool moveDupsToTail, bool disableSort) {
            Y_UNUSED(rearrangeParams);
            size_t group_cnt = grouping.Size();
            if (!group_cnt)
                return;

            TVector<TGroup*> nonDups, pessimized;
            TVector<std::pair<TGroup*,size_t>> dups;
            nonDups.reserve(group_cnt);
            pessimized.reserve(group_cnt);
            dups.reserve(group_cnt);

            std::sort(Docs.begin(), Docs.end(), TGluedDocLess());

            TPessimizedClones pessimizedClones;
            THashMap<TString, ui32> urlToCloneIndex;

            const size_t minDupPos = group_cnt - 1;
            for (TVector<TGluedDoc>::iterator curDoc = Docs.begin(); curDoc != Docs.end(); ++curDoc) {
                auto& group = grouping.GetMetaGroup(curDoc->DocPosition);
                if (curDoc->IsRemoved) {
                    if (moveDupsToTail)
                        dups.push_back(std::make_pair(&group, curDoc->DocPosition));
                }
                else {
                    if (curDoc->GetIsRearranged()) {
                        TRelevance newRelevance = grouping.GetMetaGroup(curDoc->RearrWithDocOnPosition).GetRelevance();
                        fixRelevance(curDoc->DocPosition, newRelevance);
                    }
                    if (curDoc->NeedPessimization) {
                        Y_ENSURE(AttrClonesPessimizationMultiplier.Enabled, "NeedPessimization should be activated only within AttrClonesPessimizationMultiplier");
                        curDoc->NeedPessimization = false;
                        TRelevance oldRelevance = grouping.GetMetaGroup(curDoc->DocPosition).GetRelevance();
                        TRelevance newRelevance = MultiplyRelevance(oldRelevance, AttrClonesPessimizationMultiplier.Value);
                        fixRelevance(curDoc->DocPosition, newRelevance);

                        urlToCloneIndex[curDoc->Url] = pessimizedClones.GetPessimizedClone().size();
                        TPessimizedClone* pessimizedClone = pessimizedClones.AddPessimizedClone();
                        if (curDoc->OriginalDocumentUrl.Defined()) {
                            pessimizedClone->SetOriginalDocUrl(ToString(curDoc->OriginalDocumentUrl.GetRef()));
                        }
                        pessimizedClone->SetPessimizedDocUrl(ToString(curDoc->Url));
                        pessimizedClone->SetOldPosition(curDoc->DocPosition);
                        pessimizedClone->SetOldRelevance(oldRelevance);
                        pessimizedClone->SetNewRelevance(newRelevance);
                        pessimized.push_back(&group);
                    } else {
                        nonDups.push_back(&group);
                    }
                }
            }

            fixRelevance.Commit();
            grouping.Erase(0, group_cnt);
            grouping.Reserve(nonDups.size() + pessimized.size() + dups.size());

            size_t nonDupsPos = 0, pessimizedPos = 0;
            size_t dst = 0;
            while (nonDupsPos < nonDups.size() && pessimizedPos < pessimized.size()) {
                if (nonDups[nonDupsPos]->GetRelevance() >= pessimized[pessimizedPos]->GetRelevance()) {
                    grouping.Insert(dst, nonDups[nonDupsPos]);
                    ++nonDupsPos;
                } else {
                    grouping.Insert(dst, pessimized[pessimizedPos]);
                    ++pessimizedPos;
                }
                ++dst;
            }
            while (nonDupsPos < nonDups.size()) {
                grouping.Insert(dst++, nonDups[nonDupsPos++]);
            }
            while (pessimizedPos < pessimized.size()) {
                grouping.Insert(dst++, pessimized[pessimizedPos++]);
            }

            if (moveDupsToTail) {
                for (size_t i = 0; i < dups.size(); ++i) {
                    size_t dst = Min(Max(minDupPos, dups[i].second), grouping.Size());
                    TGroup& group = *dups[i].first;
                    grouping.Insert(dst, &group);
                }
            }

            if (!disableSort)
                grouping.SortGroups();

            // Iterate over all documents and set new positions of pessimized clones; log results
            for (size_t i = 0; i < grouping.Size(); ++i) {
                const auto& group = grouping.GetMetaGroup(i);
                for (const auto& doc: group.MetaDocs) {
                    if (urlToCloneIndex.contains(doc.Url())) {
                        TPessimizedClone* pessimizedClone = pessimizedClones.MutablePessimizedClone()->Mutable(urlToCloneIndex[doc.Url()]);
                        pessimizedClone->SetNewPosition(i);

                        rearrangeParams.LogEvent(NEvClass::DecreasedCloneRelevance(
                            pessimizedClone->GetOriginalDocUrl(),
                            pessimizedClone->GetPessimizedDocUrl(),
                            pessimizedClone->GetOldRelevance(),
                            pessimizedClone->GetNewRelevance(),
                            pessimizedClone->GetOldPosition(),
                            pessimizedClone->GetNewPosition()));
                    }
                }
            }

            if (!pessimizedClones.GetPessimizedClone().empty()) {
                AddSearchProperty(rearrangeParams, "PessimizedClones_base64", Base64Encode(pessimizedClones.SerializeAsString()));
            }
        }

        const TVector<TGluedDoc>& GetResult() const {
            return Docs;
        }

    private:
        size_t RearrangeSingleGroup(IRearrange* rearranger, TDupsGroup& group, const TMaybe<size_t>& maxRemovedDocs) {
            if (rearranger) {
                rearranger->CalcSurvivorCount(group.begin(), group.end(), Docs, maxRemovedDocs);
                group.SavePositions();
                rearranger->Rearrange(group.begin(), group.end(), Docs);

                return rearranger->GetSurvivorCount();
            } else {
                group.SavePositions();
                return TSurvivorCountConstraints::Calc(NO_REARRANGER_SURVIVORS, group.Size(), maxRemovedDocs);
            }
        }

        IRearrange* SearchRearranger(EDupType dupType) const
        {
            if (auto r = Rearrangers.find(dupType))
                return r->second.Get();

            return DefaultRearranger.Get();
        }

        bool NeedDisableRearrange(const TDupsGroup& group) {
            return group.GetDupType() == EDupType::BY_URL_ORIGINAL || IsAttributeDuplicate(group.GetDupType());
        }

        bool GlueEntireGroup(const TDupsGroup& group) {
            return EqualToOneOf(group.GetDupType(), EDupType::BY_URL_REDIRECT, EDupType::BY_ATTR_FAKE, EDupType::BY_SIG_STATIC_EXACT);
        }

        template <typename TFeedback>
        void SaveGroupRearrangeResults(const TDupsGroup& group, size_t survivorCount, TVector<TGluedDoc>& Docs, TFeedback& feedback) {

            if (Docs[group.GetMainDocIdx()].IsRemoved) {
                if (GlueEntireGroup(group)) {
                    survivorCount = 0;
                } else {
                    return;
                }
            }

            size_t survived = 0;
            for (size_t docIndex = 0; docIndex < group.Size(); ++docIndex) {
                TGluedDoc& currentDoc = Docs[group[docIndex]];

                if (currentDoc.IsRemoved) {
                    continue;
                }

                // Every document in a group is a duplicate except for the main one
                if (AttrClonesPessimizationMultiplier.Enabled
                    && AttrClonesPessimizationMultiplier.Value.Defined()
                    && IsAttributeDuplicate(group.GetDupType())
                    && currentDoc.DocPosition != group.GetMainDocIdx())
                {
                    currentDoc.NeedPessimization = true;
                    currentDoc.OriginalDocumentUrl = Docs[group.GetMainDocIdx()].Url;
                    feedback.ProcessDocumentPessimization(currentDoc.DocPosition);
                }

                size_t dstDocIdx = group.GetOldPosition(docIndex);

                if (survived < survivorCount) {
                    if (!Docs[dstDocIdx].IsRemoved && !Docs[dstDocIdx].GetIsRearranged() && !currentDoc.GetIsRearranged() && dstDocIdx != group[docIndex] && !NeedDisableRearrange(group)) {
                        currentDoc.RearrWithDocOnPosition = Docs[dstDocIdx].DocPosition;
                        Docs[dstDocIdx].RearrWithDocOnPosition = currentDoc.DocPosition;
                        feedback.LogDupsEvent(EEvent::DupRearranged, currentDoc, Docs[dstDocIdx], group.GetDupType());
                    }
                    ++survived;
                } else if (!currentDoc.GetIsRearranged() || !Docs[DocPositionToIdxMap[currentDoc.RearrWithDocOnPosition]].IsRemoved) {
                    currentDoc.IsRemoved = true;
                    const auto& gluedWith = dstDocIdx != group[docIndex] && !Docs[dstDocIdx].IsRemoved ? Docs[dstDocIdx] : group.GetMainDoc();
                    feedback.LogDupsEvent(EEvent::DupRemoved, currentDoc, gluedWith, group.GetDupType());
                }
            }
        }
    };
} // namespace NDups
