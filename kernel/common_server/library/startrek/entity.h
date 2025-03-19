#pragma once

#include <kernel/common_server/library/startrek/config.h>
#include <kernel/common_server/library/startrek/logger.h>

#include <kernel/common_server/library/async_impl/client.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/generic/cast.h>
#include <util/string/cast.h>

class TStartrekTicket {
    using TAdditionalAttributes = THashMap<TString, NJson::TJsonValue>;

    RTLINE_ACCEPTOR_DEF(TStartrekTicket, Description, TString);
    RTLINE_ACCEPTOR_DEF(TStartrekTicket, Summary, TString);

protected:
    TAdditionalAttributes AdditionalAttributes;

public:
    enum class ETicketField {
        Key /* "key" */,
        QueueKey /* "queue.key" */,
        StatusKey /* "status.key" */,
    };

    TStartrekTicket() = default;
    TStartrekTicket(const TStartrekTicket&) = default;
    virtual ~TStartrekTicket() = default;

    explicit TStartrekTicket(const NJson::TJsonValue& data) {
        if (!DeserializeFromJson(data)) {
            TFLEventLog::Error("Bad statrek ticket deserialization");
        }
    }

    bool Empty() const {
        return !Description && !Summary && !AdditionalAttributes;
    }

    explicit operator bool() const {
        return !Empty();
    }

    const NJson::TJsonValue* GetAdditionalValue(const TString& path) const;

    bool GetAdditionalValue(const TString& path, TString& value) const;
    bool GetAdditionalValue(const ETicketField& field, TString& value) const;
    TString GetAdditionalValueDef(const TString& path, const TString& defaultValue = "") const;
    TString GetAdditionalValueDef(const ETicketField& field, const TString& defaultValue = "") const;

    bool SetAdditionalValue(const TString& key, const NJson::TJsonValue& value);

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& data);
    virtual NJson::TJsonValue SerializeToJson() const;

    template <typename T>
    static TMaybe<T> BuildFromJson(const NJson::TJsonValue& data) {
        T ticket;
        if (!ticket.DeserializeFromJson(data)) {
            return Nothing();
        }
        return ticket;
    }

    template <class T>
    const T* GetAs() const {
        return dynamic_cast<const T*>(this);
    }

    template <class T>
    const T& GetAsSafe() const {
        return *VerifyDynamicCast<const T*>(this);
    }

    enum class EAdditionalValuesAction {
        SET /* "set" */,
        ADD /* "add" */,
        REMOVE /* "remove" */
    };

    template <typename TContainer>
    bool GetAdditionalContainerValue(const TString& key, TContainer& result) const;

    template <typename TContainer>
    void OperateAdditionalContainerValue(const TString& key, EAdditionalValuesAction action, const TContainer& values, bool modifyExisting = false);

    template <typename TContainer>
    void SetAdditionalContainerValue(const TString& key, const TContainer& values, bool modifyExisting = false);

    template <typename TContainer>
    void AddAdditionalContainerValue(const TString& key, const TContainer& values, bool modifyExisting = false);

    template <typename TContainer>
    void RemoveAdditionalContainerValue(const TString& key, const TContainer& values, bool modifyExisting = false);

protected:
    bool GetSerializableAdditionalAttribute(const TString& key, const NJson::TJsonValue& value, NJson::TJsonValue& serializableValue) const;
};

template <typename TContainer>
bool TStartrekTicket::GetAdditionalContainerValue(const TString& key, TContainer& result) const {
    if (!AdditionalAttributes.contains(key)) {
        return false;
    }
    using TValue = TString;
    auto insertOperator = std::inserter(result, result.begin());
    for (const auto& i : AdditionalAttributes.at(key).GetArray()) {
        TValue value;
        if (!TryFromString<TValue>(i.GetStringRobust(), value)) {
            return false;
        } else {
            *insertOperator++ = value;
        }
    }
    return true;
}

