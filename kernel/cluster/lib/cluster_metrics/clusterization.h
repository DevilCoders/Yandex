#pragma once

#include "matching.h"
#include "metrics.h"

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/string/split.h>

#include <util/stream/file.h>

namespace NClusterMetrics {

inline TString SprintfMetric(double value) {
    return value >= 0
           ? Sprintf("%.5lf", value)
           : "n/a";
}

struct TClusterMetrics {
    double AlexRecall;
    double SecondaryAlexRecall;
    double BCubedPrecision;
    double BCubedRecall;
    double BCubedF1;
    double ClusteredShare;

    bool IsFlat;

    void AddMetricDiff(double& metric, double sampleMetric, double baselineMetric) {
        if (sampleMetric >= 0 && baselineMetric >= 0) {
            metric += sampleMetric - baselineMetric;
        }
    }

    void DecreaseMetric(double& metric, double addition) {
        if (addition > 0) {
            metric -= addition;
        }
    }

    TClusterMetrics(double alexRecall = 0,
                    double secondaryAlexRecall = 0,
                    double bCubedPrecision = 0,
                    double bCubedRecall = 0,
                    double clusteredShare = 0,
                    bool isFlat = true)
        : AlexRecall(alexRecall)
        , SecondaryAlexRecall(secondaryAlexRecall)
        , BCubedPrecision(bCubedPrecision)
        , BCubedRecall(bCubedRecall)
        , ClusteredShare(clusteredShare)
        , IsFlat(isFlat)
    {
        BCubedF1 = BCubedPrecision + BCubedRecall
                   ? 2 * BCubedPrecision * BCubedRecall / (BCubedPrecision + BCubedRecall)
                   : 0;
    }

    void Reset() {
        AlexRecall = 0;
        SecondaryAlexRecall = 0;
        BCubedPrecision = 0;
        BCubedRecall = 0;
        BCubedF1 = 0;
        ClusteredShare = 0;
    }

    void AddDiff(const TClusterMetrics& sample, const TClusterMetrics& baseline) {
        AddMetricDiff(AlexRecall, sample.AlexRecall, baseline.AlexRecall);
        AddMetricDiff(SecondaryAlexRecall, sample.SecondaryAlexRecall, baseline.SecondaryAlexRecall);
        AddMetricDiff(BCubedRecall, sample.BCubedRecall, baseline.BCubedRecall);
        AddMetricDiff(BCubedPrecision, sample.BCubedPrecision, baseline.BCubedPrecision);
        AddMetricDiff(BCubedF1, sample.BCubedF1, baseline.BCubedF1);
        AddMetricDiff(ClusteredShare, sample.ClusteredShare, baseline.ClusteredShare);
    }

