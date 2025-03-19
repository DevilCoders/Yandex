#pragma once

#include "async_impl.h"
#include "config.h"
#include "logger.h"

#include <util/generic/vector.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>

enum class ERequestMethod {
    GET /* "GET" */,
    POST /* "POST" */,
    PUT /* "PUT" */,
    PATCH /* "PATCH" */,
    DELETE /* "DELETE" */,
};

enum class ERequestContentType {
    Json /* "application/json" */,
    UnicodeJson /* "application/json; charset=utf-8" */,
    OctetStream /* "application/octet-stream" */
};

static const TVector<ui32> AdmissibleResponseCodes = { HttpCodes::HTTP_OK, HttpCodes::HTTP_CREATED, HttpCodes::HTTP_NO_CONTENT };

inline bool IsRequestSuccessful(const ui32 code) {
    return std::any_of(AdmissibleResponseCodes.cbegin(), AdmissibleResponseCodes.cend(), [code](const ui32 c){return c == code;});
}

template <typename TOperationType,
          typename TLogger = TRequestLogger<TOperationType>,
          typename std::enable_if<std::is_base_of<TRequestLogger<TOperationType>, TLogger>::value, int>::type = 0>
class TAsyncRequestCallback : public NNeh::THttpAsyncReport::ICallback {
    using TBase = NNeh::THttpAsyncReport::ICallback;

    RTLINE_CONST_ACCEPTOR(Type, TOperationType);
    RTLINE_READONLY_ACCEPTOR_DEF(Logger, TLogger);

public:
    TAsyncRequestCallback(const TOperationType type, const TLogger& logger)
        : TBase()
        , Type(type)
        , Logger(logger)
    {}

    virtual void OnResponse(const TVector<NNeh::THttpAsyncReport>& reports) override {
        CHECK_WITH_LOG(reports.size() == 1);
        auto result = reports.front();

        DEBUG_LOG << "Code: " << result.GetHttpCode() << Endl;
        Code = result.GetHttpCode();

        if (!NJson::ReadJsonFastTree(*result.GetReport(), &Result)) {
            Code = HttpCodes::HTTP_INTERNAL_SERVER_ERROR;
            ErrorMessage = "Incorrect json format";
        } else {
            ProcessResponseData();
        }
        Logger.ProcessReply(Type, Code);
        if (!IsRequestSuccessful(Code)) {
            Logger.ProcessError(Type);
        }
    }

private:
    virtual void ProcessResponseData() {
        if (!IsRequestSuccessful(Code)) {
            ErrorMessage = ToString(Code) + (Result.IsDefined() ? " " + Result.GetStringRobust() : "");
            ERROR_LOG << ErrorMessage << Endl;
        }
    }

protected:
    ui32 Code = HTTP_INTERNAL_SERVER_ERROR;
    NJson::TJsonValue Result;
    TString ErrorMessage;
};

template <typename TOperationType,
          typename TLogger = TRequestLogger<TOperationType>,
          typename std::enable_if<std::is_base_of<TRequestLogger<TOperationType>, TLogger>::value, int>::type = 0>
class TRequestCallback : public TAsyncRequestCallback<TOperationType, TLogger> {
public:
    using TAsyncBase = TAsyncRequestCallback<TOperationType, TLogger>;

public:
    TRequestCallback(NJson::TJsonValue& result, ui32& code, TString& errorMessage, const TOperationType type, const TLogger& logger)
        : TAsyncBase(type, logger)
        , ExternalCode(code)
        , ExternalResult(result)
        , ExternalErrorMessage(errorMessage)
    {}

    virtual void OnResponse(const TVector<NNeh::THttpAsyncReport>& reports) override {
        TAsyncBase::OnResponse(reports);
        ExternalCode = TAsyncBase::Code;
        ExternalResult = TAsyncBase::Result;
        ExternalErrorMessage = TAsyncBase::ErrorMessage;
    }

protected:
    ui32& ExternalCode;
    NJson::TJsonValue& ExternalResult;
    TString& ExternalErrorMessage;
};

template <typename TConfig,
          typename TLogger,
          typename TOperationType = typename TLogger::TOperationTypeImpl,
          typename TCallback = TRequestCallback<TOperationType, TLogger>,
          typename TAsyncCallback = typename TCallback::TAsyncBase,
          typename std::enable_if< std::is_base_of<TRequestLogger<TOperationType>, TLogger>::value &&
                                   std::is_base_of<TRequestConfig, TConfig>::value &&
                                   std::is_base_of<TRequestCallback<TOperationType, TLogger>, TCallback>::value &&
                                   std::is_base_of<TAsyncRequestCallback<TOperationType, TLogger>, TAsyncCallback>::value,
                                   int >::type = 0 >
class TRequestClient {
public:
    TRequestClient(const TConfig& config, const TString& apiName)
        : TRequestClient(config, apiName, apiName)
    {
    }

    TRequestClient(const TConfig& config, const TString& apiName, const TString& loggerName)
        : Config(config)
        , Logger(loggerName)
        , Impl(config, apiName)
    {
    }

    TRequestClient(const TConfig& config, const TString& apiName, const TString& errorSource, const TString& signalSource)
        : Config(config)
        , Logger(errorSource, signalSource)
        , Impl(config, apiName)
    {
    }

    virtual ~TRequestClient() = default;

    const TConfig& GetConfig() const {
        return Config;
    }

protected:
    NNeh::THttpRequest CreateCommonRequest(const TString& uri, const TString& method = "GET", const TString& postData = Default<TString>(), const TString& contentType = "") const {
        NNeh::THttpRequest req;
        req.SetUri(uri);
        req.SetRequestType(method);
        if (method == "POST" || method == "PUT" || method == "PATCH") {
            req.SetPostData(postData, method);
        }
        if (contentType) {
            req.AddHeader("Content-Type", contentType);
        }
        Config.Authorize(req);
        return req;
    }

    NNeh::THttpRequest CreateCommonRequest(const TString& uri, const ERequestMethod method, const NJson::TJsonValue& postData, const ERequestContentType contentType) const {
        return CreateCommonRequest(uri, ::ToString(method), postData.GetStringRobust(), ::ToString(contentType));
    }

    bool SendRequest(TOperationType operationType, const NNeh::THttpRequest& request, NJson::TJsonValue& reply) const {
        Logger.ProcessStart(operationType);

        ui32 code;
        TString errorMessage;

        const TInstant deadline = Now() + Config.GetRequestTimeout();
        auto g = Impl->SendPtr(request, deadline, MakeAtomicShared<TCallback>(reply, code, errorMessage, operationType, Logger));
        g.Wait();

        if (IsRequestSuccessful(code)) {
            DEBUG_LOG << reply.GetStringRobust() << Endl;
            return true;
        }

        Logger.ProcessError(errorMessage);
        return false;
    }

    void SendAsyncRequest(TOperationType operationType, const NNeh::THttpRequest& request) const {
        Logger.ProcessStart(operationType);

        const TInstant deadline = Now() + Config.GetRequestTimeout();
        auto g = Impl->SendPtr(request, deadline, MakeAtomicShared<TAsyncCallback>(operationType, Logger));
    }

protected:
    TConfig Config;
    TLogger Logger;

private:
    TAsyncApiImpl Impl;
};
