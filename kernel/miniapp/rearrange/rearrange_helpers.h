#pragma once

#include <kernel/search_daemon_iface/relevance_type.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/utility.h>
#include <util/string/vector.h>

namespace NMiniapp {

template <class DocType>
class TDocumentContainer {
public:
    TDocumentContainer() {}

    TDocumentContainer(
        DocType * doc,
        const TString& url,
        const TString& host,
        double proximaPredict,
        TRelevance relevance = 0,
        size_t group = 0)
        : Doc(doc), Url(url), Host(host), ProximaPredict(proximaPredict),
          Relevance(relevance), Group(group)
    {
    }

    TDocumentContainer(
        DocType * doc,
        const TString& url,
        const TString& host,
        double proximaPredict,
        TRelevance relevance,
        size_t group,
        const TString& hostToBoost,
        bool isPartnerUrl,
        bool isBoosted,
        bool fixedPosition)
        : Doc(doc), Url(url), Host(host), ProximaPredict(proximaPredict), Relevance(relevance),
          Group(group), HostToBoost(hostToBoost), PartnerUrl(isPartnerUrl),
          Boosted(isBoosted), FixedPosition(fixedPosition)
    {
    }

    TRelevance GetRelevance() const {
        return Relevance;
    }

    void SetRelevance(TRelevance value) {
        Relevance = value;
    }

    TRelevance GetBoostedRelevance(double alpha) const {
        return IsPartnerUrl() ? Relevance * (1.0 + alpha) : Relevance;
    }

    double GetProximaPredict() const {
        return ProximaPredict;
    }

    void SetProximaPredict(double value) {
        ProximaPredict = value;
    }

    size_t GetGroup() const {
        return Group;
    }

    void SetGroup(size_t value) {
        Group = value;
    }

    bool IsBoosted() const {
        return Boosted;
    }

    bool HasFixedPosition() const {
        return FixedPosition;
    }

    void SetFixedPosition(bool value = true) {
        FixedPosition = value;
    }

    void SetBoost(bool value = true) {
        Boosted = value;
    }

    bool IsPartnerUrl() const {
        return PartnerUrl;
    }

    void SetIsPartnerUrl(bool value) {
        PartnerUrl = value;
    }

    TString GetHost() const {
        return Host;
    }

    TString GetHostToBoost() const {
        return HostToBoost;
    }

    TString GetUrl() const {
        return Url;
    }

    void SetHost(TStringBuf value) {
        Host = value;
    }

    void SetHostToBoost(TStringBuf value) {
        HostToBoost = value;
    }
    void SetUrl(TStringBuf value) {
        Url = value;
    }

    void SetDoc(DocType* value) {
        Doc = value;
    }

