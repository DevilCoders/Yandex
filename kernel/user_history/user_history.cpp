#include "user_history.h"

#include <util/datetime/base.h>
#include <util/generic/adaptor.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

namespace NPersonalization {

bool RecordFits(const TUserHistoryRecord& record, const NProto::TEmbeddingOptions& opts) {
    const auto& dwThreshold = opts.GetDwelltimeThreshold();
    const auto& lessThanThreshold = opts.GetLessThanThreshold();
    const auto recordDwelltime = static_cast<ui64>(record.Dwelltime);
    Y_ENSURE(record.ResultType.Defined(), "Empty TUserHistoryRecord::ResultType. Please, fill it.");
    auto withinThreshold = (recordDwelltime < dwThreshold) == lessThanThreshold;
    if (opts.HasDwelltimeThresholdFromOppositeSide()) {
        const auto dwThresholdBound = opts.GetDwelltimeThresholdFromOppositeSide();
        withinThreshold = ((lessThanThreshold && dwThreshold > recordDwelltime && recordDwelltime >= dwThresholdBound)
                       || (!lessThanThreshold && dwThreshold <= recordDwelltime && recordDwelltime < dwThresholdBound));
    }
    return
        record.DocEmbedding.Model == opts.GetModel()
        && (!opts.GetOnlyStateful() || record.StatefulThemes.Defined())
        && (*record.ResultType == opts.GetResultType())
        && withinThreshold;
}

TUserHistoryRecord FilterRecordFields(const TUserHistoryRecord& record, const NProto::TStoreOptions& flags) {
    TUserHistoryRecord resultRecord = record;
    if (!flags.GetStoreTitle()) {
        resultRecord.Title.clear();
    }
    if (!flags.GetStoreOmniTitle()) {
        resultRecord.OmniTitle.clear();
    }
    if (!flags.GetStoreRequestTimestamp()) {
        resultRecord.RequestTimestamp.Clear();
    }
    if (!flags.GetStoreUrl()) {
        resultRecord.Url.clear();
    }
    if (!flags.GetStoreBannerUrl()) {
        resultRecord.BannerUrl.clear();
    }
    if (!flags.GetStoreBannerTitle()) {
        resultRecord.BannerTitle.clear();
    }
    if (!flags.GetStoreBannerText()) {
        resultRecord.BannerText.clear();
    }
    if (!flags.GetStoreQuery()) {
        resultRecord.Query.clear();
    }
    if (!flags.GetStoreEmbedding()) {
        resultRecord.DocEmbedding.Data.clear();
    }
    if (!flags.GetStoreUrlHash()) {
        resultRecord.UrlHash.Clear();
    }
    if (!flags.GetStoreHostHash()) {
        resultRecord.HostHash.Clear();
    }
    if (!flags.GetStoreQueryHash()) {
        resultRecord.QueryHash.Clear();
    }
    if (!flags.GetStorePosition()) {
        resultRecord.Position.Clear();
    }
    if (!flags.GetStoreRegion()) {
        resultRecord.Region.Clear();
    }
    if (!flags.GetStoreDdOsFamily()) {
        resultRecord.DdOsFamily.Clear();
    }
    if (!flags.GetStoreStatefulInfo()) {
        resultRecord.StatefulThemes.Clear();
        resultRecord.StatefulStages.Clear();
    }
    if (!flags.GetStoreDevice()) {
        resultRecord.Device.Clear();
    }
    if (!flags.GetStoreWizardName()) {
        resultRecord.WizardName.clear();
    }
    if (!flags.GetStoreResultType()) {
        resultRecord.ResultType.Clear();
    }

    THashMap<NPersonalization::NProto::EModels, TString> customEmbeddings;
    for (size_t i = 0; i < flags.StoreCustomEmbeddingsSize(); ++i) {
        auto model = flags.GetStoreCustomEmbeddings(i);
        if (auto* embeddingData = resultRecord.CustomEmbeddings.FindPtr(model)) {
            customEmbeddings.emplace(model, std::move(*embeddingData));
        }
    }
    resultRecord.CustomEmbeddings = std::move(customEmbeddings);

    return resultRecord;
}

TUserHistoryRecord::TDocEmbedding TUserHistoryRecord::TDocEmbedding::ConstructFromPB(const NProto::TUserHistoryRecord& recordPB, const NProto::TUserRecordsDescription* descriptionPB, const NProto::EModels modelByDefault) {
    NProto::EModels model = modelByDefault;
    if (recordPB.HasDocEmbeddingModel()) {
        model = recordPB.GetDocEmbeddingModel();
    } else if (descriptionPB && descriptionPB->GetOptions().HasModel()) {
        model = descriptionPB->GetOptions().GetModel();
    }
    return TDocEmbedding{
        model, recordPB.GetDocEmbedding()
    };
}

TUserHistoryRecord TUserHistoryRecord::ConstructFromPB(const NProto::TUserHistoryRecord& recordPB, const NProto::EModels modelByDefault, const NProto::TUserRecordsDescription* descriptionPB) {
    auto docEmbedding = TDocEmbedding::ConstructFromPB(recordPB, descriptionPB, modelByDefault);
    auto result = TUserHistoryRecord{recordPB.GetTimestamp(), recordPB.GetDwelltime(), std::move(docEmbedding)};
    if (recordPB.HasResultType()) {
        result.ResultType = recordPB.GetResultType();
    } else if (descriptionPB) {
        result.ResultType = descriptionPB->GetOptions().GetResultType();
    }

    if (recordPB.HasRequestTimestamp()) {
        result.RequestTimestamp = recordPB.GetRequestTimestamp();
    }
    if (recordPB.HasUrl()) {
        result.Url = recordPB.GetUrl();
    }
    if (recordPB.HasTitle()) {
        result.Title = recordPB.GetTitle();
    }
    if (recordPB.HasOmniTitle()) {
        result.OmniTitle = recordPB.GetOmniTitle();
    }
    if (recordPB.HasBannerUrl()) {
        result.BannerUrl = recordPB.GetBannerUrl();
    }
    if (recordPB.HasBannerTitle()) {
        result.BannerTitle = recordPB.GetBannerTitle();
    }
    if (recordPB.HasBannerText()) {
        result.BannerText = recordPB.GetBannerText();
    }
    result.Flags.CopyFrom(recordPB.GetFlags());
    if (recordPB.HasQuery()) {
        result.Query = recordPB.GetQuery();
    }
    if (recordPB.HasUrlHash()) {
        result.UrlHash = recordPB.GetUrlHash();
    }
    if (recordPB.HasHostHash()) {
        result.HostHash = recordPB.GetHostHash();
    }
    if (recordPB.HasQueryHash()) {
        result.QueryHash = recordPB.GetQueryHash();
    }
    if (recordPB.HasPosition()) {
        result.Position = recordPB.GetPosition();
    }
    if (recordPB.HasRegion()) {
        result.Region = recordPB.GetRegion();
    }
    if (recordPB.HasDdOsFamily()) {
        result.DdOsFamily = recordPB.GetDdOsFamily();
    }
    if (recordPB.HasDevice()) {
        result.Device = recordPB.GetDevice();
    }
    if (recordPB.HasStatefulThemes()) {
        result.StatefulThemes = recordPB.GetStatefulThemes();
    }
    if (recordPB.HasStatefulStages()) {
        result.StatefulStages = recordPB.GetStatefulStages();
    }
    if (recordPB.HasWizardName()) {
        result.WizardName = recordPB.GetWizardName();
    }
    result.ReqidHash = recordPB.GetReqidHash();

    for (const auto& customEmbedding : recordPB.GetCustomEmbeddings()) {
        result.CustomEmbeddings.emplace(customEmbedding.GetModel(), customEmbedding.GetData());
    }

    return result;
}

// TUserHistoryRecord
void TUserHistoryRecord::SaveToPB(NProto::TUserHistoryRecord& recordPB) const {
    recordPB.SetTimestamp(Timestamp);
    recordPB.SetDwelltime(Dwelltime);

    recordPB.SetDocEmbedding(DocEmbedding.Data);
    recordPB.SetDocEmbeddingModel(DocEmbedding.Model);

    if (RequestTimestamp.Defined()) {
        recordPB.SetRequestTimestamp(RequestTimestamp.GetRef());
    }
    if (!Url.empty()) {
        recordPB.SetUrl(Url);
    }
    if (!Title.empty()) {
        recordPB.SetTitle(Title);
    }
    if (!BannerUrl.empty()) {
        recordPB.SetBannerUrl(BannerUrl);
    }
    if (!BannerTitle.empty()) {
        recordPB.SetBannerTitle(BannerTitle);
    }
    if (!BannerText.empty()) {
        recordPB.SetBannerText(BannerText);
    }
    if (!OmniTitle.empty()) {
        recordPB.SetOmniTitle(OmniTitle);
    }
    if (!WizardName.empty()) {
        recordPB.SetWizardName(WizardName);
    }
    recordPB.MutableFlags()->CopyFrom(Flags);
    if (!Query.empty()) {
        recordPB.SetQuery(Query);
    }
    if (ResultType.Defined()) {
        recordPB.SetResultType(ResultType.GetRef());
    }
    if (UrlHash.Defined()) {
        recordPB.SetUrlHash(UrlHash.GetRef());
    }
    if (HostHash.Defined()) {
        recordPB.SetHostHash(HostHash.GetRef());
    }
    if (QueryHash.Defined()) {
        recordPB.SetQueryHash(QueryHash.GetRef());
    }
    if (Position.Defined()) {
        recordPB.SetPosition(Position.GetRef());
    }
    if (Region.Defined()) {
        recordPB.SetRegion(Region.GetRef());
    }
    if (DdOsFamily.Defined()) {
        recordPB.SetDdOsFamily(DdOsFamily.GetRef());
    }
    if (Device.Defined()) {
        recordPB.SetDevice(Device.GetRef());
    }
    if (StatefulThemes.Defined()) {
        recordPB.SetStatefulThemes(StatefulThemes.GetRef());
    }
    if (StatefulStages.Defined()) {
        recordPB.SetStatefulStages(StatefulStages.GetRef());
    }
    recordPB.SetReqidHash(ReqidHash);

    for (auto&& [model, embedding] : CustomEmbeddings) {
        auto& newEmbedding = *recordPB.MutableCustomEmbeddings()->Add();;
        newEmbedding.SetModel(model);
        newEmbedding.SetData(embedding);
    }
}

// TUserHistory
TUserHistory::TUserHistory(const TMaybe<NProto::TUserRecordsDescription> filter)
    : Filter(filter)
{
}

void TUserHistory::Truncate(size_t maxRecords) {
    if (Records.empty()) {
        return;
    }
    if (Records.size() > maxRecords) {
        // resize would require default constructor
        const auto eraseCount = Records.size() - maxRecords;
        Records.erase(Records.end() - eraseCount, Records.end());
    }
}

void TUserHistory::Truncate() {
    Y_ENSURE(Filter.Defined());
    Truncate(Filter->GetMaxRecords());
}

void TUserHistory::SortByDescendingTimestampTruncate() {
    SortByTimestamp();
    Truncate();
}

TUserHistory TUserHistory::Merge(const TUserHistory& other) const {
    TUserHistory newHistory;
    if (Filter.Defined()) {
        newHistory.SetFilter(*Filter);
    }

    auto first = this->rbegin();
    auto firstEnd = this->rend();
    auto second = other.rbegin();
    auto secondEnd = other.rend();

    while (first != firstEnd || second != secondEnd) {
        while (first != firstEnd && (second == secondEnd || first->Timestamp < second->Timestamp)) {
            newHistory.AddRecord(*first++);
        }
        while (second != secondEnd && (first == firstEnd || second->Timestamp < first->Timestamp)) {
            newHistory.AddRecord(*second++);
        }
        while (first != firstEnd && second != secondEnd && first->Timestamp == second->Timestamp) {
            newHistory.AddRecord(*first);
            ++first;
            ++second;
        }
    }

    if (newHistory.Filter.Defined()) {
        newHistory.Truncate();
    }

    return newHistory;
}

void TUserHistory::Append(const TUserHistory& other) {
    Y_ENSURE(Empty() || other.Empty() || Records.front().Timestamp <= other.Records.back().Timestamp
        , ToString(Records.front().Timestamp) + " <= " + ToString(other.Records.back().Timestamp));

    for (const auto& record : Reversed(other)) {
        AddRecord(record);
    }
}

bool TUserHistory::Empty() const {
    return Records.empty();
}

size_t TUserHistory::Size() const {
    return Records.size();
}

void TUserHistory::Clear() {
    Records.clear();
}

bool TUserHistory::AddRecord(const TUserHistoryRecord& record) {
    if (MatchesFilter(record)) {
        if (Filter.Defined() && Filter->HasStoreOptions()) {
            Records.emplace_front(
                FilterRecordFields(record, Filter->GetStoreOptions()));
        } else {
            Records.emplace_front(record);
        }
        return true;
    }
    return false;
}

bool TUserHistory::RemoveRecord(const TUserHistoryRecord& record) {
    if (MatchesFilter(record)) {
        if (Filter.Defined() && Filter->HasStoreOptions()) {
            auto it = std::find(Records.begin(), Records.end(), FilterRecordFields(record, Filter->GetStoreOptions()));
            if (it != Records.end()) {
              Records.erase(it);
              return true;
            }
        } else {
            auto it = std::find(Records.begin(), Records.end(), record);
            if (it != Records.end()) {
              Records.erase(it);
              return true;
            }
        }
    }
    return false;
}

void TUserHistory::PushBack(const TUserHistoryRecord& record) {
    Records.push_back(record);
}

void TUserHistory::PopRecord() {
    if (!Records.empty()) {
        Records.pop_front();
    }
}

void TUserHistory::SortByTimestamp() {
    ::Sort(Records, [] (const TUserHistoryRecord& lhs, const TUserHistoryRecord& rhs) {
        return lhs.Timestamp > rhs.Timestamp;
    });
}

void TUserHistory::SortByDwelltime() {
    ::Sort(Records, [] (const TUserHistoryRecord& lhs, const TUserHistoryRecord& rhs) {
        return lhs.Dwelltime < rhs.Dwelltime;
    });
}

void TUserHistory::DropOldRecords(const TInstant& lowerBound) {
    while (!Empty() && TInstant::Seconds(Records.back().Timestamp) < lowerBound) {
        Records.pop_back();
    }
}

void TUserHistory::DropOldRecords(time_t currentTime) {
    Y_ENSURE(Filter.Defined());
    if (Filter->HasRecordLifetime()) {
        time_t recLifetime = Filter->GetRecordLifetime();
        if (recLifetime > 0 && currentTime >= recLifetime) {
            DropOldRecords(TInstant::Seconds(currentTime - recLifetime));
        }
    }
}

bool TUserHistory::MatchesFilter(const TUserHistoryRecord& record) const {
    return !Filter.Defined() || !Filter->HasOptions() || RecordFits(record, Filter->GetOptions());
}

const NProto::TUserRecordsDescription* TUserHistory::GetDescription() const {
    return Filter.Get();
}

void TUserHistory::LoadFromPBWithFilter(const NProto::TFilteredUserRecords& recordsPB, const NProto::EModels modelByDefault) {
    LoadFromPB(recordsPB, modelByDefault);
    SetFilter(recordsPB.GetDescription());
}
void TUserHistory::SaveToPBWithFilter(NProto::TFilteredUserRecords& recordsPB) const {
    SaveToPB(recordsPB);
    if (Filter.Defined()) {
        recordsPB.MutableDescription()->CopyFrom(*Filter);
    }
}

void TUserHistory::SaveToMirror(NProto::TFilteredUserRecords& recordsPB) const {
    Y_ENSURE(Filter.Defined());
    SaveToPBWithFilter(recordsPB);
    const auto sortOrder = Filter->HasSortOrderOnMirror() ? Filter->GetSortOrderOnMirror() : NProto::SOM_NO_SORT;
    switch(sortOrder) {
        case NProto::SOM_NO_SORT:
            break;
        case NProto::SOM_SORT_BY_TIMESTAMP:
            ::Sort(*recordsPB.MutableRecords(), [] (const auto& lhs, const auto& rhs) {
                return lhs.GetTimestamp() < rhs.GetTimestamp();
            });
            break;
        case NProto::SOM_SORT_BY_DWELLTIME:
            ::Sort(*recordsPB.MutableRecords(), [] (const auto& lhs, const auto& rhs) {
                return lhs.GetDwelltime() < rhs.GetDwelltime();
            });
            break;
        case NProto::SOM_SORT_BY_REQUEST_TIMESTAMP:
            ::Sort(*recordsPB.MutableRecords(), [] (const auto& lhs, const auto& rhs) {
                return lhs.GetRequestTimestamp() < rhs.GetRequestTimestamp();
            });
            break;
    }
}

TUserHistory::TIterator TUserHistory::begin() const {
    return Records.begin();
}

TUserHistory::TIterator TUserHistory::end() const {
    return Records.end();
}

TUserHistory::TReverseIterator TUserHistory::rbegin() const {
    return Records.rbegin();
}

TUserHistory::TReverseIterator TUserHistory::rend() const {
    return Records.rend();
}

const TUserHistoryRecord& TUserHistory::front() const {
    return Records.front();
}

const TUserHistoryRecord& TUserHistory::back() const {
    return Records.back();
}

TUserHistory TUserHistory::FromString(const TString& data, const NProto::EModels modelByDefault) {
    NProto::TUserHistory historyPB;
    const auto result = historyPB.ParseFromString(data);
    Y_ASSERT(result);
    TUserHistory history;
    history.LoadFromPB(historyPB, modelByDefault);
    return history;
}

TString TUserHistory::AsString() const {
    NProto::TUserHistory historyPB;
    SaveToPB(historyPB);
    TString data;
    Y_PROTOBUF_SUPPRESS_NODISCARD historyPB.SerializeToString(&data);
    return data;
}

void TUserHistory::SetFilter(const NProto::TUserRecordsDescription& filter) {
    Filter = filter;
}

} // namespace NPersonalization
