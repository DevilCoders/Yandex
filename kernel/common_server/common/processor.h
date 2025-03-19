#pragma once

#include <kernel/common_server/abstract/common.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/report/json.h>
#include <kernel/common_server/library/searchserver/simple/context/replier.h>
#include <kernel/common_server/library/scheme/handler.h>
#include <kernel/common_server/util/accessor.h>

#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/xml/document/xml-document.h>

#include <util/generic/ptr.h>
#include <util/thread/pool.h>
#include <library/cpp/deprecated/atomic/atomic.h>

#include "abstract.h"
#include "params_processor.h"
#include "url_matcher.h"

class TGeoCoord;
class TRegExMatch;

class IRequestProcessorConfig;

class IRequestProcessor: public IRequestParamsProcessor {
protected:
    IReplyContext::TPtr Context;
    TString ThreadPoolName;
    const TRegExMatch* AccessControlAllowOrigin;
    bool DebugModeFlag = false;
    TDuration RateLimitFreshness = TDuration::Seconds(10);
    mutable TAtomic RateLimit = 0;
    mutable TAtomic RateLimitRefresh = 0;
    mutable THolder<NJson::TJsonValue> JsonData;
    mutable THolder<NXml::TDocument> XMLData;
    using TUrlParams = TMap<TString, TString>;
    CSA_PROTECTED_DEF(IRequestProcessor, TUrlParams, UrlParams);
    ui64 MaxRequestSize = 0;
private:
    using TBase = IRequestParamsProcessor;

    bool CheckRequestsRateLimit() const;
    IUserProcessorCustomizer::TPtr UserCustomizer;

    void InitJsonData() const;

protected:
    virtual void DoProcess(TJsonReport::TGuard& g) = 0;
    virtual NCS::NObfuscator::TObfuscatorKeyMap GetObfuscatorKey() const {
        auto result = NCS::NObfuscator::TObfuscatorKeyMap();
        result.Add("handler_type", GetHandlerName());
        return result;
    }

    virtual bool DoFillHandlerScheme(NCS::NScheme::THandlerScheme& scheme, const IBaseServer& server) const;
    bool CheckCache(const TString& hash, TJsonReport::TGuard& g, const TInstant reqActuality) const;
    void UpdateCache(const TString& hash, TJsonReport::TGuard& g);

    TMaybe<NCS::NScheme::THandlerResponse> BuildDefaultScheme(const IBaseServer& server) const;
    IFrontendReportBuilder::TCtx GetReportContext() const;

    TString GetContentType() const {
        return TString{Context->GetBaseRequestData().HeaderInOrEmpty("Content-Type")};
    }

    TMaybe<TString> FindRequestParam(const TString& paramName) const {
        auto it = GetUrlParams().find(paramName);
        if (it != GetUrlParams().end()) {
            return it->second;
        } else if (Context->GetCgiParameters().Has(paramName)) {
            return Context->GetCgiParameters().Get(paramName);
        } else if (HasJsonData() && GetJsonData().Has(paramName) && GetJsonData()[paramName].IsString()) {
            return GetJsonData()[paramName].GetString();
        }
        return Nothing();
    }

    const TString FindRequestParamDef(const TString& paramName, const TString& defaultValue) const {
        return FindRequestParam(paramName).GetOrElse(defaultValue);
    }

    bool HasJsonData() const;

    const NJson::TJsonValue& GetJsonData() const {
        InitJsonData();
        ReqCheckCondition(!!JsonData, ConfigHttpStatus.SyntaxErrorStatus, "http.request.content.incorrect_json");
        return *JsonData;
    }

    const NXml::TDocument& GetXMLData() const {
        if (!XMLData) {
            ReqCheckCondition(GetContentType().EndsWith("xml"), ConfigHttpStatus.SyntaxErrorStatus, "http.request.xml.content-type.incorrect");
            TBlob postData = Context->GetBuf();
            TString inp(postData.AsCharPtr(), postData.Size());
            try {
                auto doc = MakeHolder<NXml::TDocument>(inp, NXml::TDocument::String);
                XMLData.Reset(doc.Release());
            } catch (...) {
                ReqCheckCondition(false, ConfigHttpStatus.SyntaxErrorStatus, "http.request.xml.content.incorrect");
            }
        }
        return *XMLData;
    }

    const TBlob& GetRawData() const {
        return Context->GetBuf();
    }

    const TString GetData() const {
        TString postData(GetRawData().AsCharPtr(), GetRawData().Size());
        bool trim = false;
        if (postData.StartsWith("\"")) {
            trim = true;
        }
        const TString prefix =  TString((trim ? "\"" : "")) + "data: application/x-www-form-urlencoded;base64,";
        if (postData.StartsWith(prefix)) {
            // Format: "data: application/x-www-form-urlencoded;base64,BASE64DATA"
            ui64 dataSize = postData.size() - prefix.size();
            if (trim) {
                dataSize -= 1;
            }
            return Base64StrictDecode(TStringBuf(postData.Data() + prefix.Size(), dataSize));
        }
        return postData;
    }

    const TCgiParameters& GetCgiParameters() const {
        return Context->GetCgiParameters();
    }