    DocType* GetDoc() const {
        return Doc;
    }

private:
    DocType* Doc = nullptr;
    TString Url;
    TString Host;
    double ProximaPredict = 0.;
    TRelevance Relevance = 0;
    size_t Group = 0;
    TString HostToBoost;
    bool PartnerUrl = false;
    bool Boosted = false;
    bool FixedPosition = false;
};

enum class ERearrangeMode {
     CopyOnTop = 0,
     MoveOnTop = 1
};

TVector<TString> GetHostParts(TStringBuf url) {
    TVector<TString> result;
    TVector<TString> hostParts = SplitString(ToString(GetHost(url)), ".");

    TString accumulator;
    for (auto iter = hostParts.rbegin(); iter != hostParts.rend(); ++iter) {
        TString hostPart = *iter;
        if (accumulator.empty()) {
            accumulator = hostPart;
        } else {
            accumulator = TString::Join(hostPart,".", accumulator);
        }
        result.push_back(accumulator);
    }
    Reverse(result.begin(), result.end());
    return result;
}

template <class DocType>
double CalcProximaPredictMetrics(
    const TVector<TDocumentContainer<DocType>>& docsSorted,
    size_t topSize = 5,
    bool ignoreFine = false)
{
    Y_ENSURE(!docsSorted.empty());
    double result = 0.;
    size_t urlCounter = 0;
    THashMap<TString, size_t> knownHosts;
    THashMap<TString, size_t> knownUrls;
    THashSet<size_t> knownGroups;
    for (size_t i = 0; i < docsSorted.size() && urlCounter < topSize; ++i) {
        const auto& doc = docsSorted[i];
        if (knownGroups.contains(doc.GetGroup())) {
            continue;
        }

        double antidupPenalty = pow(0.1, knownUrls[doc.GetUrl()]);
        double ungroupPenalty = pow(0.7, knownHosts[doc.GetHost()]);
        knownUrls[doc.GetUrl()] += 1;
        knownHosts[doc.GetHost()] += 1;
        knownGroups.insert(doc.GetGroup());
        ++urlCounter;

        if (!ignoreFine) {
            result += doc.GetProximaPredict() * antidupPenalty * ungroupPenalty / urlCounter;
        } else {
            result += doc.GetProximaPredict() / urlCounter;
        }
    }
    return result;
}

template <class DocType>
TMaybe<size_t> FindDocIndexToBoost(const TVector<TDocumentContainer<DocType>>& prs, const THashSet<TString>& bannedHosts = {}) {
    for (size_t index = 0; index < prs.size(); ++index) {
        const auto& doc = prs[index];
        if (doc.IsPartnerUrl() && !doc.IsBoosted() && !bannedHosts.contains(doc.GetHost())) {
            return index;
        }
    }
    return {};
}

template <class DocType>
TVector<TDocumentContainer<DocType>> GetDocsToBoostWithThreshold(
    const TVector<TDocumentContainer<DocType>>& docsSorted,
    double threshold = 0.,
    ERearrangeMode mode = ERearrangeMode::MoveOnTop,
    size_t dcgValue = 5,
    const THashSet<TString>& bannedHosts = {})
{
    TVector<TDocumentContainer<DocType>> result;
    TVector<TDocumentContainer<DocType>> boostedPrs = docsSorted;
    double baselineProximaPredict = CalcProximaPredictMetrics(boostedPrs, dcgValue);
    double newProximaPredict = baselineProximaPredict;
    TMaybe<size_t> nextIndexToBoost = FindDocIndexToBoost(boostedPrs);
    size_t targetIndex = 0;
    while (!nextIndexToBoost.Empty() &&
           targetIndex < dcgValue &&
           ((baselineProximaPredict - newProximaPredict) / baselineProximaPredict * 100 < threshold))
    {
        size_t index = nextIndexToBoost.GetRef();
        boostedPrs[index].SetBoost();
        boostedPrs.insert(boostedPrs.begin() + targetIndex, boostedPrs[index]);
        boostedPrs[targetIndex].SetFixedPosition();
        if (mode == ERearrangeMode::MoveOnTop) {
            boostedPrs.erase(boostedPrs.begin() + index + 1);
        }
        newProximaPredict = CalcProximaPredictMetrics(boostedPrs, dcgValue);
        if ((baselineProximaPredict - newProximaPredict) / baselineProximaPredict * 100 < threshold) {
            result.push_back(boostedPrs[targetIndex]);
        }
        ++targetIndex;
        nextIndexToBoost = FindDocIndexToBoost(boostedPrs, bannedHosts);
    }
    return result;
}

template <class DocType>
TVector<TDocumentContainer<DocType>> RearrangeDocumentsVer3(
    const TVector<TDocumentContainer<DocType>>& docsSorted,
    double threshold = 0.,
    ERearrangeMode mode = ERearrangeMode::MoveOnTop,
    size_t dcgValue = 5,
    const THashSet<TString>& bannedHosts = {},
    size_t maxPartnersFromHost = 100)
{
    if (docsSorted.empty()) {
        return {};
    }
    TVector<TDocumentContainer<DocType>> docs = docsSorted;
    THashMap<TString, size_t> numHosts;
    for (auto& elem : docs) {
        ++numHosts[elem.GetHost()];
        if (elem.IsPartnerUrl() && numHosts[elem.GetHost()] > maxPartnersFromHost) {
            elem.SetIsPartnerUrl(false);
        }
    }
    TVector<TDocumentContainer<DocType>> docsToBoost = GetDocsToBoostWithThreshold(
        docs, threshold, mode, dcgValue, bannedHosts);
    TVector<TDocumentContainer<DocType>> result = docs;
    TRelevance currentBoostRelevance = (Max<TRelevance>(0, docs.front().GetRelevance()) + 100) * 8;

    THashSet<TString> docUrls;
    for (auto& elem : docsToBoost) {
        docUrls.insert(elem.GetUrl());
    }

    for (auto& elem : result) {
        if (docUrls.contains(elem.GetUrl())) {
            elem.SetBoost();
        }
    }

    for (auto& elem : docsToBoost) {
        elem.SetRelevance(currentBoostRelevance);
        elem.SetBoost();
        elem.SetFixedPosition();
        currentBoostRelevance *= 0.9;
        result.push_back(elem);
    }
    if (mode == ERearrangeMode::MoveOnTop) {
        EraseIf(result, [] (const TDocumentContainer<DocType>& elem) {
            return elem.IsBoosted() && !elem.HasFixedPosition();
        });
    }
    Sort(result.rbegin(), result.rend(), [] (const TDocumentContainer<DocType>& a, const TDocumentContainer<DocType>& b) {
        return a.GetRelevance() < b.GetRelevance();
    });
    return result;
}

template <class DocType>
TVector<TDocumentContainer<DocType>> GetDocsToBoostWithAlpha(
    const TVector<TDocumentContainer<DocType>>& docsSorted,
    double alpha,
    size_t maxPartnersFromHost = 100)
{
    TVector<TDocumentContainer<DocType>> result = docsSorted;
    THashMap<TString, size_t> numHosts;
    for (auto& elem : result) {
        ++numHosts[elem.GetHost()];
        if (elem.IsPartnerUrl() && numHosts[elem.GetHost()] <= maxPartnersFromHost) {
            elem.SetBoost();
            elem.SetRelevance(elem.GetBoostedRelevance(alpha));
        }
    }
    Sort(result.rbegin(), result.rend(), [&] (const TDocumentContainer<DocType>& a, const TDocumentContainer<DocType>& b) {
        return a.GetRelevance() < b.GetRelevance();
    });
    return result;
}

template <class DocType>
TVector<TDocumentContainer<DocType>> RearrangeDocumentsVer2(
    const TVector<TDocumentContainer<DocType>>& docsSorted,
    double alpha,
    size_t maxPartnersFromHost = 100)
{
    return GetDocsToBoostWithAlpha(docsSorted, alpha, maxPartnersFromHost);
}

template <class ParamType>
TString GetMainHostToBoost(const TVector<TString>& hostParts, const ParamType& params) {
    for (const TString& part : hostParts) {
        if (params.contains(part)) {
            return part;
        }
    }
    return "";
}

template <class DocType, class ParamType>
TVector<TDocumentContainer<DocType>> RearrangeDocumentsVer1(
    const TVector<TDocumentContainer<DocType>>& docsSorted,
    double defaultBoostValue,
    ParamType params,
    size_t maxPartnersFromHost = 100)
{
    TVector<TDocumentContainer<DocType>> result = docsSorted;
    THashMap<TString, size_t> numHosts;
    for (auto& elem : result) {
        const TVector<TString> hosts = GetHostParts(elem.GetHost());
        const TString host = GetMainHostToBoost(hosts, params);
        if (host.empty()) {
            continue;
        }
        ++numHosts[host];
        if (elem.IsPartnerUrl() && numHosts[host] > maxPartnersFromHost) {
            continue;
        }
        double boostValue = defaultBoostValue;
        if (params.contains(host)) {
            boostValue = params[host];
        }
        elem.SetRelevance(elem.GetRelevance() * (1.0 + boostValue));
        elem.SetBoost();
    }
    Sort(result.rbegin(), result.rend(), [&] (const TDocumentContainer<DocType>& a, const TDocumentContainer<DocType>& b) {
        return a.GetRelevance() < b.GetRelevance();
    });
    return result;
}

}
