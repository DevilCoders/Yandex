#include <kernel/miniapp/rearrange/rearrange_helpers.h>
#include <library/cpp/getopt/opt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/svnversion/svnversion.h>
#include <mapreduce/yt/interface/client.h>
#include <util/charset/wide.h>
#include <util/generic/hash_set.h>
#include <util/generic/size_literals.h>

using namespace NYT;
namespace NMiniapp {

template <class ParamType>
void AddParsedPrsToSerp(
    const TNode& row,
    ParamType& Partners,
    TVector<TDocumentContainer<TNode>>* serp)
{
    bool isPartnerUrl = false;
    TString url;
    TString host;
    TString hostToBoost;
    if (row["url"].HasValue()) {
        TUtf16String urlUtf16 = UTF8ToWide(row["url"].AsString());
        urlUtf16.to_lower();
        url = WideToUTF8(urlUtf16);
        hostToBoost = GetMainHostToBoost(GetHostParts(url), Partners);
        isPartnerUrl = !hostToBoost.empty();
    }
    if (!hostToBoost.empty()) {
        host = hostToBoost;
    } else if (row["owner"].HasValue() && !row["owner"].AsString().empty()) {
        host = row["owner"].AsString();
    } else if (row["host"].HasValue()) {
        host = CutWWWPrefix(GetHost(url));
    }
    serp->emplace_back(TDocumentContainer<TNode>(
        new TNode(row),
        url,
        host,
        row["proxima_predict"].AsDouble(),
        row["relevance"].AsInt64(),
        FromString(row["group"].AsString()),
        hostToBoost,
        isPartnerUrl,
        false,
        false));
}

double TrafficDcg(const TDocumentContainer<TNode>&, size_t index) {
    TVector<double> coeffs = {
        27.7362,
        14.7715,
        9.8729,
        7.2218,
        5.4889,
        4.3412,
        3.4909,
        2.9066,
        2.5076,
        2.2812
    };
    return coeffs[index];
}

double NumShows(const TDocumentContainer<TNode>&, size_t) {
    return 1.;
}

double ClickDcg(const TDocumentContainer<TNode>& doc, size_t index) {
    return  100. * (*doc.GetDoc())["clicks_predict"].AsDouble() / (index + 1);
}

template <class FuncType>
double GetDcgValue(
    const TVector<TDocumentContainer<TNode>>& docsSorted,
    FuncType func,
    size_t topSize = 10,
    bool ignoreFine = true,
    bool onlyPartner = true)
{
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
        if (onlyPartner && doc.IsPartnerUrl()) {
            if (!ignoreFine) {
                result += func(doc, urlCounter) * antidupPenalty * ungroupPenalty;
            } else {
                result += func(doc, urlCounter);
            }
        } else if (!onlyPartner && !doc.GetHost().empty()) {
            if (!ignoreFine) {
                result += func(doc, urlCounter) * antidupPenalty * ungroupPenalty ;
            } else {
                result += func(doc, urlCounter);
            }
        }
        ++urlCounter;
    }
    return result;
}

template <class FuncType>
THashMap<TString, TNode> GetDcgValueByPartner(
    const TVector<TDocumentContainer<TNode>>& docsSorted,
    FuncType func,
    size_t topSize = 10,
    bool ignoreFine = true,
    bool onlyPartner = true)
{
    THashMap<TString, TNode> result;
    size_t urlCounter = 0;
    THashSet<size_t> knownGroups;
    THashMap<TString, size_t> knownHosts;
    THashMap<TString, size_t> knownUrls;
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
        if (onlyPartner && doc.IsPartnerUrl()) {
            TString host = doc.GetHostToBoost();
            if (!result.contains(host)) {
                result[host] = static_cast<double>(0);
            }
            double value = func(doc, urlCounter);
            if (!ignoreFine) {
                value *= antidupPenalty * ungroupPenalty;
            }
            result[host] = result[host].AsDouble() + value;
        } else if (!onlyPartner && !doc.GetHost().empty()) {
            TString host = doc.GetHost();
            if (!result.contains(host)) {
                result[host] = static_cast<double>(0);
            }
            double value = func(doc, urlCounter);
            if (!ignoreFine) {
                value *= antidupPenalty * ungroupPenalty;
            }
            result[host] = result[host].AsDouble() + value;
        }
        ++urlCounter;
    }
    return result;
}

TVector<TDocumentContainer<TNode>> GetSerp(const TVector<TDocumentContainer<TNode>>& docsSorted) {
    TVector<TDocumentContainer<TNode>> result;
    size_t urlCounter = 0;
    THashSet<size_t> knownGroups;
    for (size_t i = 0; i < docsSorted.size() && urlCounter < 10; ++i) {
        const auto& doc = docsSorted[i];
        if (knownGroups.contains(doc.GetGroup())) {
            continue;
        }
        knownGroups.insert(doc.GetGroup());
        result.push_back(doc);
        ++urlCounter;
    }
    return result;
}


THashMap<TString, TNode> FillPartnerMap(
    const THashMap<TString, TNode>& mp,
    double value)
{
    THashMap<TString, TNode> result;
    for (const auto& elem : mp) {
        result[elem.first] = value;
    }
    return result;
}

