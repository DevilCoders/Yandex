#pragma once

#include <kernel/user_history/proto/user_history.pb.h>

#include <library/cpp/binsaver/bin_saver.h>

#include <util/ysaveload.h>
#include <util/generic/deque.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/stream/str.h>

class TInstant;

namespace NPersonalization {

struct TUserHistoryRecord {
    struct TDocEmbedding {
        NProto::EModels Model;
        TString Data;
        TDocEmbedding(NProto::EModels model, const TString& data)
            : Model(model)
            , Data(data)
        {
        }

        static TDocEmbedding ConstructFromPB(const NProto::TUserHistoryRecord& recordPB, const NProto::TUserRecordsDescription* descriptionPB, const NProto::EModels modelByDefault);

        bool operator== (const TDocEmbedding& other) const {
            return (Model == other.Model) && (Data == other.Data);
        }
    };
    time_t Timestamp;
    time_t Dwelltime;
    TMaybe<time_t> RequestTimestamp;
    TMaybe<ui32> Position;

    TDocEmbedding DocEmbedding;
    THashMap<NProto::EModels, TString> CustomEmbeddings;

    TString Url;
    TString Title;
    TString OmniTitle;
    NPersonalization::NProto::TUserHistoryRecord::TFlags Flags;

    TString BannerUrl;
    TString BannerTitle;
    TString BannerText;

    TString Query;
    TMaybe<i32> Region;
    TMaybe<TString> DdOsFamily;
    TMaybe<TString> Device;
    TMaybe<TString> StatefulThemes;
    TMaybe<TString> StatefulStages;

    TMaybe<ui64> UrlHash;
    TMaybe<ui64> HostHash;
    TMaybe<ui64> QueryHash;
    ui64 ReqidHash = 0;

    TMaybe<NProto::EResultType> ResultType = NProto::EResultType::ORGANIC;
    TString WizardName;

    TUserHistoryRecord(time_t timestamp, time_t dwelltime, const TDocEmbedding& docEmbedding)
        : Timestamp(timestamp)
        , Dwelltime(dwelltime)
        , DocEmbedding(docEmbedding)
    {
    }

    TUserHistoryRecord(time_t timestamp, time_t dwelltime, const TDocEmbedding& docEmbedding, TString url, TString title)
        : Timestamp(timestamp)
        , Dwelltime(dwelltime)
        , DocEmbedding(docEmbedding)
        , Url(url)
        , Title(title)
    {
    }

    bool operator== (const TUserHistoryRecord& other) const {
        return (Timestamp == other.Timestamp)
            && (Dwelltime == other.Dwelltime)
            && (DocEmbedding == other.DocEmbedding);
    }

    // Model by default will be used when field DocEmbeddingModel is not set
    static TUserHistoryRecord ConstructFromPB(const NProto::TUserHistoryRecord& recordPB, const NProto::EModels modelByDefault, const NProto::TUserRecordsDescription* descriptionPB = nullptr);
    void SaveToPB(NProto::TUserHistoryRecord& recordPB) const;
};

bool RecordFits(const TUserHistoryRecord& record, const NProto::TEmbeddingOptions& opts);
TUserHistoryRecord FilterRecordFields(const TUserHistoryRecord& record, const NProto::TStoreOptions& flags);

class TUserHistory {
    // TODO: use more sophisticated storing (eliminate duplication and so on)
    TDeque<TUserHistoryRecord> Records;
    TMaybe<NProto::TUserRecordsDescription> Filter;

public:
    TUserHistory(const TMaybe<NProto::TUserRecordsDescription> filter = Nothing());

    void Truncate(size_t maxRecords);
    void Truncate();
    TUserHistory Merge(const TUserHistory& other) const;
    void Append(const TUserHistory& other);
    bool Empty() const;
    size_t Size() const;
    void Clear();
    bool AddRecord(const TUserHistoryRecord& record);
    bool AddRecord(TUserHistoryRecord&& record, const NProto::TStoreOptions& flags);
    bool RemoveRecord(const TUserHistoryRecord& record);
    void PushBack(const TUserHistoryRecord& record);
    void PopRecord();
    void SortByTimestamp();
    void SortByDescendingTimestampTruncate();
    void DropOldRecords(time_t currentTime);
    void DropOldRecords(const TInstant& lowerBound);
    void SortByDwelltime();
    bool MatchesFilter(const TUserHistoryRecord& record) const;
    const NProto::TUserRecordsDescription* GetDescription() const;

    using TIterator = decltype(Records)::const_iterator;
    using TReverseIterator = decltype(Records)::const_reverse_iterator;

    TIterator begin() const;
    TIterator end() const;
    TReverseIterator rbegin() const;
    TReverseIterator rend() const;
    const TUserHistoryRecord& front() const;
    const TUserHistoryRecord& back() const;

    inline const NProto::TUserRecordsDescription* GetDescription(const NProto::TUserHistory&) {
        return nullptr;
    }

    inline const NProto::TUserRecordsDescription* GetDescription(const NProto::TFilteredUserRecords& historyPB) {
        return &historyPB.GetDescription();
    }

    template <typename TProtoContainer>
    void AddFromPB(const TProtoContainer& historyPB, const NProto::EModels modelByDefault) {
        for (size_t i = 0; i < historyPB.RecordsSize(); ++i) {
            const NProto::TUserHistoryRecord& recordPB = historyPB.GetRecords(i);
            auto record = TUserHistoryRecord::ConstructFromPB(recordPB, modelByDefault, GetDescription(historyPB));
            AddRecord(record);
        }
    }

    template <typename TProtoContainer>
    void LoadFromPB(const TProtoContainer& historyPB, const NProto::EModels modelByDefault) {
        Clear();
        AddFromPB(historyPB, modelByDefault);
    }

    template <typename TProtoContainer>
    void SaveToPB(TProtoContainer& historyPB) const {
        historyPB.Clear();
        for (auto recordIt = this->rbegin(); recordIt != this->rend(); ++recordIt) {
            NProto::TUserHistoryRecord& recordPB = *historyPB.AddRecords();
            recordIt->SaveToPB(recordPB);
        }
    }

    void LoadFromPBWithFilter(const NProto::TFilteredUserRecords& recordsPB, const NProto::EModels modelByDefault);
    void SaveToPBWithFilter(NProto::TFilteredUserRecords& recordsPB) const;

    void SaveToMirror(NProto::TFilteredUserRecords& recordsPB) const;

    static TUserHistory FromString(const TString& data, const NProto::EModels modelByDefault);
    TString AsString() const;
private:
    void SetFilter(const NProto::TUserRecordsDescription& filter);
};

} // namespace NPersonalization
