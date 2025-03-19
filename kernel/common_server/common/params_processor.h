#pragma once

#include <library/cpp/cgiparam/cgiparam.h>

#include <kernel/common_server/abstract/common.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/util/accessor.h>

class IRequestParamsProcessor {
private:
    CSA_READONLY_DEF(TString, HandlerName);
public:
    explicit IRequestParamsProcessor(const IBaseServer* server, const TString& handlerName);
    virtual ~IRequestParamsProcessor() = default;

    void ReqCheckCondition(const bool checkValue, const ui32 code, const TString& errorId, const std::function<NJson::TJsonValue()>& getJsonDetails) const;
    void ReqCheckCondition(const bool checkValue, const ui32 code, const TString& errorId, const NJson::TJsonValue& jsonDetails = NJson::JSON_STRING) const;
    void ReqCheckCondition(const bool checkValue, const ui32 code, const ELocalizationCodes errorId, const NJson::TJsonValue& jsonDetails = NJson::JSON_STRING) const;

    TVector<TString> GetStrings(const TCgiParameters& cgi, TStringBuf name, bool required = true, ui64 itemsLimit = 0) const;
    TVector<TString> GetStrings(const NJson::TJsonValue& data, TStringBuf name, bool required = true, ui64 itemsLimit = 0) const;
    TVector<TString> GetStrings(const TCgiParameters& cgi, TConstArrayRef<TStringBuf> names, bool merge = true, bool required = true, ui64 itemsLimit = 0) const;
    TVector<TString> GetStrings(const NJson::TJsonValue& data, TConstArrayRef<TStringBuf> names, bool merge = true, bool required = true, ui64 itemsLimit = 0) const;

    TMaybe<TString> GetStringMaybe(const TCgiParameters& cgi, TStringBuf name, bool required = true, bool notEmpty = false) const;
    TString GetString(const TCgiParameters& cgi, TStringBuf name, bool required = true, bool notEmpty = false) const;
    TString GetString(const NJson::TJsonValue& data, TStringBuf name, bool required = true, bool notEmpty = false) const;
    TString GetString(const TCgiParameters& cgi, TConstArrayRef<TStringBuf> names, bool required = true, bool notEmpty = false) const;
    TString GetString(const NJson::TJsonValue& data, TConstArrayRef<TStringBuf> names, bool required = true, bool notEmpty = false) const;

    template <class T>
    TDuration GetDuration(const T& request, TStringBuf name, bool required = true) const;
    template <class T>
    TDuration GetDuration(const T& request, TStringBuf name, TDuration fallback) const;

    template <class T>
    TInstant GetTimestamp(const T& request, TStringBuf name, bool required = true) const;
    template <class T>
    TInstant GetTimestamp(const T& request, TStringBuf name, TInstant fallback) const;

    bool IsUUID(const TString& value) const;

    void ValidateUUIDs(const TVector<TString>& values) const;
    void ValidateUUID(const TString& value) const;

    template <class... TArgs>
    TVector<TString> GetUUIDs(TArgs... args) const;

    template <class... TArgs>
    TString GetUUID(TArgs... args) const;

    template <class T>
    T ParseValue(TStringBuf value) const;

    template<>
    TString ParseValue(TStringBuf value) const;

    template <class T, class... TArgs>
    TVector<T> GetValuesIgnoreErrors(TArgs... args) const;

    template <class T, class... TArgs>
    TVector<T> GetValues(TArgs... args) const;

    template <class T, class... TArgs>
    TSet<T> GetValuesSet(TArgs... args) const;

    template <class T, class... TArgs>
    TMaybe<T> GetValue(TArgs... args) const;

protected:
    const THttpStatusManagerConfig ConfigHttpStatus;
    const IBaseServer* BaseServer;

    template <class TServer>
    const TServer& GetServer() const {
        return *VerifyDynamicCast<const TServer*>(BaseServer);
    }

    template <class TServer>
    const TServer& GetServerAs() const {
        return *VerifyDynamicCast<const TServer*>(BaseServer);
    }