TNode GetProximaPredictStatRow(
    const TVector<TDocumentContainer<TNode>>& initialSerp,
    const TVector<TDocumentContainer<TNode>>& finalSerp,
    TNode extraData = TNode())
{
    TNode proxima = extraData;
    proxima["proxima-predict-dcg-5-original"] = CalcProximaPredictMetrics(initialSerp);
    proxima["proxima-predict-dcg-5-no-fine-original"] = CalcProximaPredictMetrics(initialSerp, 5, true);
    proxima["traffic-original"] = GetDcgValue(initialSerp, TrafficDcg);
    proxima["traffic-rearranged"] = GetDcgValue(finalSerp, TrafficDcg);
    proxima["traffic-by-partner-original"] = GetDcgValueByPartner(initialSerp, TrafficDcg);
    proxima["traffic-by-partner-rearranged"] = GetDcgValueByPartner(finalSerp, TrafficDcg);
    proxima["traffic-for-host-original"] = GetDcgValueByPartner(initialSerp, TrafficDcg, 10, true, false);
    proxima["traffic-for-host-rearranged"] = GetDcgValueByPartner(finalSerp, TrafficDcg, 10, true, false);
    auto originalShows = GetDcgValueByPartner(initialSerp, NumShows);
    auto rearrangedShows =  GetDcgValueByPartner(finalSerp, NumShows);
    proxima["num-shows-by-partner-original"] = originalShows;
    proxima["num-shows-by-partner-rearranged"] = rearrangedShows;
    proxima["num-serps-shows-by-partner-original"] = FillPartnerMap(originalShows, 1);
    proxima["num-serps-shows-by-partner-rearranged"] = FillPartnerMap(rearrangedShows, 1);

    if ((*finalSerp.front().GetDoc()).AsMap().contains("clicks_predict")) {
        proxima["clicks-dcg-10-no-fine-original"] = GetDcgValue(initialSerp, ClickDcg);
        proxima["clicks-dcg-10-no-fine-rearranged"] = GetDcgValue(finalSerp, ClickDcg);
        proxima["clicks-dcg-10-by-partner-no-fine-original"] = GetDcgValueByPartner(initialSerp, ClickDcg);
        proxima["clicks-dcg-10-by-partner-no-fine-rearranged"] = GetDcgValueByPartner(finalSerp, ClickDcg);
        proxima["clicks-dcg-10-original"] = GetDcgValue(initialSerp, ClickDcg, 10, false);
        proxima["clicks-dcg-10-rearranged"] = GetDcgValue(finalSerp, ClickDcg, 10, false);
        proxima["clicks-dcg-10-by-partner-original"] = GetDcgValueByPartner(initialSerp, ClickDcg, 10, false);
        proxima["clicks-dcg-10-by-partner-rearranged"] = GetDcgValueByPartner(finalSerp, ClickDcg, 10, false);

    }

    proxima["key"] = (*finalSerp.front().GetDoc())["key"];
    proxima["proxima-predict-dcg-5-rearranged"] = CalcProximaPredictMetrics(finalSerp);
    proxima["proxima-predict-dcg-5-no-fine-rearranged"] = CalcProximaPredictMetrics(finalSerp, 5, true);
    return proxima;
}

TNode ConvertDocumentContainer(TDocumentContainer<TNode>* elem) {
    TNode result = *elem->GetDoc();
    result["new_relevance"] = TNode(elem->GetRelevance());
    result["new_relevance_reverse"] = TNode(-static_cast<i64>(elem->GetRelevance()));
    result["is_partner_url"] = TNode(elem->IsPartnerUrl());
    result["group"] = ToString(elem->GetGroup());
    return result;
}

class TRearrangeReducerVer3 :
    public IReducer<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    TRearrangeReducerVer3() {}

    TRearrangeReducerVer3(
        const TVector<double>& thresholds,
        const TVector<TString>& partners,
        bool copyOnTop,
        bool addPartnerStats,
        bool noStoveMode,
        const TVector<TString>& commercialFactors,
        const TVector<double>& commercialThresholds,
        size_t maxPartnersFromHost = 100)
            : Thresholds(thresholds), Partners(partners.begin(), partners.end()),
              AddPartnerStats(addPartnerStats), NoStoveMode(noStoveMode), MaxPartnersFromHost(maxPartnersFromHost)
        {
            if (copyOnTop) {
                Mode = ERearrangeMode::CopyOnTop;
            }
            for (size_t i = 0; i < Min(commercialFactors.size(), commercialThresholds.size()); ++i) {
                CommercialThresholds[commercialFactors[i]] = commercialThresholds[i];
            }
        }

    Y_SAVELOAD_JOB(Thresholds, Partners, Mode, AddPartnerStats, NoStoveMode, CommercialThresholds, MaxPartnersFromHost);
    void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
        TVector<TDocumentContainer<TNode>> serp;
        bool skipNotCommercial = false;
        THashMap<TString, TNode> consideredFactors;
        for (;input->IsValid(); input->Next()) {
            TNode row = input->GetRow();
            AddParsedPrsToSerp(row, Partners, &serp);
            if (CommercialThresholds.empty()) {
                if (row.AsMap().contains("is_commercial_query") && !row["is_commercial_query"].AsBool()) {
                    skipNotCommercial = true;
                }
            } else {
                skipNotCommercial = true;
                for (const auto& elem : CommercialThresholds) {
                    if (row.AsMap().contains(elem.first) && row[elem.first].AsDouble() >= elem.second) {
                        skipNotCommercial = false;
                        consideredFactors["desicion-based-on"] = elem.first;
                    }
                    if (row.AsMap().contains(elem.first)) {
                        consideredFactors[elem.first] = row[elem.first];
                    }
                }
            }
        }
        if (serp.empty()) {
            return;
        }

        Sort(serp.rbegin(), serp.rend(),
            [] (const TDocumentContainer<TNode>& a, const TDocumentContainer<TNode>& b)
            { return a.GetRelevance() < b.GetRelevance(); });

        for (size_t i = 0; i < Thresholds.size(); ++i) {
            TVector<TDocumentContainer<TNode>> rearrangedSerp = RearrangeSerp(serp, Thresholds[i], 5, skipNotCommercial, {});
            if (!NoStoveMode) {
                for (auto& elem : GetSerp(rearrangedSerp)) {
                    output->AddRow(ConvertDocumentContainer(&elem), i * 2);
                }
            }
            if (AddPartnerStats) {
                TNode partnerStat = EvalPartners(serp, rearrangedSerp, skipNotCommercial, Thresholds[i]);
                partnerStat["is_commercial_query"] = !skipNotCommercial;
                partnerStat["commercial_query_factors"] = consideredFactors;
                output->AddRow(GetProximaPredictStatRow(serp, rearrangedSerp, partnerStat), i * 2 + 1);
            } else {
                TNode tmp;
                tmp["is_commercial_query"] = !skipNotCommercial;
                tmp["commercial_query_factors"] = consideredFactors;
                output->AddRow(GetProximaPredictStatRow(serp, rearrangedSerp, tmp), i * 2 + 1);
            }
        }
    }