    void Normalize(size_t recallNormalizer, size_t precisionNormalizer, size_t totalDocumentsCount) {
        if (recallNormalizer) {
            AlexRecall /= recallNormalizer;
            SecondaryAlexRecall /= recallNormalizer;
            BCubedRecall /= recallNormalizer;
        }

        if (precisionNormalizer) {
            BCubedPrecision /= precisionNormalizer;
            BCubedF1 /= precisionNormalizer;
        }

        if (totalDocumentsCount) {
            ClusteredShare /= totalDocumentsCount;
        }
    }
};

inline void PrintIntegralMetrics(IOutputStream& out,
                                 const TClusterMetrics& metrics,
                                 const TClusterMetrics& metricBounds,
                                 const TString& title = "")
{
    if (!!title) {
        out << title << "\n";
    }
    out << Sprintf("AlexRecall    %.5lf (%.5lf)\n", metrics.AlexRecall, metricBounds.AlexRecall);

    if (metrics.IsFlat) {
        out << Sprintf("BCPrecision   %.5lf (%.5lf)\n", metrics.BCubedPrecision, metricBounds.BCubedPrecision)
            << Sprintf("BCRecall      %.5lf (%.5lf)\n", metrics.BCubedRecall, metricBounds.BCubedRecall)
            << Sprintf("BCF1          %.5lf (%.5lf)\n", metrics.BCubedF1, metricBounds.BCubedF1)
            << Sprintf("SAlexRecall   %.5lf (%.5lf)\n", metrics.SecondaryAlexRecall, metricBounds.SecondaryAlexRecall);
    }

    if (metrics.ClusteredShare <= 1) {
        out << Sprintf("ClusterShare  %.5lf\n", metrics.ClusteredShare);
    } else {
        out << Sprintf("Clustered     %.0lf\n", metrics.ClusteredShare);
    }
}

inline void PrintAlexRecallMetrics(IOutputStream& out,
                                   const TClusterMetrics& metrics,
                                   const TClusterMetrics& metricBounds,
                                   const TString& title = "")
{
    if (!!title) {
        out << title << "\n";
    }
    out << Sprintf("AlexRecall    %.5lf (%.5lf)\n", metrics.AlexRecall, metricBounds.AlexRecall);
    out << Sprintf("SAlexRecall   %.5lf (%.5lf)\n", metrics.SecondaryAlexRecall, metricBounds.SecondaryAlexRecall);
}

inline void PrintBCubedMetrics(IOutputStream& out,
                               const TClusterMetrics& metrics,
                               const TClusterMetrics& metricBounds,
                               const TString& title = "")
{
    if (!!title) {
        out << title << "\n";
    }
    out << Sprintf("BCPrecision   %.5lf (%.5lf)\n", metrics.BCubedPrecision, metricBounds.BCubedPrecision)
        << Sprintf("BCRecall      %.5lf (%.5lf)\n", metrics.BCubedRecall, metricBounds.BCubedRecall)
        << Sprintf("BCF1          %.5lf (%.5lf)\n", metrics.BCubedF1, metricBounds.BCubedF1);
}

template <typename TClusterId, typename TClusterElement>
class TClusterizationBase {
private:
    THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > > ClusterElements;
    THashMap<TClusterElement, TVector<TClusterId> > ElementCluster;

    THashMap<TClusterId, TVector<TMatching<TClusterId> > > Matchings;
    THashMap<TClusterId, ui32> ImportantSizes;

    THashMap<TClusterId, TClusterMetrics> ClusterMetrics;
    THashMap<TClusterId, TClusterMetrics> ClusterMetricBounds;

    TClusterMetrics IntegralMetrics;
    TClusterMetrics IntegralMetricBounds;

    bool DoRegularization;
    double Factor;

    TVector<std::pair<TClusterId, float> > EmptyCluster;

    bool IsFlat;
public:
    TClusterizationBase(bool doRegularization = true, double factor = 0)
        : DoRegularization(doRegularization)
        , Factor(factor)
        , IsFlat(true)
    {
    }

    const THashMap<TClusterId, TVector<TMatching<TClusterId> > >& GetMatchings() const {
        return Matchings;
    }

    void Add(const TClusterElement& element, const TClusterId& cluster, float weight = 1) {
        ClusterElements[cluster].push_back(std::make_pair(element, weight));

        TVector<TClusterId>& elementCluster = ElementCluster[element];
        elementCluster.push_back(cluster);

        IsFlat &= elementCluster.size() == 1;
    }

    bool Flat() const {
        return IsFlat;
    }

    size_t Size() const {
        return ElementCluster.size();
    }

    size_t ClustersCount(size_t lowerBound) const {
        if (!lowerBound) {
            return ClusterElements.size();
        }

        size_t result = 0;
        typename THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >::const_iterator it = ClusterElements.begin();
        for (; it != ClusterElements.end(); ++it) {
            result += it->second.size() >= lowerBound;
        }
        return result;
    }

    size_t ClusteredDocumentsCount(size_t lowerBound) const {
        if (!lowerBound) {
            return ClusterElements.size();
        }

        size_t result = 0;
        typename THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >::const_iterator it = ClusterElements.begin();
        for (; it != ClusterElements.end(); ++it) {
            result += it->second.size() >= lowerBound ? it->second.size() : 0;
        }
        return result;
    }

    TClusterMetrics GetIntegralMetrics() const {
        return IntegralMetrics;
    }

