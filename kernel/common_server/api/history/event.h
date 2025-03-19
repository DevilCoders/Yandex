#pragma once
#include "common.h"
#include "filter.h"
#include <kernel/common_server/abstract/common.h>
#include <kernel/common_server/library/storage/selection/filter.h>
#include <kernel/common_server/library/storage/selection/sorting.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/typetraits.h>

class IBaseServer;

template <class TObject>
class TObjectEventDecoder: public TObject::TDecoder {
private:
    using TSelf = TObjectEventDecoder<TObject>;
    CS_ACCESS(TSelf, i32, HistoryAction, -1);
    CS_ACCESS(TSelf, i32, HistoryUserId, -1);
    CS_ACCESS(TSelf, i32, HistoryOriginatorId, -1);
    CS_ACCESS(TSelf, i32, HistoryEventId, -1);
    CS_ACCESS(TSelf, i32, HistoryComment, -1);
    CS_ACCESS(TSelf, i32, HistoryInstant, -1);

public:
    TObjectEventDecoder() = default;

    static TString GetFieldsForRequest() {
        TString result = TObject::TDecoder::GetFieldsForRequest();
        if (result != "*") {
            result += ",history_action,history_user_id,history_originator_id,history_event_id,history_timestamp,history_comment";
        }
        return result;
    }

    TObjectEventDecoder(const TMap<TString, ui32>& decoderOriginal)
        : TObject::TDecoder(decoderOriginal)
    {
        {
            auto it = decoderOriginal.find("history_action");
            CHECK_WITH_LOG(it != decoderOriginal.end());
            HistoryAction = it->second;
        }
        {
            auto it = decoderOriginal.find("history_user_id");
            CHECK_WITH_LOG(it != decoderOriginal.end());
            HistoryUserId = it->second;
        }
        {
            auto it = decoderOriginal.find("history_originator_id");
            if (it != decoderOriginal.end()) {
                HistoryOriginatorId = it->second;
            }
        }
        {
            auto it = decoderOriginal.find("history_event_id");
            CHECK_WITH_LOG(it != decoderOriginal.end());
            HistoryEventId = it->second;
        }
        {
            auto it = decoderOriginal.find("history_timestamp");
            CHECK_WITH_LOG(it != decoderOriginal.end());
            HistoryInstant = it->second;
        }
        {
            auto it = decoderOriginal.find("history_comment");
            if (it != decoderOriginal.end()) {
                HistoryComment = it->second;
            }
        }
    }
};

template <class TEvent>
class THistoryObjectDescription {
public:
    static TString GetSeqFieldName() {
        return "history_event_id";
    }

    static TString GetTimestampFieldName() {
        return "history_timestamp";
    }

    static TString GetIdFieldName() {
        return TEvent::GetIdFieldName();
    }
};

template <class TObject>
class TObjectEvent: public TObject {
public:
    using TEventId = ui64;
    using TEntity = TObject;
private:
    Y_HAS_MEMBER(GetObjectIdFieldName);
public:
    static constexpr auto IncorrectEventId = Max<TEventId>();
    using TPtr = TAtomicSharedPtr<TObjectEvent<TObject>>;
private:
    using TBase = TObject;
    using TSelf = TObjectEvent<TObject>;

public:
    CS_ACCESS(TObjectEvent, EObjectHistoryAction, HistoryAction, EObjectHistoryAction::Unknown);
    CS_ACCESS(TObjectEvent, TInstant, HistoryInstant, TInstant::Max());
    CSA_DEFAULT(TObjectEvent, TString, HistoryUserId);
    CSA_DEFAULT(TObjectEvent, TString, HistoryOriginatorId);
    CS_ACCESS(TObjectEvent, TEventId, HistoryEventId, IncorrectEventId);
    CSA_DEFAULT(TObjectEvent, TString, HistoryComment);
public:
    class TFilter: public NCS::NSelection::NFilter::TComposite {
    public:
        TFilter() {
            Register<NCS::NSelection::NFilter::TInstantInterval>().SetInstantFieldName("history_timestamp");
            if constexpr (THasGetObjectIdFieldName<TObject>::value) {
                Register<NCS::NSelection::NFilter::TObjectIds>().SetObjectIdFieldName(TObject::GetObjectIdFieldName());
            } else {
                Register<NCS::NSelection::NFilter::TObjectIds>().SetObjectIdFieldName(TObject::GetIdFieldName());
            }
        }
    };

    class TSorting: public NCS::NSelection::NSorting::TLinear {
    public:
        TSorting() {
            RegisterField("history_event_id");
        }
    };

    using TDecoder = TObjectEventDecoder<TObject>;

