#include "captcha_api_check.h"

#include "eventlog_err.h"
#include "neh_requesters.h"

#include <antirobot/lib/http_helpers.h>

namespace NAntiRobot {
    NThreading::TFuture<TErrorOr<TCaptchaApiResult>> GetCaptchaApiResultAsync(const TCaptchaKey& key,
                                                                              const TRequest& request,
                                                                              const TCaptchaApiOptions& options,
                                                                              TCaptchaStat& captchaStat) {
        static const TString method = "/check";
        static const size_t logErrorBytes = 2048;

        const auto& rep = request.CaptchaRequest.TextResponse;
        TCaptchaApiResult result;
        if (!rep) {
            result.ImageCheckOk = false;
            result.Warnings.push_back("Empty rep");
        }
        if (!key.ImageKey) {
            result.ImageCheckOk = false;
            result.Warnings.push_back("Empty key");
        }
        if (result.Warnings) {
            return NThreading::MakeFuture(TErrorOr<TCaptchaApiResult>(std::move(result)));
        }

        auto http_request = HttpGet(options.Host, method);
        http_request.AddCgiParam("key", key.ImageKey.substr(0, 256))
            .AddCgiParam("rep", rep.substr(0, 256))
            .AddCgiParam("client", "antirobot")
            .AddCgiParam("service-for", ToString(request.HostType))
            .AddCgiParam("json", "1");

        auto measureDuration = MakeAtomicShared<TMeasureDuration>(captchaStat.GetApiCaptchaCheckResponseTimeStats());
        return FetchHttpDataAsync(&TCaptchaNehRequester::Instance(), options.Host, http_request, options.Timeout, options.Protocol)
            .Apply([measureDuration](const auto& future) mutable -> TErrorOr<TCaptchaApiResult> {
                measureDuration.Drop();
                NNeh::TResponseRef response;
                if (TError err = future.GetValue().PutValueTo(response); err.Defined()) {
                    return TError(__LOCATION__ + yexception() << "Failed to send request (" << err->what() << ")");
                }
                if (response && !response->IsError()) {
                    NJson::TJsonValue jsonValue;
                    if (!NJson::ReadJsonTree(response->Data, &jsonValue)) {
                        return TError(__LOCATION__ + yexception() << "Failed to send request (invalid json"
                                                                  << (response->Data.size() > logErrorBytes ? " [truncated]" : "")
                                                                  << " '" << response->Data.substr(0, logErrorBytes) << "')");
                    }
                    TCaptchaApiResult result;
                    result.ImageCheckOk = jsonValue.Has("status") && jsonValue["status"].GetString() == "ok";
                    if (jsonValue.Has("answer") && jsonValue["answer"].IsString()) {
                        result.Answers.emplace_back(jsonValue["answer"].GetStringSafe());
                    }
                    if (jsonValue.Has("voice_answer") && jsonValue["voice_answer"].IsString()) {
                        result.Answers.emplace_back(jsonValue["voice_answer"].GetStringSafe());
                    }
                    if (jsonValue.Has("session_metadata") && jsonValue["session_metadata"].IsMap()) {
                        result.SessionMetadata = jsonValue["session_metadata"];
                    }
                    if (jsonValue.Has("error")) {
                        TString warning = jsonValue["error"].GetString();
                        if (jsonValue.Has("error_desc")) {
                            warning += "; " + jsonValue["error_desc"].GetString();
                        }
                        result.Warnings.emplace_back(warning);
                    }
                    return result;
                }
                if (!response) {
                    return TError(__LOCATION__ + yexception() << "Failed to send request (null response)");
                } else {
                    return TError(__LOCATION__ + yexception() << "Failed to send request "
                                                              << "(type=" << static_cast<int>(response->GetErrorType())
                                                              << "; code=" << response->GetErrorCode()
                                                              << "; system code=" << response->GetSystemErrorCode()
                                                              << "): " << response->GetErrorText()
                                                              << "; " << response->Data.substr(0, logErrorBytes));
                }
            });
    }
}
