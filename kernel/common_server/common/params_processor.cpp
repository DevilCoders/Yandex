#include "params_processor.h"

#include <kernel/common_server/library/network/data/data.h>
#include <kernel/common_server/util/coded_exception.h>

#include <util/generic/guid.h>
#include <util/string/subst.h>

IRequestParamsProcessor::IRequestParamsProcessor(const IBaseServer* server, const TString& handlerName)
    : HandlerName(handlerName)
    , ConfigHttpStatus(server->GetHttpStatusManagerConfig())
    , BaseServer(server)
{
}

void IRequestParamsProcessor::ReqCheckCondition(const bool checkValue, const ui32 code, const TString& errorId, const std::function<NJson::TJsonValue()>& getJsonDetails) const {
    TString errorIdResult = "handlers." + HandlerName + "." + errorId;
    SubstGlobal(errorIdResult, "/", ".");
    if (!checkValue) {
        TCodedException exception(code);
        exception.AddErrorCode(::ToString(code));
        exception.SetErrorMessage(errorId);
        exception.SetDetails(getJsonDetails());
        throw exception << errorIdResult;
    }
}

void IRequestParamsProcessor::ReqCheckCondition(const bool checkValue, const ui32 code, const ELocalizationCodes errorId, const NJson::TJsonValue& jsonDetails) const {
    ReqCheckCondition(checkValue, code, ::ToString(errorId), jsonDetails);
}

void IRequestParamsProcessor::ReqCheckCondition(const bool checkValue, const ui32 code, const TString& errorId, const NJson::TJsonValue& jsonDetails) const {
    ReqCheckCondition(checkValue, code, errorId, [&jsonDetails]() { return jsonDetails; });
}

TString IRequestParamsProcessor::GetHandlerLocalization(const TString& resourceId, const TString& defaultResult) const {
    if (!BaseServer) {
        return defaultResult;
    }
    const ILocalization& localization = BaseServer->GetLocalization();
    return localization.GetLocalString("rus", "handlers." + HandlerName + "." + resourceId, localization.GetLocalString("rus", "handlers.default." + resourceId, defaultResult));
}

TVector<TString> IRequestParamsProcessor::GetStrings(const TCgiParameters& cgi, TStringBuf name, bool required /*= true*/, ui64 itemsLimit /*= 0*/) const {
    TVector<TString> result;
    auto range = cgi.equal_range(name);
    for (auto i = range.first; i != range.second; ++i) {
        StringSplitter(i->second).SplitBySet(",").SkipEmpty().AddTo(&result);
    }
    if (required) {
        ReqCheckCondition(
            !result.empty(),
            ConfigHttpStatus.EmptyRequestStatus,
            TString("missing ") + name + " in Cgi parameters"
        );
    }
    if (itemsLimit) {
        ReqCheckCondition(result.size() <= itemsLimit, ConfigHttpStatus.UserErrorState, TStringBuilder() << "Parameter '" << name << "' contains " << result.size() << " elements, max allowed " << itemsLimit << ".");
    }
    return result;
}

TVector<TString> IRequestParamsProcessor::GetStrings(const NJson::TJsonValue& data, TStringBuf name, bool required /*= true*/, ui64 itemsLimit /*= 0*/) const {
    TVector<TString> result;
    if (!required) {
        if (!data.Has(name)) {
            return result;
        }
    } else {
        ReqCheckCondition(
            data.Has(name),
            ConfigHttpStatus.EmptyRequestStatus,
            TString("missing ") + name + " in JSON: " + data.GetStringRobust()
        );
    }
    const NJson::TJsonValue& section = data[name];
    if (section.IsArray()) {
        for (auto&& i : section.GetArraySafe()) {
            auto s = i.GetStringRobust();
            for (auto&& v : StringSplitter(s).Split(',')) {
                result.emplace_back(v.Token());
            }
        }
    } else {
        StringSplitter(section.GetStringRobust()).SplitBySet(", ").SkipEmpty().AddTo(&result);
    }
    if (required) {
        ReqCheckCondition(
            !result.empty(),
            ConfigHttpStatus.EmptyRequestStatus,
            TString("missing ") + name + " in JSON: " + data.GetStringRobust()
        );
    }
    if (itemsLimit) {
        ReqCheckCondition(result.size() <= itemsLimit, ConfigHttpStatus.UserErrorState, TStringBuilder() << "Parameter '" << name << "' contains " << result.size() << " elements, max allowed " << itemsLimit << ".");
    }
    return result;
}

