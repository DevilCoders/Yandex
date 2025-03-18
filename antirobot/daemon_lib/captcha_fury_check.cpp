#include "captcha_fury_check.h"

#include "eventlog_err.h"
#include "neh_requesters.h"

#include <antirobot/lib/http_helpers.h>
#include <library/cpp/json/writer/json.h>

namespace NAntiRobot {
    static const size_t LOG_ERROR_BYTES = 2048;

    static TError ParseFuryError(TSourceLocation location, const TString& message, const TString& response) {
        return TError(location + yexception() << "Failed to send request ("
                                              << message << ", '"
                                              << (response.size() > LOG_ERROR_BYTES ? " [truncated]" : "")
                                              << TStringBuf(response).Head(LOG_ERROR_BYTES)
                                              << "')");
    }

    TErrorOr<TVector<EFuryCategory>> ParseFuryResult(const TString& response) {
        NJson::TJsonValue jsonValue;
        if (!NJson::ReadJsonTree(response, &jsonValue)) {
            return ParseFuryError(__LOCATION__, "invalid json", response);
        }

        TVector<EFuryCategory> result;
        if (!jsonValue.Has("result") || !jsonValue["result"].IsArray()) {
            return ParseFuryError(__LOCATION__, "invalid json[result]", response);
        }

        TVector<EFuryCategory> allCategories;
        for (const auto& result : jsonValue["result"].GetArraySafe()) {
            if (!result.Has("name") || !result["name"].IsString()) {
                return ParseFuryError(__LOCATION__, "invalid json[result][name]", response);
            }
            if (!result.Has("value") || !result["value"].IsBoolean()) {
                return ParseFuryError(__LOCATION__, "invalid json[result][value]", response);
            }
            TString name = result["name"].GetStringSafe();
            bool value = result["value"].GetBooleanSafe();
            if (value) {
                EFuryCategory categ;
                if (TryFromString<EFuryCategory>(name, categ)) {
                    allCategories.push_back(categ);
                }
            }
        }
        return allCategories;
    }