private:
    TNode EvalPartners(
        const TVector<TDocumentContainer<TNode>>& serp,
        const TVector<TDocumentContainer<TNode>>& rearrangedSerp,
        bool skipNotCommercial,
        double threshold)
    {
        TNode partnerStat;
        THashSet<TString> hostsToCheck;
        for (const auto& elem : serp) {
            if (elem.IsPartnerUrl() && !elem.GetHostToBoost().empty()) {
                hostsToCheck.insert(elem.GetHostToBoost());
            }
        }
        for (const auto& elem : hostsToCheck) {
            THashSet<TString> bannedHosts;
            bannedHosts.insert(elem);
            TVector<TDocumentContainer<TNode>> statSerp = RearrangeSerp(
                serp, threshold, 5, skipNotCommercial, bannedHosts);
            double newTraffic = GetDcgValue(statSerp, TrafficDcg);
            double initialTraffic = GetDcgValue(serp, TrafficDcg);
            double rearrangedTraffic = GetDcgValue(rearrangedSerp, TrafficDcg);
            partnerStat["eval-exclude-partner-traffic-sum-original"][elem] = initialTraffic;
            partnerStat["eval-exclude-partner-traffic-sum-rearranged"][elem] = rearrangedTraffic;
            partnerStat["eval-exclude-partner-traffic-sum-excluded"][elem] = newTraffic;

            double newProxima = CalcProximaPredictMetrics(statSerp);
            double initialProxima = CalcProximaPredictMetrics(serp);
            double rearrangedProxima = CalcProximaPredictMetrics(rearrangedSerp);
            partnerStat["eval-exclude-partner-proxima-dcg-5-original"][elem] = initialProxima;
            partnerStat["eval-exclude-partner-proxima-dcg-5-excluded"][elem] = newProxima;
            partnerStat["eval-exclude-partner-proxima-dcg-5-rearranged"][elem] = rearrangedProxima;

            double newProximaNoFine = CalcProximaPredictMetrics(statSerp,  5, true);
            double initialProximaNoFine = CalcProximaPredictMetrics(serp,  5, true);
            double rearrangedProximaNoFine = CalcProximaPredictMetrics(rearrangedSerp,  5, true);
            partnerStat["eval-exclude-partner-proxima-dcg-5-no-fine-original"][elem] = initialProximaNoFine;
            partnerStat["eval-exclude-partner-proxima-dcg-5-no-fine-excluded"][elem] = newProximaNoFine;
            partnerStat["eval-exclude-partner-proxima-dcg-5-no-fine-rearranged"][elem] = rearrangedProximaNoFine;

            if ((*statSerp.front().GetDoc()).AsMap().contains("clicks_predict")) {
                double initialClicksTraffic = GetDcgValue(serp, ClickDcg);
                double newClicksTraffic = GetDcgValue(statSerp, ClickDcg);
                double rearrangedClicksTraffic = GetDcgValue(rearrangedSerp, ClickDcg);
                partnerStat["eval-exclude-partner-clicks-traffic-original"] = initialClicksTraffic;
                partnerStat["eval-exclude-partner-clicks-traffic-excluded"] = newClicksTraffic;
                partnerStat["eval-exclude-partner-clicks-traffic-rearranged"] = rearrangedClicksTraffic;
            }
        }
        THashMap<TString, TNode> tmp;
        partnerStat["num-queries-with-partner-prs"] = tmp;
        if (partnerStat.AsMap().contains("eval-exclude-partner-traffic-sum-original")) {
            tmp = FillPartnerMap(partnerStat["eval-exclude-partner-traffic-sum-original"].AsMap(), 1.);
            partnerStat["num-queries-with-partner-prs"] = FillPartnerMap(tmp, 1.);
        }
        return partnerStat;
    }

     TVector<TDocumentContainer<TNode>> RearrangeSerp(
        const TVector<TDocumentContainer<TNode>>& serp,
        double threshold = 0.,
        size_t dcgValue = 5,
        bool skipNotCommercial = false,
        const THashSet<TString>& bannedHosts = {})
    {
        size_t updateGrouping = 0;
        TVector<TDocumentContainer<TNode>> rearrangedSerp;
        if (skipNotCommercial) {
            rearrangedSerp = serp;
        } else {
            rearrangedSerp = RearrangeDocumentsVer3(serp, threshold, Mode, dcgValue, bannedHosts, MaxPartnersFromHost);
        }
        for (const auto& elem : rearrangedSerp) {
            if (Mode == ERearrangeMode::CopyOnTop) {
                updateGrouping = Max(updateGrouping, FromString<size_t>((*elem.GetDoc())["group"].AsString()));
            }
        }
        updateGrouping += 1;
        size_t* ptr = nullptr;
        if (Mode == ERearrangeMode::CopyOnTop) {
            ptr = &updateGrouping;
        }
        for (auto& elem : rearrangedSerp) {
            if (elem.IsBoosted() && ptr != nullptr) {
                elem.SetGroup(*ptr);
                *ptr += 1;
            }
        }
        return rearrangedSerp;
    }