    TClusterMetrics GetIntegralMetricsBounds() const {
        return IntegralMetricBounds;
    }

    TClusterMetrics GetClusterMetrics(const TClusterId& cluster) const {
        typename THashMap<TClusterElement, TClusterMetrics>::const_iterator it = ClusterMetrics.find(cluster);
        return it != ClusterMetrics.end()
               ? it->second
               : TClusterMetrics();
    }

    TClusterMetrics GetClusterMetricBounds(const TClusterId& cluster) const {
        typename THashMap<TClusterElement, TClusterMetrics>::const_iterator it = ClusterMetricBounds.find(cluster);
        return it != ClusterMetrics.end()
               ? it->second
               : TClusterMetrics();
    }

    bool Matches(const TClusterId& clusterId, const TClusterElement& element) const {
        if (const TVector<TClusterId>* clusterIds = ElementCluster.FindPtr(element)) {
            return Find(clusterIds->begin(), clusterIds->end(), clusterId) != clusterIds->end();
        }
        return false;
    }

    const TVector<TClusterId>* GetClusters(const TClusterElement& element) const {
        return ElementCluster.FindPtr(element);
    }

    // for flat clustering only
    bool GetClusterId(const TClusterElement& element, TClusterId& clusterId) const {
        if (const TVector<TClusterId>* clusters = ElementCluster.FindPtr(element)) {
            Y_ASSERT(clusters->size() == 1);
            clusterId = clusters->front();
            return true;
        }
        return false;
    }

    const TVector<std::pair<TClusterElement, float> >& GetClusterElements(const TClusterId& clusterId) const {
        typename THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >::const_iterator it = ClusterElements.find(clusterId);
        if (it == ClusterElements.end()) {
            ythrow yexception() << "cluster does not exist: " << clusterId << "\n";
        }
        return it->second;
    }

    size_t GetClusterSize(const TClusterId& clusterId) const {
        typename THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >::const_iterator it = ClusterElements.find(clusterId);
        return it == ClusterElements.end() ? 0 : it->second.size();
    }