    template <class TReportItemPolicy>
    NJson::TJsonValue BuildReportItemCustom() const {
        NJson::TJsonValue item;
        item.InsertValue("timestamp", HistoryInstant.Seconds());
        item.InsertValue("event_id", HistoryEventId);
        item.InsertValue("user_id", HistoryUserId);
        if (!!HistoryOriginatorId) {
            item.InsertValue("originator_id", HistoryOriginatorId);
        }
        item.InsertValue("action", ToString(HistoryAction));

        TReportItemPolicy::DoBuildReportItem(this, item);
        return item;
    }

    NJson::TJsonValue BuildReportItem() const {
        NJson::TJsonValue item;
        item.InsertValue("timestamp", HistoryInstant.Seconds());
        item.InsertValue("event_id", HistoryEventId);
        item.InsertValue("user_id", HistoryUserId);
        if (!!HistoryOriginatorId) {
            item.InsertValue("originator_id", HistoryOriginatorId);
        }
        item.InsertValue("action", ToString(HistoryAction));

        TObject::DoBuildReportItem(item);
        return item;
    }

    bool operator< (const TSelf& obj) const {
        if (HistoryInstant == obj.HistoryInstant) {
            return HistoryEventId < obj.HistoryEventId;
        } else {
            return HistoryInstant < obj.HistoryInstant;
        }
    }

    TObjectEvent() = default;
    TObjectEvent(const TObject& object, const EObjectHistoryAction action, const TInstant& instant, const TString& actorId, const TString& originatorId, const TString& comment)
        : TBase(object)
        , HistoryAction(action)
        , HistoryInstant(instant)
        , HistoryUserId(actorId)
        , HistoryOriginatorId(originatorId)
        , HistoryComment(comment)
    {
    }

    void FillHistoryInfo(NJson::TJsonValue& json) const {
        JWRITE(json, "history_event_id", HistoryEventId);
        JWRITE(json, "history_timestamp", HistoryInstant.Seconds());
        JWRITE(json, "history_action", ::ToString(HistoryAction));
        JWRITE(json, "history_originator_id", HistoryOriginatorId);
        JWRITE(json, "history_user_id", HistoryUserId);
        JWRITE(json, "history_comment", HistoryComment);
        json.InsertValue("_title_event", HistoryUserId + ": " + ::ToString(HistoryAction) + ": " + ::ToString(HistoryInstant.Seconds()));
    }

    NJson::TJsonValue SerializeToJson() const {
        auto json = TBase::SerializeToJson();
        FillHistoryInfo(json);
        return json;
    }

    NStorage::TTableRecord SerializeToTableRecord() const {
        NStorage::TTableRecord result = TBase::SerializeToTableRecord();
        if (HistoryEventId != IncorrectEventId) {
            result.Set("history_event_id", HistoryEventId);
        }
        result.Set("history_action", HistoryAction).Set("history_timestamp", HistoryInstant.Seconds()).Set("history_user_id", HistoryUserId);
        if (!!HistoryOriginatorId) {
            result.Set("history_originator_id", HistoryOriginatorId);
        }
        if (!!HistoryComment) {
            result.Set("history_comment", HistoryComment);
        }
        return result;
    }

    bool DeserializeWithDecoder(const TObjectEventDecoder<TObject>& decoder, const TConstArrayRef<TStringBuf>& values) {
        try {
            if (!TBase::DeserializeWithDecoder(decoder, values)) {
                return false;
            }
        } catch (...) {
            ERROR_LOG << CurrentExceptionMessage() << Endl;
            return false;
        }
        READ_DECODER_VALUE(decoder, values, HistoryAction);
        READ_DECODER_VALUE(decoder, values, HistoryUserId);
        READ_DECODER_VALUE(decoder, values, HistoryOriginatorId);
        READ_DECODER_VALUE(decoder, values, HistoryEventId);
        READ_DECODER_VALUE(decoder, values, HistoryComment);
        READ_DECODER_VALUE_INSTANT(decoder, values, HistoryInstant);
        return true;
    }

    bool DeserializeFromTableRecord(const NCS::NStorage::TTableRecordWT& record) {
        return TBaseDecoder::DeserializeFromTableRecordStrictable(*this, record, false);
    }

    template <class TServer>
    static NFrontend::TScheme GetScheme(const TServer& server) {
        NFrontend::TScheme result = TBase::GetScheme(server);
        result.Add<TFSString>("history_user_id").SetReadOnly(true);
        result.Add<TFSNumeric>("history_event_id").SetReadOnly(true);
        result.Add<TFSString>("history_originator_id").SetReadOnly(true);
        result.Add<TFSString>("history_action").SetReadOnly(true);
        result.Add<TFSNumeric>("history_timestamp").SetReadOnly(true);
        result.Add<TFSString>("history_comment").SetReadOnly(true);
        result.Add<TFSString>("_title_event").SetReadOnly(true);
        return result;
    }

    static NFrontend::TScheme GetSearchScheme(const IBaseServer& /*server*/) {
        return THistoryEventsFilterImpl<THistoryObjectDescription<TSelf>, TSelf>::GetScheme();
    }
};