    NThreading::TFuture<TErrorOr<TCaptchaFuryResult>> GetCaptchaFuryResultAsync(
        const TCaptchaKey& key,
        const TCaptchaApiResult& captchaApiResult,
        const TRequest& request,
        const TFuryOptions& options,
        TCaptchaSettingsPtr settings,
        TTimeStats& timeStat
    ) {
        if (!options.Enabled || !captchaApiResult.Warnings.empty()) {
            TCaptchaFuryResult result;
            result.CheckOk = captchaApiResult.ImageCheckOk;
            return NThreading::MakeFuture(TErrorOr<TCaptchaFuryResult>(std::move(result)));
        }

        auto http_request = HttpPost(options.Host, "/");
        http_request.AddHeader(X_YA_SERVICE_TICKET, options.TVMServiceTicket);
        TString requestString;
        TStringOutput so(requestString);
        NJsonWriter::TBuf json(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);
        json.BeginObject();
        {
            json.WriteKey("jsonrpc").WriteString("2.0");
            json.WriteKey("method").WriteString("process");
            json.WriteKey("id").WriteString("1");
            json.WriteKey("params").BeginObject();
            {
                json.WriteKey("service").WriteString("captcha");
                json.WriteKey("key").WriteString(key.ImageKey);
                json.WriteKey("body").BeginObject();
                {
                    if (request.CaptchaRequest.TextResponse) {
                        json.WriteKey("rep").WriteString(request.CaptchaRequest.TextResponse);
                        json.WriteKey("captcha_type").WriteString("image");
                    } else {
                        json.WriteKey("captcha_type").WriteString("button");
                    }
                    json.WriteKey("captcha_result").WriteBool(captchaApiResult.ImageCheckOk);
                    json.WriteKey("random_captcha").WriteBool(false); // TODO: DEPRECATED, удалить после удаления в фурии
                    json.WriteKey("host_type").WriteString(ToString(request.HostType));
                    json.WriteKey("user_agent").WriteString(request.UserAgent());
                    json.WriteKey("ja3").WriteString(request.Ja3());
                    json.WriteKey("ja4").WriteString(request.Ja4());
                    json.WriteKey("p0f").WriteString(request.P0f());
                    json.WriteKey("hodor").WriteString(request.Hodor);
                    json.WriteKey("hodor_hash").WriteString(request.HodorHash);
                    json.WriteKey("referer").WriteString(request.Referer());
                    json.WriteKey("host").WriteString(request.Host);
                    json.WriteKey("req_type").WriteString(ToString(request.ReqType));
                    json.WriteKey("uid").WriteString(ToString(request.Uid));
                    json.WriteKey("yandexuid").WriteString(request.YandexUid);
                    json.WriteKey("icookie").WriteString(request.ICookie);
                    json.WriteKey("puid").WriteULongLong(request.LCookieUid);
                    json.WriteKey("spravka_addr").WriteString(ToString(request.SpravkaAddr));
                    json.WriteKey("user_addr").WriteString(ToString(request.UserAddr));
                    json.WriteKey("timestamp").WriteInt(TInstant::Now().Seconds());
                    json.WriteKey("has_valid_icookie").WriteBool(request.HasValidOldICookie);
                    json.WriteKey("has_valid_fuid").WriteBool(request.HasValidFuid);
                    json.WriteKey("has_valid_lcookie").WriteBool(request.HasValidLCookie);
                    json.WriteKey("has_valid_spravka").WriteBool(request.HasValidSpravka);
                    json.WriteKey("has_valid_spravka_hash").WriteBool(request.HasValidSpravkaHash);
                    json.WriteKey("unique_key").WriteString(request.UniqueKey);
                    json.WriteKey("is_api").WriteBool(ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService);

                    json.WriteKey("answer").BeginList(); // TODO: DEPRECATED, удалить после удаления в фурии
                    for (const auto& answer : captchaApiResult.Answers) {
                        json.WriteString(answer);
                    }
                    json.EndList();

                    if (captchaApiResult.SessionMetadata.IsMap()) {
                        json.WriteKey("session_metadata").WriteJsonValue(&captchaApiResult.SessionMetadata);
                    }

                    json.WriteKey("js_print");
                    request.CaptchaRequest.BrowserJsPrint.WriteToJson(json);

                    if (settings) {
                        json.WriteKey("sitekey").WriteString(settings->Getclient_key());
                        json.WriteKey("complexity").WriteInt(static_cast<int>(settings->Getcomplexity()));
                        json.WriteKey("is_api_yandex").WriteBool(settings->Getis_yandex_client());
                    } else {
                        json.WriteKey("sitekey").WriteString("");
                        json.WriteKey("complexity").WriteInt(0);
                        json.WriteKey("is_api_yandex").WriteBool(false);
                    }
                }
                json.EndObject();
            }
            json.EndObject();
        }
        json.EndObject();
        http_request.SetContent(requestString);

        auto measureDuration = MakeAtomicShared<TMeasureDuration>(timeStat);
        return FetchHttpDataAsync(&TFuryNehRequester::Instance(), options.Host, http_request, options.Timeout, options.Protocol)
            .Apply([measureDuration](const auto& future) mutable -> TErrorOr<TCaptchaFuryResult> {
                measureDuration.Drop();
                NNeh::TResponseRef response;
                if (TError err = future.GetValue().PutValueTo(response); err.Defined()) {
                    return TError(__LOCATION__ + yexception() << "Failed to send request (" << err->what() << ")");
                }
                if (response && !response->IsError()) {
                    auto furyParseResult = ParseFuryResult(response->Data);
                    TVector<EFuryCategory> categories;
                    if (TError err = furyParseResult.PutValueTo(categories); err.Defined()) {
                        return err;
                    }

                    TCaptchaFuryResult result;
                    for (const auto& category: categories) {
                        switch (category) {
                            case EFuryCategory::CaptchaRobot:
                                result.CheckOk = false;
                                break;
                            case EFuryCategory::DegradationMarket:
                                result.Degradation.Market = true;
                                break;
                            case EFuryCategory::DegradationWeb:
                                result.Degradation.Web = true;
                                break;
                            case EFuryCategory::DegradationUslugi:
                                result.Degradation.Uslugi = true;
                                break;
                            case EFuryCategory::DegradationAutoru:
                                result.Degradation.Autoru = true;
                                break;
                            default:
                                return TError(__LOCATION__ + yexception() << "Unknown EFuryCategory::" << category);
                        }
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
                                                              << "; " << response->Data.substr(0, LOG_ERROR_BYTES));
                }
            });
    }
} // namespace NAntiRobot