    void SetupMetrics(const TClusterizationBase& sampleClusterization,
                      const THashSet<TClusterElement>& importantElements = THashSet<TClusterElement>(),
                      size_t totalDocumentsCount = 1)
    {
        Matchings = GetMatchings(sampleClusterization, importantElements);

        IntegralMetrics.Reset();
        IntegralMetricBounds.Reset();

        size_t recallNormalizer = 0;
        size_t precisionNormalizer = 0;
        size_t secondaryRecallNormalizer = 0;

        bool useImportant = !importantElements.empty();

        typename THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >::const_iterator it = ClusterElements.begin();
        for (; it != ClusterElements.end(); ++it) {
            const TClusterId& cluster = it->first;
            const TVector<std::pair<TClusterElement, float> >& elements = it->second;

            if ((DoRegularization && elements.size() < 2) ||
                (useImportant && !ImportantSizes[cluster]))
            {
                ClusterMetrics[cluster] = TClusterMetrics();
                ClusterMetricBounds[cluster] = TClusterMetrics();
                continue;
            }

            const TVector<TMatching<TClusterId> >& sortedMatchings = Matchings[cluster];

            double bcubedRecall = TClusterMetricsCalculator::BCubedRecall(sortedMatchings, DoRegularization, useImportant);

            double alexRecall = TClusterMetricsCalculator::AlexRecall(sortedMatchings, Factor, DoRegularization);
            double secondaryAlexRecall = TClusterMetricsCalculator::SecondaryAlexRecall(sortedMatchings, Factor, DoRegularization);
            double bCubedPrecision = TClusterMetricsCalculator::BCubedPrecision(sortedMatchings, Factor, DoRegularization, useImportant);

            double alexRecallBound = TClusterMetricsCalculator::AlexRecall(sortedMatchings, 1, DoRegularization);
            double secondaryAlexRecallBound = TClusterMetricsCalculator::SecondaryAlexRecall(sortedMatchings, 1, DoRegularization);
            double bCubedPrecisionBound = TClusterMetricsCalculator::BCubedPrecision(sortedMatchings, 1, DoRegularization, useImportant);

            ClusterMetrics[cluster] = TClusterMetrics(alexRecall,
                                                      secondaryAlexRecall,
                                                      bCubedPrecision,
                                                      bcubedRecall);
            ClusterMetricBounds[cluster] = TClusterMetrics(alexRecallBound,
                                                           secondaryAlexRecallBound,
                                                           bCubedPrecisionBound,
                                                           bcubedRecall);

            IntegralMetrics.AlexRecall += alexRecall;
            IntegralMetrics.BCubedPrecision += bCubedPrecision;
            IntegralMetrics.BCubedRecall += bcubedRecall;
            IntegralMetrics.BCubedF1 += TClusterMetricsCalculator::BCubedF1(bCubedPrecision, bcubedRecall);

            IntegralMetricBounds.AlexRecall += alexRecallBound;
            IntegralMetricBounds.BCubedPrecision += bCubedPrecisionBound;
            IntegralMetricBounds.BCubedRecall += bcubedRecall;
            IntegralMetricBounds.BCubedF1 += TClusterMetricsCalculator::BCubedF1(bCubedPrecisionBound, bcubedRecall);

            if (bcubedRecall && elements.size() >= (DoRegularization ? (ui32) 3 : (ui32) 2)) {
                IntegralMetrics.SecondaryAlexRecall += secondaryAlexRecall;
                IntegralMetricBounds.SecondaryAlexRecall += secondaryAlexRecallBound;
                ++secondaryRecallNormalizer;
            } else {
                ClusterMetrics[cluster].SecondaryAlexRecall = -1;
                ClusterMetricBounds[cluster].SecondaryAlexRecall = -1;
            }

            ++recallNormalizer;

            if (!useImportant) {
                bool hasCover = false;
                for (size_t i = 0; i < sortedMatchings.size(); ++i) {
                    if (sortedMatchings[i].GetSampleClusterSize() > 1 || !DoRegularization) {
                        hasCover = true;
                        break;
                    }
                }

                if (hasCover) {
                    ++precisionNormalizer;
                } else {
                    TClusterMetrics& metrics = ClusterMetrics[cluster];
                    metrics.BCubedPrecision = -1;
                    metrics.BCubedF1 = -1;

                    TClusterMetrics& metricBounds = ClusterMetricBounds[cluster];
                    metricBounds.BCubedPrecision = -1;
                    metricBounds.BCubedF1 = -1;
                };
                continue;
            }

            bool hasImportantCover = false;
            for (size_t i = 0; i < sortedMatchings.size(); ++i) {
                if (sortedMatchings[i].GetCommonImportantSize()) {
                    hasImportantCover = true;
                    break;
                }
            }

            if (hasImportantCover) {
                ++precisionNormalizer;
            } else {
                TClusterMetrics& metrics = ClusterMetrics[cluster];
                metrics.BCubedPrecision = -1;
                metrics.BCubedF1 = -1;

                TClusterMetrics& metricBounds = ClusterMetricBounds[cluster];
                metricBounds.BCubedPrecision = -1;
                metricBounds.BCubedF1 = -1;
            }
        }

        NormalizeValue(IntegralMetrics.AlexRecall, recallNormalizer);
        NormalizeValue(IntegralMetrics.BCubedPrecision, precisionNormalizer);
        NormalizeValue(IntegralMetrics.BCubedRecall, recallNormalizer);
        NormalizeValue(IntegralMetrics.BCubedF1, precisionNormalizer);
        NormalizeValue(IntegralMetrics.SecondaryAlexRecall, secondaryRecallNormalizer);

        IntegralMetrics.ClusteredShare = sampleClusterization.ClusteredDocumentsCount(DoRegularization ? 2 : 1);
        NormalizeValue(IntegralMetrics.ClusteredShare, totalDocumentsCount);

        NormalizeValue(IntegralMetricBounds.AlexRecall, recallNormalizer);
        NormalizeValue(IntegralMetricBounds.BCubedPrecision, precisionNormalizer);
        NormalizeValue(IntegralMetricBounds.BCubedRecall, recallNormalizer);
        NormalizeValue(IntegralMetricBounds.SecondaryAlexRecall, secondaryRecallNormalizer);
        NormalizeValue(IntegralMetricBounds.BCubedF1, precisionNormalizer);

        IntegralMetrics.IsFlat = IsFlat && sampleClusterization.IsFlat;
        IntegralMetricBounds.IsFlat = IsFlat && sampleClusterization.IsFlat;
    }