private:
    TVector<double> Thresholds;
    THashSet<TString> Partners;
    THashMap<TString, double> CommercialThresholds;
    NMiniapp::ERearrangeMode Mode = ERearrangeMode::MoveOnTop;
    bool AddPartnerStats = false;
    bool NoStoveMode = false;
    size_t MaxPartnersFromHost = 100;
};

class TRearrangeReducerVer2 :
    public IReducer<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    TRearrangeReducerVer2() {}

    TRearrangeReducerVer2(
        TVector<double> alpha,
        const TVector<TString>& partners,
        bool noStoveMode,
        size_t maxPartnersFromHost)
            : Alpha(alpha), Partners(partners.begin(), partners.end()),
              NoStoveMode(noStoveMode), MaxPartnersFromHost(maxPartnersFromHost) {}

    Y_SAVELOAD_JOB(Alpha, Partners, NoStoveMode, MaxPartnersFromHost);
    void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
        TVector<TDocumentContainer<TNode>> serp;
        bool skipNotCommercial = false;
        for (;input->IsValid(); input->Next()) {
            TNode row = input->GetRow();
            AddParsedPrsToSerp(row, Partners, &serp);
            if (row.AsMap().contains("is_commercial_query") && !row["is_commercial_query"].AsBool()) {
                skipNotCommercial = true;
            }
        }
        if (serp.empty()) {
            return;
        }

        Sort(serp.rbegin(), serp.rend(),
            [] (const TDocumentContainer<TNode>& a, const TDocumentContainer<TNode>& b)
            { return a.GetRelevance() < b.GetRelevance(); });
        for (size_t i = 0; i < Alpha.size(); ++i) {
            TVector<TDocumentContainer<TNode>> rearrangedSerp;
            if (skipNotCommercial) {
                rearrangedSerp = serp;
            } else {
                rearrangedSerp = RearrangeDocumentsVer2(serp, Alpha[i], MaxPartnersFromHost);
            }
            for (auto& elem : GetSerp(rearrangedSerp)) {
                output->AddRow(ConvertDocumentContainer(&elem), i * 2);
            }
            output->AddRow(GetProximaPredictStatRow(serp, rearrangedSerp), i * 2 + 1);
        }
    }
private:
    TVector<double> Alpha;
    THashSet<TString> Partners;
    bool NoStoveMode = false;
    size_t MaxPartnersFromHost = 100;
};

class TPartnersExtractor :
    public IMapper<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    TPartnersExtractor() = default;
    TPartnersExtractor(const TVector<TString>& partners)
        : Partners(partners.begin(), partners.end()) {}
    Y_SAVELOAD_JOB(Partners);

    void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
        for (;input->IsValid(); input->Next()) {
            TNode row = input->GetRow();
            TUtf16String urlUtf16 = UTF8ToWide(row["url"].AsString());
            urlUtf16.to_lower();
            row["is_partner_url"]  = !GetMainHostToBoost(
                GetHostParts(WideToUTF8(urlUtf16)), Partners).empty();
             row["partner_host"] = GetMainHostToBoost(GetHostParts(WideToUTF8(urlUtf16)), Partners);
            output->AddRow(row);
        }
    }
private:
    THashSet<TString> Partners;
};

class TPartnersTrafficExtractor :
    public IMapper<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    TPartnersTrafficExtractor() = default;
    TPartnersTrafficExtractor(const TVector<TString>& columnNames)
        : ColumnNames(columnNames) {}
    Y_SAVELOAD_JOB(ColumnNames)

    void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
        THashMap<TString, THashMap<TString, double>> partnersTraffic;
        size_t rowCounter = 0;
        for (;input->IsValid(); input->Next()) {
            TNode row = input->GetRow();
            for (const auto& columnName : ColumnNames) {
                if (!row.AsMap().contains(columnName)) {
                    continue;
                }
                for (const auto& elem : row[columnName].AsMap()) {
                    if (partnersTraffic.contains(elem.first)) {
                        partnersTraffic[elem.first][columnName] += elem.second.AsDouble();
                    } else {
                        THashMap<TString, double> tmp;
                        for (const auto& column : ColumnNames) {
                            if (row.AsMap().contains(column)) {
                                tmp[column] = 0;
                            }
                        }
                        partnersTraffic[elem.first] = tmp;
                        partnersTraffic[elem.first][columnName] += elem.second.AsDouble();
                    }
                }
            }
            ++rowCounter;
        }
        for (const auto& elem : partnersTraffic) {
            TNode outputRow;
            outputRow["key"] = elem.first;
            outputRow["num-mapper-rows"] = rowCounter;
            for (const auto& traffic : elem.second) {
                outputRow[traffic.first] = traffic.second;
            }
            output->AddRow(outputRow);
        }
    }
private:
    TVector<TString> ColumnNames;
};