template <class T>
TVector<TString> IRequestParamsProcessor::GetStringsImpl(const T& source, TConstArrayRef<TStringBuf> names, bool merge, bool required, ui64 itemsLimit) const {
    TVector<TString> result;
    for (auto&& name : names) {
        auto ss = GetStrings(source, name, false);
        if (result.empty()) {
            result = std::move(ss);
        } else {
            result.insert(result.end(), ss.begin(), ss.end());
        }
        if (!result.empty() && !merge) {
            break;
        }
    }
    if (required) {
        ReqCheckCondition(!result.empty(), ConfigHttpStatus.EmptyRequestStatus, TString("missing ") + JoinSeq(",", names));
    }
    if (itemsLimit) {
        ReqCheckCondition(result.size() <= itemsLimit, ConfigHttpStatus.UserErrorState, TStringBuilder() << "Parameters '" << JoinStrings(names.begin(), names.end(), "', '") << "' combined contains " << result.size() << " elements, max allowed " << itemsLimit << ".");
    }
    return result;
}

TVector<TString> IRequestParamsProcessor::GetStrings(const TCgiParameters& cgi, TConstArrayRef<TStringBuf> names, bool merge /*= true*/, bool required /*= true*/, ui64 itemsLimit /*= 0*/) const {
    return GetStringsImpl(cgi, names, merge, required, itemsLimit);
}

TVector<TString> IRequestParamsProcessor::GetStrings(const NJson::TJsonValue& data, TConstArrayRef<TStringBuf> names, bool merge /*= true*/, bool required /*= true*/, ui64 itemsLimit /*= 0*/) const {
    return GetStringsImpl(data, names, merge, required, itemsLimit);
}

TMaybe<TString> IRequestParamsProcessor::GetStringMaybe(const TCgiParameters& cgi, TStringBuf name, bool required /*= true*/, bool notEmpty /*= false*/) const {
    TMaybe<TString> result;
    auto range = cgi.equal_range(name);
    for (auto i = range.first; i != range.second; ++i) {
        ReqCheckCondition(!result, ConfigHttpStatus.SyntaxErrorStatus, TString("multiple ") + name + " are specified");
        result = i->second;
    }
    if (required) {
        ReqCheckCondition(
            !!result,
            ConfigHttpStatus.EmptyRequestStatus,
            TString("missing ") + name + " in Cgi parameters"
        );
    }
    if (notEmpty) {
        ReqCheckCondition(
            !result || *result,
            ConfigHttpStatus.EmptyRequestStatus,
            TString("empty ") + name + " in Cgi parameters"
        );
    }

    return result;
}

TString IRequestParamsProcessor::GetString(const TCgiParameters& cgi, TStringBuf name, bool required, bool notEmpty /*= false*/) const {
    return GetStringMaybe(cgi, name, required, notEmpty).GetOrElse({});
}

TString IRequestParamsProcessor::GetString(const NJson::TJsonValue& data, TStringBuf name, bool required /*= true*/, bool notEmpty /*= false*/) const {
    const NJson::TJsonValue& parameter = data[name];
    ReqCheckCondition(
        !required || parameter.IsString() || parameter.IsBoolean() || parameter.IsDouble() || parameter.IsInteger() || parameter.IsUInteger(),
        ConfigHttpStatus.SyntaxErrorStatus,
        TString("required POD type for ") + name + " field"
    );
    if (parameter.IsDefined()) {
        ReqCheckCondition(!notEmpty || parameter.GetStringRobust(), ConfigHttpStatus.SyntaxErrorStatus, TString("field ") + name + " is empty");
        return parameter.GetStringRobust();
    } else {
        ReqCheckCondition(!notEmpty, ConfigHttpStatus.SyntaxErrorStatus, TString("field ") + name + " is empty");
        return {};
    }
}

template <class T>
TString IRequestParamsProcessor::GetStringImpl(const T& source, TConstArrayRef<TStringBuf> names, bool required, bool notEmpty) const {
    TString result;
    for (size_t i = 0; i < names.size(); ++i) {
        bool req = required ? (i + 1 == names.size()) : false;
        result = GetString(source, names[i], req, notEmpty && req);
        if (result) {
            break;
        }
    }
    return result;
}

TString IRequestParamsProcessor::GetString(const TCgiParameters& cgi, TConstArrayRef<TStringBuf> names, bool required /*= true*/, bool notEmpty /*= false*/) const {
    return GetStringImpl(cgi, names, required, notEmpty);
}

TString IRequestParamsProcessor::GetString(const NJson::TJsonValue& data, TConstArrayRef<TStringBuf> names, bool required /*= true*/, bool notEmpty /*= false*/) const {
    return GetStringImpl(data, names, required, notEmpty);
}

template<>
TString IRequestParamsProcessor::ParseValue(TStringBuf value) const {
    return TString(value.Data(), value.Size());
}

bool IRequestParamsProcessor::IsUUID(const TString& value) const {
    return !GetUuid(value).IsEmpty();
}

void IRequestParamsProcessor::ValidateUUIDs(const TVector<TString>& values) const {
    for (auto&& value : values) {
        ValidateUUID(value);
    }
}

void IRequestParamsProcessor::ValidateUUID(const TString& value) const {
    ReqCheckCondition(
        IsUUID(value),
        ConfigHttpStatus.SyntaxErrorStatus,
        TString("ill-formed UUID ") + value
    );
}