    void Read(const TString& clusteringFileName) {
        TFileInput clusterInput(clusteringFileName);

        TString dataStr;
        while (clusterInput.ReadLine(dataStr)) {
            TStringBuf dataStrBuf(dataStr);

            TStringBuf clusterIds = dataStrBuf.NextTok('\t');
            TString clusterId = ToString(clusterIds.NextTok(':'));

            TString elementId = ToString(dataStrBuf.NextTok('\t'));

            Add(elementId, clusterId, 1.f);
        }
    }

    void Print(IOutputStream& out) const {
        typename THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >::const_iterator it = ClusterElements.begin();
        for (; it != ClusterElements.end(); ++it) {
            const TClusterId& cluster = it->first;
            const TVector<std::pair<TClusterElement, float> >& elements = it->second;

            for (size_t i = 0; i < elements.size(); ++i) {
                out << cluster << "\t" << elements[i].first << "\t" << elements[i].second << "\n";
            }
        }
    }

    void PrintDetailedAlexRecallMetrics(IOutputStream& out, bool printSecondaryAR) const {
        typename THashMap<TClusterId, TClusterMetrics>::const_iterator it = ClusterMetrics.begin();
        for (; it != ClusterMetrics.end(); ++it) {
            const TClusterId& cluster = it->first;
            const TClusterMetrics& metrics = it->second;
            const TClusterMetrics* metricsBounds = ClusterMetricBounds.FindPtr(cluster);

            out << ToString<TClusterId>(cluster) << "\t"
                << SprintfMetric(metrics.AlexRecall) << " " << SprintfMetric(metricsBounds->AlexRecall);
            if (printSecondaryAR) {
                out << "\t" << SprintfMetric(metrics.SecondaryAlexRecall) << " " << SprintfMetric(metricsBounds->SecondaryAlexRecall);
            }
            out << "\n";
        }
    }

    void PrintDetailedBCubedMetrics(IOutputStream& out) const {
        typename THashMap<TClusterId, TClusterMetrics>::const_iterator it = ClusterMetrics.begin();
        for (; it != ClusterMetrics.end(); ++it) {
            const TClusterId& cluster = it->first;
            const TClusterMetrics& metrics = it->second;
            const TClusterMetrics* metricsBounds = ClusterMetricBounds.FindPtr(cluster);

            out << ToString<TClusterId>(cluster) << "\t"
                << SprintfMetric(metrics.BCubedPrecision) << " " << metricsBounds->BCubedPrecision << "\t"
                << SprintfMetric(metrics.BCubedRecall) << "\t"
                << SprintfMetric(metrics.BCubedF1) << " " << metricsBounds->BCubedF1 << "\n";
        }
    }