class TTrafficReducer :
    public IReducer<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    TTrafficReducer() = default;
    TTrafficReducer(
        const TVector<TString>& columnPrefixes,
        const TVector<TString>& columnPrefixesForEval)
            : ColumnPrefixes(columnPrefixes), ColumnPrefixesForEval(columnPrefixesForEval)
        {
            for (const auto& elem : columnPrefixes) {
                ColumnNames.push_back(elem + "-original");
                ColumnNames.push_back(elem + "-rearranged");
            }
            for (const auto& elem : columnPrefixesForEval) {
                ColumnNames.push_back(elem + "-original");
                ColumnNames.push_back(elem + "-rearranged");
                ColumnNames.push_back(elem + "-excluded");
            }
        }
    Y_SAVELOAD_JOB(ColumnNames, ColumnPrefixes, ColumnPrefixesForEval);

    void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
        THashMap<TString, TNode> storage;
        uint64_t numRows = 0;
        for (;input->IsValid(); input->Next()) {
            TNode row = input->GetRow();
            if (numRows == 0) {
                storage["key"] = row["key"];
                for (const auto& column : ColumnNames) {
                    if (row.AsMap().contains(column)) {
                        storage[column] = row[column];
                    }
                }
                numRows += row["num-mapper-rows"].AsUint64();
            } else {
                for (const auto& column : ColumnNames) {
                    if (row.AsMap().contains(column) && storage.contains(column)) {
                        storage[column] = storage[column].AsDouble() + row[column].AsDouble();
                    }
                }
                numRows += row["num-mapper-rows"].AsUint64();
            }
        }
        TNode outputRow;
        outputRow["key"] = storage["key"];
        if (storage.contains("num-queries-with-partner-prs")) {
            outputRow["num-queries-with-partner-prs"] = storage["num-queries-with-partner-prs"];
        }
        outputRow["num_queries"] = numRows;
        for (const auto& prefix : ColumnPrefixes) {
            if (!storage.contains(prefix + "-original") || !storage.contains(prefix + "-rearranged")) {
                continue;
            }
            double originalValueSum =  storage[prefix + "-original"].AsDouble();
            double rearrangedValueSum = storage[prefix + "-rearranged"].AsDouble();
            THashMap<TString, TNode> tmp;
            if (!prefix.StartsWith("num")) {
                tmp[prefix + "-original" + "-avg-all" ] = originalValueSum / numRows;
                tmp[prefix + "-rearranged" + "-avg-all" ] = rearrangedValueSum / numRows;
                if (storage.contains("num-serps-shows-by-partner-original") && storage["num-serps-shows-by-partner-original"].AsDouble() > 0) {
                    double delim = storage["num-serps-shows-by-partner-original"].AsDouble();
                    tmp[prefix + "-original" + "-avg"] = originalValueSum / delim;
                }
                if (storage.contains("num-serps-shows-by-partner-rearranged") && storage["num-serps-shows-by-partner-rearranged"].AsDouble() > 0) {
                    double delim = storage["num-serps-shows-by-partner-rearranged"].AsDouble();
                    tmp[prefix + "-rearranged" + "-avg"] = rearrangedValueSum / delim;
                }
            } else {
                tmp[prefix + "-original" + "-sum"] = originalValueSum;
                tmp[prefix + "-rearranged" + "-sum"] = rearrangedValueSum;
            }
            outputRow[prefix] = tmp;
        }
        for (const auto& prefix : ColumnPrefixesForEval) {
            double originalValueSum =  storage[prefix + "-original"].AsDouble();
            double rearrangedValueSum = storage[prefix + "-rearranged"].AsDouble();
            double excludedValueSum = storage[prefix + "-excluded"].AsDouble();
            THashMap<TString, TNode> tmp;
            if (!prefix.StartsWith("num")) {
                tmp[prefix + "-original" + "-avg-all" ] = originalValueSum / numRows;
                tmp[prefix + "-rearranged" + "-avg-all" ] = rearrangedValueSum / numRows;
                tmp[prefix + "-excluded" + "-avg-all" ] = excludedValueSum / numRows;
                if (storage["num-queries-with-partner-prs"].AsDouble() > 0) {
                    tmp[prefix + "-original" + "-avg" ] = originalValueSum / storage["num-queries-with-partner-prs"].AsDouble();
                    tmp[prefix + "-rearranged" + "-avg" ] = rearrangedValueSum / storage["num-queries-with-partner-prs"].AsDouble();
                    tmp[prefix + "-excluded" + "-avg" ] = excludedValueSum / storage["num-queries-with-partner-prs"].AsDouble();
                }
            } else {
                tmp[prefix + "-original" + "-sum"] = originalValueSum;
                tmp[prefix + "-rearranged" + "-sum"] = rearrangedValueSum;
            }
            outputRow[prefix] = tmp;

        }
        output->AddRow(outputRow);
    }
private:
    TVector<TString> ColumnNames = {"num-queries-with-partner-prs"};
    TVector<TString> ColumnPrefixes;
    TVector<TString> ColumnPrefixesForEval;

    TVector<TString> ColumnNamesForDelta;
};