    TCSSignals::TSignalBuilder Signal(const TString& name = "") const {
        return TCSSignals::Signal(HandlerName + name ? ("." + name) : "");
    }

    TCSSignals::TSignalBuilder SignalProblem(const TString& name = "") const {
        return TCSSignals::SignalProblem(HandlerName + name ? ("." + name) : "");
    }

    TString GetHandlerLocalization(const TString& resourceId, const TString& defaultResult) const;

private:
    template <class T>
    TVector<TString> GetStringsImpl(const T& source, TConstArrayRef<TStringBuf> names, bool merge, bool required, ui64 itemsLimit) const;

    template <class T>
    TString GetStringImpl(const T& source, TConstArrayRef<TStringBuf> names, bool required, bool notEmpty) const;
};

template <class T>
TDuration IRequestParamsProcessor::GetDuration(const T& request, TStringBuf name, bool required) const {
    auto value = GetValue<ui64>(request, name, required);
    if (value) {
        return TDuration::MicroSeconds(*value);
    } else {
        return TDuration::Zero();
    }
}

template <class T>
TDuration IRequestParamsProcessor::GetDuration(const T& request, TStringBuf name, TDuration fallback) const {
    auto result = GetDuration<T>(request, name, /*required=*/false);
    if (result) {
        return result;
    } else {
        return fallback;
    }
}

template <class T>
TInstant IRequestParamsProcessor::GetTimestamp(const T& request, TStringBuf name, bool required) const {
    auto value = GetValue<ui64>(request, name, required);
    if (value) {
        if (value > 9999999999) {
            return TInstant::MilliSeconds(*value);
        } else {
            return TInstant::Seconds(*value);
        }
    } else {
        return TInstant::Zero();
    }
}

template <class T>
TInstant IRequestParamsProcessor::GetTimestamp(const T& request, TStringBuf name, TInstant fallback) const {
    auto result = GetTimestamp<T>(request, name, /*required=*/false);
    if (result) {
        return result;
    } else {
        return fallback;
    }
}

template <class... TArgs>
TVector<TString> IRequestParamsProcessor::GetUUIDs(TArgs... args) const {
    auto result = GetStrings(args...);
    ValidateUUIDs(result);
    return result;
}

template <class... TArgs>
TString IRequestParamsProcessor::GetUUID(TArgs... args) const {
    auto result = GetString(args...);
    if (!!result) {
        ValidateUUID(result);
    }
    return result;
}

template <class T>
T IRequestParamsProcessor::ParseValue(TStringBuf value) const {
    T result;
    TStringStream errorMessage;
    errorMessage << "cannot parse " << value << " as " << ::TypeName<T>();
    ReqCheckCondition(
        TryFromString(value, result),
        ConfigHttpStatus.UserErrorState,
        errorMessage.Str()
    );
    return result;
}

template <class T, class... TArgs>
TVector<T> IRequestParamsProcessor::GetValuesIgnoreErrors(TArgs... args) const {
    TVector<T> result;
    for (auto&& i : GetStrings(args...)) {
        T value;
        if (TryFromString<T>(i, value)) {
            result.emplace_back(std::move(value));
        }
    }
    return result;
}
template <class T, class... TArgs>
TVector<T> IRequestParamsProcessor::GetValues(TArgs... args) const {
    TVector<T> result;
    for (auto&& i : GetStrings(args...)) {
        result.push_back(ParseValue<T>(i));
    }
    return result;
}

template <class T, class... TArgs>
TSet<T> IRequestParamsProcessor::GetValuesSet(TArgs... args) const {
    TSet<T> result;
    for (const auto& x : GetStrings(args...)) {
        result.emplace(ParseValue<T>(x));
    }
    return result;
}

template <class T, class... TArgs>
TMaybe<T> IRequestParamsProcessor::GetValue(TArgs... args) const {
    TString s = GetString(args...);
    if (s) {
        return ParseValue<T>(s);
    } else {
        return {};
    }
}