    void PrintMatchings(const TClusterizationBase& sampleClusterization, IOutputStream& out) const {
        typename THashMap<TClusterId, TVector<TMatching<TClusterId> > >::const_iterator it = Matchings.begin();
        for (; it != Matchings.end(); ++it) {
            const TClusterId& cluster = it->first;
            const TVector<TMatching<TClusterId> >& matchings = it->second;

            typename THashMap<TClusterId, TClusterMetrics>::const_iterator cmIt = ClusterMetrics.find(cluster);
            if (cmIt == ClusterMetrics.end()) {
                if (!matchings.empty()) {
                    Cerr << "warning: no metrics found for non-empty matching!\n";
                }
                continue;
            }
            const TClusterMetrics& metrics = cmIt->second;

            typename THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >::const_iterator ceIt = ClusterElements.find(cluster);
            if (ceIt == ClusterElements.end()) {
                Cerr << "warning: cluster without elements!\n";
                continue;
            }
            const TVector<std::pair<TClusterElement, float> >& myElements = ceIt->second;

            for (size_t matchingNumber = 0; matchingNumber < matchings.size(); ++matchingNumber) {
                const TMatching<TClusterId>& matching = matchings[matchingNumber];
                const TClusterId& sampleCluster = matching.GetSampleClusterId();

                TStringStream ss;
                ss << ToString<TClusterId>(cluster) << "\t"
                   << Sprintf("%.5lf\t", metrics.AlexRecall)
                   << Sprintf("%.5lf\t", metrics.SecondaryAlexRecall)
                   << Sprintf("%.5lf\t", metrics.BCubedPrecision)
                   << Sprintf("%.5lf\t", metrics.BCubedRecall)
                   << ToString<TClusterId>(sampleCluster) << "\t"
                   << Sprintf("%.5lf\t", matching.Precision(Factor, DoRegularization))
                   << Sprintf("%.5lf\t", matching.Recall(DoRegularization));

                for (size_t myElementNumber = 0; myElementNumber < myElements.size(); ++myElementNumber) {
                    const TClusterElement& element = myElements[myElementNumber].first;
                    TClusterId sampleClusterId;
                    if (sampleClusterization.Matches(matching.GetSampleClusterId(), element)) {
                        out << ss.Str() << ToString<TClusterElement>(element) << "\n";
                    }
                }
            }
        }
    }

    static std::pair<TClusterMetrics, TClusterMetrics> Compare(
        const TClusterizationBase<TClusterId, TClusterElement>& sampleClusterization,
        const TClusterizationBase<TClusterId, TClusterElement>& baselineClusterization,
        TClusterizationBase<TClusterId, TClusterElement>& markup,
        const THashSet<TClusterElement>& importantElements = THashSet<TClusterElement>(),
        size_t totalDocumentsCount = 1)
    {
        markup.SetupMetrics(baselineClusterization, importantElements, totalDocumentsCount);
        THashMap<TClusterId, TClusterMetrics> baseClusterMetrics = markup.ClusterMetrics;
        THashMap<TClusterId, TClusterMetrics> baseClusterMetricBounds = markup.ClusterMetricBounds;

        markup.SetupMetrics(sampleClusterization, importantElements, totalDocumentsCount);
        THashMap<TClusterId, TClusterMetrics> sampleClusterMetrics = markup.ClusterMetrics;
        THashMap<TClusterId, TClusterMetrics> sampleClusterMetricBounds = markup.ClusterMetricBounds;

        ui32 commonPrecisionNormalizer = 0;
        TClusterMetrics diffMetrics;
        TClusterMetrics diffMetricsBounds;

        const THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >& markupClusters = markup.ClusterElements;
        typename THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >::const_iterator it = markupClusters.begin();
        for (; it != markupClusters.end(); ++it) {
            const TClusterId& cluster = it->first;

            const TClusterMetrics& baseMetrics = baseClusterMetrics[cluster];
            const TClusterMetrics& baseMetricsBound = baseClusterMetricBounds[cluster];

            const TClusterMetrics& sampleMetrics = sampleClusterMetrics[cluster];
            const TClusterMetrics& sampleMetricsBound = sampleClusterMetricBounds[cluster];

            diffMetrics.AddDiff(sampleMetrics, baseMetrics);
            diffMetricsBounds.AddDiff(sampleMetricsBound, baseMetricsBound);

            commonPrecisionNormalizer += baseMetrics.BCubedPrecision >= 0 &&
                                         sampleMetrics.BCubedPrecision >= 0;
        }

        diffMetrics.Normalize(markupClusters.size(), commonPrecisionNormalizer, baselineClusterization.Size());
        diffMetricsBounds.Normalize(markupClusters.size(), commonPrecisionNormalizer, baselineClusterization.Size());

        return std::make_pair(diffMetrics, diffMetricsBounds);
    }
private:
    struct TCommonSizeInfo {
        ui32 CommonSize;
        ui32 CommonImportantSize;