class TRearrangeReducerVer1 :
    public IReducer<TTableReader<TNode>, TTableWriter<TNode>>
{
public:
    TRearrangeReducerVer1() {}

    TRearrangeReducerVer1(
        TVector<double> alpha,
        double defaultBoostValue,
        const TVector<TString>& partners,
        bool noStoveMode,
        size_t maxPartnersFromHost)
    : DefaultBoostValue(defaultBoostValue), NoStoveMode(noStoveMode), MaxPartnersFromHost(maxPartnersFromHost)
    {
        Y_ENSURE(alpha.size() == partners.size());
        for (size_t i = 0; i < alpha.size(); ++i) {
            Partners[partners[i]] = alpha[i];
        }
    }

    Y_SAVELOAD_JOB(Partners, NoStoveMode, DefaultBoostValue, MaxPartnersFromHost);
    void Do(TTableReader<TNode>* input, TTableWriter<TNode>* output) override {
        TVector<TDocumentContainer<TNode>> serp;
        bool skipNotCommercial = false;
        for (;input->IsValid(); input->Next()) {
            TNode row = input->GetRow();
            AddParsedPrsToSerp(row, Partners, &serp);
            if (row.AsMap().contains("is_commercial_query") && !row["is_commercial_query"].AsBool()) {
                skipNotCommercial = true;
            }
        }
        if (serp.empty()) {
            return;
        }

        Sort(serp.rbegin(), serp.rend(),
            [] (const TDocumentContainer<TNode>& a, const TDocumentContainer<TNode>& b)
            { return a.GetRelevance() < b.GetRelevance(); });
        TVector<TDocumentContainer<TNode>> rearrangedSerp;
        if (skipNotCommercial) {
            rearrangedSerp = serp;
        } else {
            rearrangedSerp = RearrangeDocumentsVer1(serp, DefaultBoostValue, Partners, MaxPartnersFromHost);
        }
        if (!NoStoveMode) {
            for (auto& elem : GetSerp(rearrangedSerp)) {
                output->AddRow(ConvertDocumentContainer(&elem), 0);
            }
        }
        output->AddRow(GetProximaPredictStatRow(serp, rearrangedSerp), 1);
    }
private:
    double DefaultBoostValue = 0.;
    THashMap<TString, double> Partners;
    bool NoStoveMode = false;
    size_t MaxPartnersFromHost = 100;
};

REGISTER_REDUCER(NMiniapp::TRearrangeReducerVer1);
REGISTER_REDUCER(NMiniapp::TRearrangeReducerVer2);
REGISTER_REDUCER(NMiniapp::TRearrangeReducerVer3);
REGISTER_REDUCER(NMiniapp::TTrafficReducer);
REGISTER_MAPPER(NMiniapp::TPartnersExtractor);
REGISTER_MAPPER(NMiniapp::TPartnersTrafficExtractor);

struct TMstandSpec {
    TString InputTable;
    TString PartnerFile;
    TString PartnerBuf;
    TString Output;
    TString YtProxy = "hahn";
};

int Ver1(int argc, const char** argv) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    TMstandSpec storage;
    opts.AddLongOption("input", "Input yt table")
        .Required()
        .StoreResult(&storage.InputTable);
    opts.AddLongOption("output", "Output yt directory")
        .Required()
        .StoreResult(&storage.Output);
    opts.AddLongOption("proxy", "Yt cluster")
        .DefaultValue("hahn")
        .StoreResult(&storage.YtProxy);

    opts.AddLongOption("partners", "file with partners' hosts separated by ,")
        .Required()
        .StoreResult(&storage.PartnerFile);
    TString alphaBuf;
    opts.AddLongOption("values", "boost values for each partner for Ver1 rearrange separated by ,")
        .StoreResult(&alphaBuf);
    bool noStoveMode = false;
    opts.AddLongOption("no-stove", "Do not save rearrange tables for Ver1 Rearrange")
        .Optional()
        .DefaultValue(false)
        .NoArgument()
        .SetFlag(&noStoveMode);
    size_t maxPartnersFromHost = 100;
    opts.AddLongOption("max-partners-from-host", "max partners from host")
        .StoreResult(&maxPartnersFromHost);

    double defaultBoostValue = 0.;
    opts.AddLongOption("default-value", "default boost value for Ver1 rearrange")
        .StoreResult(&defaultBoostValue);

    TString commercialBuf;
    opts.AddLongOption("commercial-factors", "partners' hosts separated by ,")
        .Optional()
        .StoreResult(&commercialBuf);
    TString commercialThresholdsBuf;
    opts.AddLongOption("commercial-thresholds", "partners' hosts separated by ,")
        .Optional()
        .StoreResult(&commercialThresholdsBuf);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    TFileInput inputFile(storage.PartnerFile);
    inputFile.ReadLine(storage.PartnerBuf);
    auto client = CreateClient(storage.YtProxy);
    TVector<TString> partners = SplitString(storage.PartnerBuf, ",");
    TReduceOperationSpec spec;
    spec.AddInput<TNode>(storage.InputTable)
        .ReduceBy({"key"}).SortBy({"key"});

    TVector<TString> commercialFactors = SplitString(commercialBuf, ",");
    TVector<double> commercialThresholds;
    for (const auto& elem: SplitString(commercialThresholdsBuf, ",")) {
        commercialThresholds.push_back(FromString<double>(elem));
    }

    TVector<double> alphas;
    for (auto elem : SplitString(alphaBuf, ",")) {
        alphas.push_back(FromString<double>(elem));
    }
    TString resultPrefix = storage.Output + "/ver1_result";
    TString metricsPrefix = storage.Output + "/ver1_metrics";

    spec.AddOutput<TNode>(resultPrefix);
    spec.AddOutput<TNode>(metricsPrefix);
    client->Reduce(
        spec,
        new NMiniapp::TRearrangeReducerVer1(alphas, defaultBoostValue, partners, noStoveMode, maxPartnersFromHost));
    return 0;
}

