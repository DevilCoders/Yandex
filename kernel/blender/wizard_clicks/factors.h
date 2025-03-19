#pragma once

#include "typedefs.h"
#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/generic/ymath.h>

namespace NWizardsClicks {

    class TTokenFactorCalculator {
    public:
        TTokenFactorCalculator(
                const NSc::TValue* cont,
                TStringBuf name,
                TStringBuf navigName,
                double regValue = 1.f,
                int position = -1)
            : Container(cont == nullptr ? &NSc::TValue::DefaultValue() : cont)
            , Name(name)
            , NavigName(navigName)
            , RegValue(regValue)
            , Position(position)
        {}

        TCounterContainerConstWrap Data() const {
            return TCounterContainerConstWrap(Container);
        }

        TWizardCountersConstWrap Wizard() const {
            return Position == -1 ? Data().Wizards()[Name] : Data().PosWizards()[Name][Position];
        }

        TNavigCountersConstWrap Navig() const {
            return Data().Navigs()[NavigName];
        }

        // clicks
        #define CLICK_FACTORS(time)                                                                 \
        double CTR##time() const {                                                                  \
            return RegDiv(Wizard().Clicks##time(), Wizard().Shows());                               \
        }                                                                                           \
        double CorrectedCTR##time() const {                                                         \
            return RegDiv(Wizard().Clicks##time(), Wizard().CorrectedShows());                      \
        }                                                                                           \
        double FRC##time() const {                                                                  \
            return RegDivNorm(Wizard().Clicks##time(), Data().Common().WebClickedRequests##time()); \
        }                                                                                           \

        CLICK_FACTORS(0)
        CLICK_FACTORS(15)
        CLICK_FACTORS(60)
        CLICK_FACTORS(120)

        #undef CLICK_FACTORS

        // weight clicks
        double WeightCTR() const {
            return RegDiv(Wizard().WeightedClicks(), Wizard().Shows());
        }

        double WeightCorrectedCTR() const {
            return RegDiv(Wizard().WeightedClicks(), Wizard().CorrectedShows());
        }

        double WeightFRC() const {
            return RegDivNorm(Wizard().WeightedClicks(), Data().Common().WebWeightedClickedRequests());
        }

        // skips and shows
        double SkipToShows() const {
            return RegDiv(Wizard().Skips(), Wizard().Shows());
        }

        double SkipToCorrectedShows() const {
            return RegDiv(Wizard().Skips(), Wizard().CorrectedShows());
        }

        double SkipFRC() const {
            return RegDivNorm(Wizard().Skips(), Data().Common().WebClickedRequests0());
        }

        double SatShows() const {
            return Saturate(Wizard().Shows());
        }

        // contrast
        double ImagesServiceRequestContrast() const {
            return RegDivNorm(Data().Common().ImagesServiceRequests(), Data().Common().WebRequests());
        }

        double VideoServiceRequestContrast() const {
            return RegDivNorm(Data().Common().VideoServiceRequests(), Data().Common().WebRequests());
        }

        // navig, assume that navig url shows for all queries, shows isn't supported in ralib
        double NavigCTR0() const {
            return RegDiv(Navig().Clicks0(), Data().Common().WebRequests());
        }

        double NavigFRC0() const {
            return RegDivNorm(Navig().Clicks0(), Data().Common().WebClickedRequests0());
        }

        // surplus
        double WinToShows() const {
            return RegDiv(Wizard().Wins(), Wizard().Shows());
        }

        double LossToShows() const {
            return RegDiv(Wizard().Losses(), Wizard().Shows());
        }

        double SurplusToShows() const {
            return RegDiv(Wizard().Wins() - Wizard().Losses(), Wizard().Shows());
        }

    private:
        template <typename TNum, typename TDenom>
        inline double RegDiv(TNum num, TDenom denom) const {
            return static_cast<double>(num) / (static_cast<double>(denom) + RegValue);
        }

        template <typename TNum, typename TDenom>
        inline double RegDivNorm(TNum num, TDenom denom) const {
            return static_cast<double>(num) / (Abs(static_cast<double>(num)) + static_cast<double>(denom) + RegValue);
        }

        template <typename T>
        inline double Saturate(T v) const {
            return static_cast<double>(v) / (Abs(static_cast<double>(v)) + RegValue);
        }

     public:
        const NSc::TValue* Container;
        TStringBuf Name;
        TStringBuf NavigName;
        double RegValue;
        int Position;
    };

    class TVideoSerpTokenFactorCalculator {
    public:
        TVideoSerpTokenFactorCalculator(
                const NSc::TValue* cont,
                long showsThreshold = 20,
                double regValue = 1.f)
            : Container(cont == nullptr ? &NSc::TValue::DefaultValue() : cont)
            , ShowsThreshold(showsThreshold)
            , RegValue(regValue)
        {}

        TVideoSerpTokenFactorCalculator()
            : Container(&NSc::TValue::DefaultValue())
            , ShowsThreshold(20)
            , RegValue(1.f)

        {}

        TCounterContainerConstWrap Data() const {
            return TCounterContainerConstWrap(Container);
        }

        bool IsEmptyContainer() const{
            return Container == nullptr || Container->IsNull();
        }

        TVideoCountersConstWrap Video(const TString& wizName) const {
            return Data().Video()[wizName];
        }

        bool IsSignificantShowsCount(const TString& wizName) const {
            return Video(wizName).Shows() >= ShowsThreshold;
        }

        double VideoWizLossToShows(const TString& wizName) const {
            return RegDiv(Video(wizName).Loss(), Video(wizName).Shows());
        }

        double VideoWizSurplusTotal(const TString& wizName) const {
            return Video(wizName).Win() - Video(wizName).Loss();
        }

        double VideoWizClicksToShows(const TString& wizName) const {
            return RegDiv(Video(wizName).Clicks(), Video(wizName).Shows());
        }

        double VideoWizRefusesToShows(const TString& wizName) const {
            return RegDiv(Video(wizName).Refuses(), Video(wizName).Shows());
        }

        double VideoWizClicksAfterToShows(const TString& wizName) const {
            return RegDiv(Video(wizName).ClicksAfter(), Video(wizName).Shows());
        }

        double VideoWizRefusesToClicksAfter(const TString& wizName) const {
            return RegDiv(Video(wizName).Refuses(), Video(wizName).ClicksAfter());
        }

        double VideoWizWinToShows(const TString& wizName) const {
            return RegDiv(Video(wizName).Win(), Video(wizName).Shows());
        }

        double VideoWizViewsToShows(const TString& wizName) const {
            return RegDiv(Video(wizName).Views(), Video(wizName).Shows());
        }

        double VideoWizSurplusToShows(const TString& wizName) const {
            return RegDiv((Video(wizName).Win() - Video(wizName).Loss()), Video(wizName).Shows());
        }

        double VideoWizClickedHigher(const TString& wizName) const {
            return Video(wizName).ClickedHigher();
        }

        double VideoWizLVTWhenWasHigher(const TString& wizName) const {
            return Video(wizName).LVTWhenWasHigher();
        }

        double VideoWizRefusesToClicks(const TString& wizName) const {
            return RegDiv(Video(wizName).Refuses(), Video(wizName).Clicks());
        }

    private:
        template <typename TNum, typename TDenom>
        inline double RegDiv(TNum num, TDenom denom) const {
            return static_cast<double>(num) / (static_cast<double>(denom) + RegValue);
        }

    public:
        const NSc::TValue* Container;
        long ShowsThreshold;
        double RegValue;
    };
}