        TCommonSizeInfo(ui32 commonSize = 0, ui32 CommonImportantSize = 0)
            : CommonSize(commonSize)
            , CommonImportantSize(CommonImportantSize)
        {}
    };

    THashMap<TClusterId, TVector<TMatching<TClusterId> > > GetMatchings(const TClusterizationBase& sampleClusterization,
                                                                     const THashSet<TClusterElement>& importantElements)
    {
        THashMap<TClusterId, ui32>().swap(ImportantSizes);

        typename THashSet<TClusterElement>::const_iterator ieIt = importantElements.begin();
        for (; ieIt != importantElements.end(); ++ieIt) {
            if (const TVector<TClusterId>* clusters = GetClusters(*ieIt)) {
                for (const TClusterId* clusterId = clusters->begin(); clusterId != clusters->end(); ++clusterId) {
                    ++ImportantSizes[*clusterId];
                }
            }
        }

        THashMap<TClusterId, ui32> sampleMarkedSizes;
        THashMap<TClusterId, THashMap<TClusterId, TCommonSizeInfo> > commonSizes;
        typename THashMap<TClusterElement, TVector<TClusterId> >::const_iterator sIt = sampleClusterization.ElementCluster.begin();
        for (; sIt != sampleClusterization.ElementCluster.end(); ++sIt) {
            for (const TClusterId* sampleClusterId = sIt->second.begin(); sampleClusterId != sIt->second.end(); ++sampleClusterId) {
                const TClusterElement& element = sIt->first;
                if (const TVector<TClusterId>* clusters = GetClusters(element)) {
                    ++sampleMarkedSizes[*sampleClusterId];
                    for (const TClusterId* myClusterId = clusters->begin(); myClusterId != clusters->end(); ++myClusterId) {
                        TCommonSizeInfo& commonSizeInfo = commonSizes[*myClusterId][*sampleClusterId];
                        ++commonSizeInfo.CommonSize;
                        if (importantElements.contains(element)) {
                            ++commonSizeInfo.CommonImportantSize;
                        }
                    }
                }
            }
        }

        THashMap<TClusterId, TVector<TMatching<TClusterId> > > result;
        typename THashMap<TClusterId, TVector<std::pair<TClusterElement, float> > >::const_iterator ceIt = ClusterElements.begin();
        for (; ceIt != ClusterElements.end(); ++ceIt) {
            const TClusterId& myCluster = ceIt->first;
            size_t myClusterSize = GetClusterSize(myCluster);
            size_t myImportantSize = ImportantSizes[myCluster];

            typename THashMap<TClusterId, THashMap<TClusterId, TCommonSizeInfo> >::const_iterator csIt = commonSizes.find(myCluster);
            if (csIt == commonSizes.end()) {
                result[myCluster] = TVector<TMatching<TClusterId> >();
                continue;
            }

            const THashMap<TClusterId, TCommonSizeInfo> currentCommonSizes = csIt->second;

            TVector<TMatching<TClusterId> >& matchings = result[myCluster];
            matchings.reserve(currentCommonSizes.size());

            typename THashMap<TClusterId, TCommonSizeInfo>::const_iterator ccsIt = currentCommonSizes.begin();
            for (; ccsIt != currentCommonSizes.end(); ++ccsIt) {
                const TClusterId& sampleClusterId = ccsIt->first;
                const TCommonSizeInfo& commonSizes = ccsIt->second;
                ui32 sampleClusterSize = sampleClusterization.GetClusterSize(sampleClusterId);
                ui32 markedSize = sampleMarkedSizes[sampleClusterId];

                matchings.push_back(TMatching<TClusterId>(sampleClusterId,
                                                          sampleClusterSize,
                                                          myClusterSize,
                                                          myImportantSize,
                                                          commonSizes.CommonSize,
                                                          commonSizes.CommonImportantSize,
                                                          markedSize));
            }

            Sort(matchings.begin(), matchings.end(), TGreater<TMatching<TClusterId> >());
        }
        return result;
    }

    void NormalizeValue(double& value, double normalizer) const {
        value = normalizer ? value / normalizer : 0;
    }
};

typedef TClusterizationBase<TString, TString> TClusterization;

}