int Ver2(int argc, const char** argv) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    TMstandSpec storage;
    opts.AddLongOption("input", "Input yt table")
        .Required()
        .StoreResult(&storage.InputTable);
    opts.AddLongOption("output", "Output yt directory")
        .Required()
        .StoreResult(&storage.Output);
    opts.AddLongOption("proxy", "Yt cluster")
        .DefaultValue("hahn")
        .StoreResult(&storage.YtProxy);

    opts.AddLongOption("partners", "file with partners' hosts separated by ,")
        .Required()
        .StoreResult(&storage.PartnerFile);
    TString alphaBuf;
    opts.AddLongOption("alpha", "alpha values for Ver2 rearrange separated by ,")
        .StoreResult(&alphaBuf);
    size_t maxPartnersFromHost = 100;
    opts.AddLongOption("max-partners-from-host", "max partners from host")
        .StoreResult(&maxPartnersFromHost);


    bool noStoveMode = false;
    opts.AddLongOption("no-stove", "Do not save rearrange tables for Ver2 Rearrange")
        .Optional()
        .DefaultValue(false)
        .NoArgument()
        .SetFlag(&noStoveMode);

    TString commercialBuf;
    opts.AddLongOption("commercial-factors", "partners' hosts separated by ,")
        .Optional()
        .StoreResult(&commercialBuf);
    TString commercialThresholdsBuf;
    opts.AddLongOption("commercial-thresholds", "partners' hosts separated by ,")
        .Optional()
        .StoreResult(&commercialThresholdsBuf);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    TFileInput inputFile(storage.PartnerFile);
    inputFile.ReadLine(storage.PartnerBuf);
    auto client = CreateClient(storage.YtProxy);
    TVector<TString> partners = SplitString(storage.PartnerBuf, ",");
    TReduceOperationSpec spec;
    spec.AddInput<TNode>(storage.InputTable)
        .ReduceBy({"key"}).SortBy({"key"});

    TVector<TString> commercialFactors = SplitString(commercialBuf, ",");
    TVector<double> commercialThresholds;
    for (const auto& elem: SplitString(commercialThresholdsBuf, ",")) {
        commercialThresholds.push_back(FromString<double>(elem));
    }

    TVector<double> alphas;
    for (auto elem : SplitString(alphaBuf, ",")) {
        alphas.push_back(FromString<double>(elem));
    }

    TString resultPrefix = storage.Output + "/ver2_result_";
    TString metricsPrefix = storage.Output + "/ver2_metrics_";

    for (double alpha : alphas) {
        spec.AddOutput<TNode>(resultPrefix + ToString(alpha));
        spec.AddOutput<TNode>(metricsPrefix + "/ver2_metrics_" + ToString(alpha));
    }
    client->Reduce(
        spec,
        new NMiniapp::TRearrangeReducerVer2(alphas, partners, noStoveMode, maxPartnersFromHost));
    return 0;
}

int Ver3(
    int argc,
    const char** argv)
{
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    TMstandSpec storage;
    opts.AddLongOption("input", "Input yt table")
        .Required()
        .StoreResult(&storage.InputTable);
    opts.AddLongOption("output", "Output yt directory")
        .Required()
        .StoreResult(&storage.Output);
    opts.AddLongOption("proxy", "Yt cluster")
        .DefaultValue("hahn")
        .StoreResult(&storage.YtProxy);
    size_t maxPartnersFromHost = 100;
    opts.AddLongOption("max-partners-from-host", "max partners from host")
        .StoreResult(&maxPartnersFromHost);

    opts.AddLongOption("partners", "file with partners' hosts separated by ,")
        .Required()
        .StoreResult(&storage.PartnerFile);

    TString thresholdBuf;
    opts.AddLongOption("thresholds", "thresholds for Ver3 separated by ,")
        .StoreResult(&thresholdBuf);

    bool copyOnTop = false;
    opts.AddLongOption("copy-on-top", "CopyOnTop mode for Ver3 Rearrange")
        .Optional()
        .DefaultValue(false)
        .NoArgument()
        .SetFlag(&copyOnTop);

    bool noStoveMode = false;
    opts.AddLongOption("no-stove", "Do not save rearrange tables for Ver3 Rearrange")
        .Optional()
        .DefaultValue(false)
        .NoArgument()
        .SetFlag(&noStoveMode);

    TString commercialBuf;
    opts.AddLongOption("commercial-factors", "partners' hosts separated by ,")
        .Optional()
        .StoreResult(&commercialBuf);
    TString commercialThresholdsBuf;
    opts.AddLongOption("commercial-thresholds", "partners' hosts separated by ,")
        .Optional()
        .StoreResult(&commercialThresholdsBuf);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    TFileInput inputFile(storage.PartnerFile);
    inputFile.ReadLine(storage.PartnerBuf);

    auto client = CreateClient(storage.YtProxy);
    TVector<TString> partners = SplitString(storage.PartnerBuf, ",");
    TVector<TString> commercialFactors = SplitString(commercialBuf, ",");
    TVector<double> commercialThresholds;
    for (const auto& elem: SplitString(commercialThresholdsBuf, ",")) {
        commercialThresholds.push_back(FromString<double>(elem));
    }

    TReduceOperationSpec spec;
    spec.AddInput<TNode>(storage.InputTable)
        .ReduceBy({"key"}).SortBy({"key"})
        .DataSizePerJob(8_GB)
        .ReducerSpec(TUserJobSpec().MemoryLimit(16_GB));

    TVector<double> thresholds;
    for (auto elem : SplitString(thresholdBuf, ",")) {
        thresholds.push_back(FromString<double>(elem));
    }

    TString resultPrefix = storage.Output + "/ver3_result_";
    TString metricsPrefix = storage.Output + "/ver3_metrics_";
    if (copyOnTop) {
        resultPrefix += "copy_on_top_";
        metricsPrefix += "copy_on_top_";
    }
    for (double threshold : thresholds) {
        spec.AddOutput<TNode>(resultPrefix  + ToString(threshold));
        spec.AddOutput<TNode>(metricsPrefix + ToString(threshold));
    }
    client->Reduce(
        spec,
        new NMiniapp::TRearrangeReducerVer3(thresholds, partners, copyOnTop, true,
            noStoveMode, commercialFactors, commercialThresholds, maxPartnersFromHost));
    return 0;
}