    const TInstant GetRequestStartTime() const {
        return Context->GetRequestStartTime();
    }

    const TCgiParameters GetXFormUrlEncodedData() const {
        return TCgiParameters(GetRawData().AsStringBuf());
    }

public:
    virtual TVector<TString> GetHandlerSettingPathes() const {
        return {"handlers." + GetHandlerName(), "handlers.default"};
    }

    using TPtr = TAtomicSharedPtr<IRequestProcessor>;

public:
    IRequestProcessor(const IRequestProcessorConfig& config, IReplyContext::TPtr context, const IBaseServer* server);

    TMaybe<NCS::NScheme::THandlerScheme> BuildScheme(const NCS::TPathHandlerInfo& pathInfo, const IBaseServer& server) const;

    IRequestProcessor& SetUserCustomizer(IUserProcessorCustomizer::TPtr customizer) {
        UserCustomizer = customizer;
        return *this;
    }

    IReplyContext::TPtr GetContext() const;
    virtual IThreadPool* GetHandler() const;

    virtual void Process() final;

    bool IsDebugMode() const {
        return DebugModeFlag;
    }

    virtual void AfterFinish(const int code) {
        Y_UNUSED(code);
    }

    TMaybe<TGeoCoord> GetUserLocation(const TString& cgiUserLocation = "") const;

    template <class T>
    TMaybe<T> GetHandlerSetting(const TString& key) const {
        if (!BaseServer) {
            return {};
        }
        TString value;
        if (UserCustomizer && UserCustomizer->OverrideOption("handlers." + GetHandlerName() + "." + key, value)) {
            T result;
            if (TryFromString(value, result)) {
                return result;
            }
        }
        const NFrontend::ISettings& settings = BaseServer->GetSettings();
        auto local = settings.GetValue<T>("handlers." + GetHandlerName() + "." + key);
        if (local) {
            return local;
        }
        if (UserCustomizer && UserCustomizer->OverrideOption("handlers.default." + key, value)) {
            T result;
            if (TryFromString(value, result)) {
                return result;
            }
        }
        auto global = settings.GetValue<T>("handlers.default." + key);
        if (global) {
            return global;
        }
        return {};
    }

    template <class T>
    T GetHandlerSettingDef(const TString& key, const T& defaultValue) const {
        auto mayBeResult = GetHandlerSetting<T>(key);
        if (!mayBeResult) {
            return defaultValue;
        } else {
            return *mayBeResult;
        }
    }
};

class IRequestProcessorConfig {
public:
    constexpr static TDuration NotInitializedTimeout = TDuration::Seconds(30);
private:
    CSA_READONLY_DEF(TString, AdditionalCgi);
    CSA_READONLY_DEF(TString, OverrideCgi);
    CSA_READONLY_DEF(TString, OverrideCgiPart);
    CSA_READONLY_DEF(TString, OverridePost);
    CSA_READONLY(TDuration, RequestTimeout, NotInitializedTimeout);
    CSA_READONLY(TDuration, RateLimitFreshness, TDuration::Seconds(10));
    CSA_READONLY(ui64, MaxRequestSize, 0);
    CSA_READONLY_DEF(TString, HandlerName);
    CSA_READONLY_DEF(TString, ProcessorType);

    using TParams = TMap<TString, TString>;
    CSA_READONLY_DEF(TParams, DefaultUrlParams);
private:
    TString ThreadPoolName = "default";
protected:
    virtual bool DoInit(const TYandexConfig::Section* /*section*/) {
        return true;
    }

    virtual void DoToString(IOutputStream& /*os*/) const {
    }

public:
    using TFactory = NObjectFactory::TParametrizedObjectFactory<IRequestProcessorConfig, TString, TString>;
    using TPtr = TAtomicSharedPtr<IRequestProcessorConfig>;

public:
    IRequestProcessorConfig(const TString& handlerName);
    virtual ~IRequestProcessorConfig();

    const TRegExMatch* GetAccessControlAllowOrigin() const {
        return AccessControlAllowOriginRegEx.Get();
    }

    virtual const TString& GetThreadPoolName() const final {
        return ThreadPoolName;
    }

    virtual IRequestProcessor::TPtr ConstructProcessor(IReplyContext::TPtr context, const IBaseServer* server, const TMap<TString, TString>& selectionParams) const final;
    virtual void CheckServerForProcessor(const IBaseServer* server) const;

    virtual bool Init(const TYandexConfig::Section* section) final;
    virtual void ToString(IOutputStream& os) const;

private:
    virtual IRequestProcessor::TPtr DoConstructProcessor(IReplyContext::TPtr context, const IBaseServer* server) const = 0;

private:
    TString AccessControlAllowOrigin;
    THolder<TRegExMatch> AccessControlAllowOriginRegEx;
};

class TRequestProcessorConfigContainer: public TBaseInterfaceContainer<IRequestProcessorConfig> {
private:
    using TBase = TBaseInterfaceContainer<IRequestProcessorConfig>;
    using TSelectionParameters = TMap<TString, TString>;
    CSA_DEFAULT(TRequestProcessorConfigContainer, TSelectionParameters, SelectionParams);
public:
    using TBase::TBase;
};