template <typename TContainer>
void TStartrekTicket::OperateAdditionalContainerValue(const TString& key, EAdditionalValuesAction action, const TContainer& values, bool modifyExisting) {
    if (!AdditionalAttributes.contains(key)) {
        AdditionalAttributes[key] = NJson::JSON_ARRAY;
    }

    NJson::TJsonValue& actualValues = AdditionalAttributes[key];

    if (!modifyExisting) {
        TString actionName = ToString(action);
        if (!actualValues.Has(actionName)) {
            actualValues[actionName] = NJson::JSON_ARRAY;
        }
        for (const auto& value : values) {
            actualValues[actionName].AppendValue(value);
        }
    } else {
        if (EAdditionalValuesAction::SET == action) {
            actualValues = NJson::JSON_ARRAY;
            for (const auto& value : values) {
                actualValues.AppendValue(value);
            }
        } else if (EAdditionalValuesAction::ADD == action) {
            for (const auto& value : values) {
                actualValues.AppendValue(value);
            }
        } else if (EAdditionalValuesAction::REMOVE == action) {
            using TValue = typename TContainer::value_type;
            NJson::TJsonValue filtered(NJson::JSON_ARRAY);
            TSet<TValue> removeFilter(values.cbegin(), values.cend());
            for (const auto& i : actualValues.GetArray()) {
                TValue actualValue;
                if (TryFromString<TValue>(i.GetStringRobust(), actualValue) && removeFilter.contains(actualValue)) {
                    continue;
                }
                filtered.AppendValue(i);  // remains original value
            }
            AdditionalAttributes[key] = filtered;
        } else {
            Y_UNREACHABLE();
        }
    }
}

template <typename TContainer>
void TStartrekTicket::SetAdditionalContainerValue(const TString& key, const TContainer& values, bool modifyExisting) {
    OperateAdditionalContainerValue<TContainer>(key, EAdditionalValuesAction::SET, values, modifyExisting);
}

template <typename TContainer>
void TStartrekTicket::AddAdditionalContainerValue(const TString& key, const TContainer& values, bool modifyExisting) {
    OperateAdditionalContainerValue<TContainer>(key, EAdditionalValuesAction::ADD, values, modifyExisting);
}

template <typename TContainer>
void TStartrekTicket::RemoveAdditionalContainerValue(const TString& key, const TContainer& values, bool modifyExisting) {
    OperateAdditionalContainerValue<TContainer>(key, EAdditionalValuesAction::REMOVE, values, modifyExisting);
}

class TStartrekComment {
    using TSummonees = TVector<TString>;

    RTLINE_ACCEPTOR(TStartrekComment, Id, ui64, 0);
    RTLINE_ACCEPTOR(TStartrekComment, CreatedAt, TInstant, TInstant::Zero());
    RTLINE_ACCEPTOR(TStartrekComment, UpdatedAt, TInstant, TInstant::Zero());
    RTLINE_ACCEPTOR_DEF(TStartrekComment, Text, TString);
    RTLINE_ACCEPTOR_DEF(TStartrekComment, Summonees, TSummonees);

public:
    TStartrekComment() = default;
    TStartrekComment(const TString& text, const TSummonees& summonees = Default<TSummonees>())  // implicit conversion from TString
        : Text(text)
        , Summonees(summonees)
    {
    }
    ~TStartrekComment() = default;

    explicit operator bool() const {
        return !Text.Empty();
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& info);
    Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TString& s);
    NJson::TJsonValue SerializeToJson() const;
};

class TStartrekTransition {
    RTLINE_ACCEPTOR_DEF(TStartrekTransition, Name, TString);
    RTLINE_ACCEPTOR_DEF(TStartrekTransition, DisplayName, TString);
    RTLINE_ACCEPTOR_DEF(TStartrekTransition, TargetStatusKey, TString);
    RTLINE_ACCEPTOR_DEF(TStartrekTransition, TargetStatusName, TString);

public:
    TStartrekTransition() = default;
    ~TStartrekTransition() = default;

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& info);
};