int ExtractPartners(
    int argc,
    const char** argv)
{
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    TMstandSpec storage;
    opts.AddLongOption("input", "Input yt table")
        .Required()
        .StoreResult(&storage.InputTable);
    opts.AddLongOption("output", "Output yt table")
        .Required()
        .StoreResult(&storage.Output);
    opts.AddLongOption("proxy", "Yt cluster")
        .DefaultValue("hahn")
        .StoreResult(&storage.YtProxy);

    opts.AddLongOption("partners", "file with partners' hosts separated by ,")
        .Required()
        .StoreResult(&storage.PartnerFile);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    TFileInput inputFile(storage.PartnerFile);
    inputFile.ReadLine(storage.PartnerBuf);
    auto client = CreateClient(storage.YtProxy);
    TVector<TString> partners = SplitString(storage.PartnerBuf, ",");
    client->Map(
        TMapOperationSpec{}
            .AddInput<TNode>(storage.InputTable)
            .AddOutput<TNode>(storage.Output),
        new TPartnersExtractor(partners)
    );
    return 0;
}

int ExtractPartnersTraffic(
    int argc,
    const char** argv)
{
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    TMstandSpec storage;
    opts.AddLongOption("input", "Input yt table")
        .Required()
        .StoreResult(&storage.InputTable);
    opts.AddLongOption("output", "Output yt directory")
        .Required()
        .StoreResult(&storage.Output);
    opts.AddLongOption("proxy", "Yt cluster")
        .DefaultValue("hahn")
        .StoreResult(&storage.YtProxy);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    auto client = CreateClient(storage.YtProxy);
    TVector<TString> columns = {
        "traffic-by-partner-original",
        "traffic-by-partner-rearranged",
        "clicks-dcg-10-by-partner-original",
        "clicks-dcg-10-by-partner-rearranged",
        "clicks-dcg-10-by-partner-no-fine-original",
        "clicks-dcg-10-by-partner-no-fine-rearranged",
        "eval-exclude-partner-proxima-dcg-5-original",
        "eval-exclude-partner-proxima-dcg-5-excluded",
        "eval-exclude-partner-proxima-dcg-5-rearranged",
        "eval-exclude-partner-proxima-dcg-5-no-fine-original",
        "eval-exclude-partner-proxima-dcg-5-no-fine-excluded",
        "eval-exclude-partner-proxima-dcg-5-no-fine-rearranged",
        "eval-exclude-partner-traffic-sum-original",
        "eval-exclude-partner-traffic-sum-excluded",
        "eval-exclude-partner-traffic-sum-rearranged",
        "num-shows-by-partner-original",
        "num-shows-by-partner-rearranged",
        "num-serps-shows-by-partner-original",
        "num-serps-shows-by-partner-rearranged",
        "num-queries-with-partner-prs",
    };
    TVector<TString> prefixes = {
        "traffic-by-partner",
        "clicks-dcg-10-by-partner",
        "clicks-dcg-10-by-partner-no-fine",
        "num-shows-by-partner",
        "num-serps-shows-by-partner",
        "num-uses-eval-exclude-partner",
    };

    TVector<TString> prefixesForEval = {
        "eval-exclude-partner-traffic-sum",
        "eval-exclude-partner-proxima-dcg-5",
        "eval-exclude-partner-proxima-dcg-5-no-fine",
    };

    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(storage.InputTable)
            .AddOutput<TNode>(storage.Output + "/partners"),
        new TPartnersTrafficExtractor(columns)
    );
    client->Sort(
        TSortOperationSpec()
            .AddInput(storage.Output + "/partners")
            .Output(storage.Output + "/partners")
            .SortBy({"key"})
    );
    client->Reduce(
        TReduceOperationSpec()
            .AddInput<TNode>(storage.Output + "/partners")
            .AddOutput<TNode>(storage.Output + "/partners")
            .ReduceBy({"key"}).SortBy({"key"}),
        new TTrafficReducer(prefixes, prefixesForEval)
    );
    client->Map(
        TMapOperationSpec()
            .AddInput<TNode>(storage.InputTable)
            .AddOutput<TNode>(storage.Output + "/hosts"),
        new TPartnersTrafficExtractor({"traffic-for-host-original", "traffic-for-host-rearranged"})
    );
    client->Sort(
        TSortOperationSpec()
            .AddInput(storage.Output + "/hosts")
            .Output(storage.Output + "/hosts")
            .SortBy({"key"})
    );
    client->Reduce(
        TReduceOperationSpec()
            .AddInput<TNode>(storage.Output + "/hosts")
            .AddOutput<TNode>(storage.Output + "/hosts")
            .ReduceBy({"key"}).SortBy({"key"}),
        new TTrafficReducer({"traffic-for-host"}, {})
    );

    return 0;
}

}

int main(int argc, const char** argv) {
    Initialize(argc, argv);
    TModChooser modChooser;
    modChooser.SetSeparatedMode();
    modChooser.SetVersionHandler(PrintProgramSvnVersion);
    modChooser.AddMode(
        "Ver1",
        &NMiniapp::Ver1,
        "-- Ver 1 rearrange"
        );
    modChooser.AddMode(
        "Ver2",
        &NMiniapp::Ver2,
        "-- Ver 2 rearrange"
        );
    modChooser.AddMode(
        "Ver3",
        &NMiniapp::Ver3,
        "-- Ver 3 rearrange"
        );
    modChooser.AddMode(
        "ExtractPartners",
        &NMiniapp::ExtractPartners,
        "-- ExtractPartners mode"
        );
    modChooser.AddMode(
        "ExtractPartnersTraffic",
        &NMiniapp::ExtractPartnersTraffic,
        "-- ExtractPartnersTraffic mode"
        );

    return modChooser.Run(argc, argv);
}
