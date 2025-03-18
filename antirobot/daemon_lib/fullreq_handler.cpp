#include "fullreq_handler.h"

#include "antirobot_cookie.h"
#include "eventlog_err.h"
#include "fullreq_info.h"
#include "host_ops.h"
#include "time_stats.h"
#include "unified_agent_log.h"

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/evp.h>

#include <contrib/libs/openssl/include/openssl/rand.h>

#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/string/printf.h>

#include <array>


namespace NAntiRobot {

namespace {

NDaemonLog::TCacherRecord CreateDaemonLogCacherRecord(const TRequestContext& rc);

NThreading::TFuture<TResponse> AddBalancerHeadersDummy(
    TResponse&& resp
) {
    resp.AddHeader(FORWARD_TO_USER_HEADER, resp.IsForwardedToUser());
    if (resp.IsForwardedToBalancer()) {
        resp.AddHeader("X-Yandex-Internal-Request", 0);
        resp.AddHeader("X-Yandex-Suspected-Robot", 0);

        resp.AddHeader("X-Yandex-Antirobot-Degradation", false);
        resp.AddHeader("X-Antirobot-Region-Id", 0);
    }
    resp.AddHeader("X-Yandex-EU-Request", 0);
    return NThreading::MakeFuture(std::move(resp));
}

NThreading::TFuture<TResponse> AddBalancerHeaders(
    TResponse&& resp,
    const TRequestContext& rc
) {
    const TRequest& req = *rc.Req.Get();

    resp.AddHeader(FORWARD_TO_USER_HEADER, resp.IsForwardedToUser());

    if (resp.IsForwardedToBalancer()) {


        if (!AtomicGet(rc.Env.AntirobotDisableExperimentsFlag.Enable) && req.HostType == HOST_WEB && !req.ExperimentsHeader.empty()) {
            TStringStream experimentHeader;
            for (const auto& expInfo : req.ExperimentsHeader) {
                if (!experimentHeader.empty()) {
                    experimentHeader << ';';
                }
                experimentHeader << ToString(expInfo.TestId) << ",0," << ToString(expInfo.Bucket);
            }

            resp.AddHeader("X-Antirobot-Experiments", experimentHeader.Str());
        }

        if (req.AntirobotCookieDirty) {
            const auto arCookieEnc = req.AntirobotCookie.Encrypt(rc.Env.YascKey);
            if (!arCookieEnc.empty()) {
                TStringStream valueStream;
                valueStream << ANTIROBOT_COOKIE_KEY << "=" << arCookieEnc;
                AddCookieSuffix(valueStream, GetCookiesDomainFromHost(rc.Req->Host), TInstant::Now() + TDuration::Days(30));
                valueStream << "; secure";
                resp.AddHeader(
                    "X-Antirobot-Set-Cookie",
                    valueStream.Str()
                );
            }
        }

        resp.AddHeader("X-Yandex-Internal-Request", req.UserAddr.IsYandex());
        resp.AddHeader("X-Yandex-Suspected-Robot", req.HasValidSpravka);

        if (req.UserAddr.IsSearchEngine()) {
            resp.AddHeader("X-Antirobot-Is-Crawler", ToString<ECrawler>(req.UserAddr.SearchEngine()));
        }

        if (
            rc.Suspiciousness > 0 &&
            ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(req.HostType, req.Tld).SuspiciousnessHeaderEnabled
        ) {
            resp.AddHeader("X-Antirobot-Suspiciousness-Y", Sprintf("%.1f", rc.Suspiciousness));
        }

        resp.AddHeader("X-Antirobot-Robotness-Y", Sprintf("%.1f", rc.RobotnessHeaderValue));
        resp.AddHeader("X-Yandex-Antirobot-Degradation", rc.Degradation);
        resp.AddHeader("X-Antirobot-Region-Id", req.UserAddr.CountryId());
        resp.AddHeader("X-Antirobot-Jws-Info", ToString(req.JwsState));
        if (req.HasYandexTrust) {
            resp.AddHeader("X-Antirobot-Yandex-Trust-Info", ToString(req.YandexTrustState));
        }
        if (!req.Hodor.empty()) {
            resp.AddHeader("X-Antirobot-Hodor", req.Hodor);
        }
        if (!req.HodorHash.empty()) {
            resp.AddHeader("X-Antirobot-Hodor-Hash", req.HodorHash);
        }
    }
    resp.AddHeader("X-Yandex-EU-Request", req.UserAddr.IsEuropean());

    if (rc.BanSourceIp) {
        resp.AddHeader("X-Antirobot-Ban-Source-Ip", true);
    }

    if (req.UserAddr.IsYandex()) {
        rc.Env.ServiceCounters.Inc(req, TEnv::EServiceCounter::YandexRequests);
    }
    if (req.UserAddr.IsSearchEngine()) {
        rc.Env.ServiceCounters.Inc(req, TEnv::EServiceCounter::CrawlerRequests);
    }

    NDaemonLog::TCacherRecord rec = CreateDaemonLogCacherRecord(rc);
    ANTIROBOT_DAEMON_CONFIG.CacherDaemonLog->WriteLogRecord(rec);

    return NThreading::MakeFuture(std::move(resp));
}


template <typename F>
std::invoke_result_t<F> HandleExceptions(TRequestContext* rc, F f) {
    try {
        return f();
    } catch (const TFullReqInfo::TBadRequest&) {
        NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(NAntirobotEvClass::TBadRequest(CurrentExceptionMessage()));
        ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);
        rc->Env.ServerExceptionsStats.Inc(EServerExceptionCounter::BadRequests);
        return AddBalancerHeadersDummy(TResponse::ToUser(HTTP_BAD_REQUEST));
    } catch (const TFullReqInfo::TBadExpect&) {
        rc->Env.ServerExceptionsStats.Inc(EServerExceptionCounter::BadExpects);
        return AddBalancerHeadersDummy(TResponse::ToUser(HTTP_EXPECTATION_FAILED));
    } catch (const TFullReqInfo::TInvalidPartnerRequest&) {
        rc->Env.ServerExceptionsStats.Inc(EServerExceptionCounter::InvalidPartnerRequests);
        return AddBalancerHeadersDummy(TResponse::ToUser(HTTP_BAD_REQUEST));
    } catch (const TFullReqInfo::TUidCreationFailure&) {
        EVLOG_MSG << EVLOG_ERROR << ' ' << CurrentExceptionMessage();
        rc->Env.ServerExceptionsStats.Inc(EServerExceptionCounter::UidCreationFailures);
        return AddBalancerHeadersDummy(TResponse::ToBalancer(HTTP_BAD_REQUEST));
    } catch (...) {
        EVLOG_MSG << EVLOG_ERROR << rc->Req->RequesterAddr << ' ' << CurrentExceptionMessage();
        rc->Env.ServerExceptionsStats.Inc(EServerExceptionCounter::OtherExceptions);
        return AddBalancerHeadersDummy(TResponse::ToBalancer(HTTP_OK));
    }
}

NDaemonLog::TCacherRecord CreateDaemonLogCacherRecord(const TRequestContext& rc) {
    const TRequest& req = *rc.Req;

    static const TString DASH = "-";

    const TInstant& startTime = rc.FirstArrivalTime;

    ECaptchaRedirectType redirectType = rc.RedirectType;
    TStringBuf severity = redirectType != ECaptchaRedirectType::NoRedirect ? TStringBuf("ENEMY") : TStringBuf("NEUTRAL");
    TIsBlocked::TBlockState blockState = rc.Env.IsBlocked.GetByService(req.HostType)->GetBlockState(rc);
    if (blockState.IsBlocked) {
        severity = TStringBuf("BLOCKED");
    }

    const TStringBuf& reqId = !req.ServiceReqId.empty() ? req.ServiceReqId : DASH;

    TStringBuf reqUrl;
    TStringBuilder maskedReqUrl;

    TStringBuf referer;
    TStringBuilder maskedReferer;

    if (ANTIROBOT_DAEMON_CONFIG.JsonConfig[req.HostType].CgiSecrets.empty()) {
        reqUrl = req.RequestString;
        referer = req.Referer();
    } else {
        maskedReqUrl.reserve(req.RequestString.size());
        WriteMaskedUrl(req.HostType, req.RequestString, maskedReqUrl.Out);
        reqUrl = maskedReqUrl;

        const TStringBuf unmaskedReferer = req.Referer();
        maskedReferer.reserve(unmaskedReferer.size());
        WriteMaskedUrl(req.HostType, unmaskedReferer, maskedReferer.Out);
        referer = maskedReferer;
    }

    reqUrl.Trunc(
        reqUrl.StartsWith("/clck/") ?
            MAX_REQ_URL_CLCK_SIZE :
            ANTIROBOT_DAEMON_CONFIG.JsonConfig[req.HostType].MaxReqUrlSize
    );
    referer.Trunc(MAX_REFERER_URL_SIZE);

    const auto& ua = req.UserAgent();

    const auto& groupName = rc.Env.ReqGroupClassifier.GroupName(req.HostType, req.ReqGroup);

    TVector<TBinnedCbbRuleKey> matchedCbbRuleList = rc.MatchedRules->Captcha;
    TVector<TBinnedCbbRuleKey> markingCbbRuleList = rc.MatchedRules->Mark;
    Copy(
        rc.MatchedRules->MarkLogOnly.begin(), rc.MatchedRules->MarkLogOnly.end(),
        std::back_inserter(markingCbbRuleList)
    );

    NDaemonLog::TCacherRecord rec;

    rec.set_unique_key(req.UniqueKey);
    rec.set_timestamp(startTime.MicroSeconds());
    rec.set_verdict(TString(severity));
    rec.set_req_type(ToString(req.HostType) + '-' + ToString(req.ReqType));
    rec.set_balancer_ip(NiceAddrStr(req.RequesterAddr));
    rec.set_spravka_ip(ToString(req.SpravkaAddr));
    rec.set_ident_type(ToString(req.Uid));
    rec.set_user_ip(ToString(req.UserAddr));
    rec.set_req_id(TString(reqId));
    rec.set_random_captcha(rc.IsTraining);
    rec.set_partner_ip(req.IsPartnerRequest() ? req.PartnerAddr.ToString() : DASH);
    rec.set_yandexuid(!req.YandexUid.empty() ? TString(req.YandexUid) : DASH);
    rec.set_cacher_host(ShortHostName());
    rec.set_icookie(!req.ICookie.empty() ? ToString(req.ICookie) : DASH);
    rec.set_service(ToString(req.HostType));
    rec.set_may_ban(req.MayBanFor());
    rec.set_can_show_captcha(req.CanShowCaptcha());
    rec.set_req_url(reqUrl.data(), reqUrl.size());
    rec.set_ip_list(JoinSeq(",", req.UserAddr.IpList()));
    rec.set_block_reason(blockState.Reason);
    rec.set_cacher_blocked(rc.WasBlocked);
    rec.set_cacher_block_reason(rc.BlockReason);

    if (const auto ja3 = req.Ja3(); !ja3.empty()) {
        rec.set_ja3(ja3.Data(), ja3.Size());
    }

    rec.set_robotness(rc.RobotnessHeaderValue);
    rec.set_host({req.Host.data(), req.Host.size()});
    rec.set_suspiciousness(rc.Suspiciousness);
    rec.set_user_agent({ua.data(), ua.size()});
    rec.set_referer(referer.data(), referer.size());
    rec.set_cbb_whitelist(rc.CbbWhitelist);

    if (const auto ja4 = req.Ja4(); !ja4.empty()) {
        rec.set_ja4(ja4.Data(), ja4.Size());
    }

    rec.set_req_group(groupName.Data(), groupName.Size());
    *rec.mutable_cbb_ban_rules() = ConvertToProtoStrings(matchedCbbRuleList);
    *rec.mutable_cbb_mark_rules() = ConvertToProtoStrings(markingCbbRuleList);
    rec.set_spravka_ignored(req.SpravkaIgnored);
    rec.set_catboost_whitelist(rc.CatboostWhitelist);
    *rec.mutable_cbb_checkbox_blacklist_rules() = ConvertToProtoStrings(rc.MatchedRules->CheckboxBlacklist);
    *rec.mutable_cbb_can_show_captcha_rules() = ConvertToProtoStrings(rc.MatchedRules->CanShowCaptcha);
    rec.set_degradation(rc.Degradation);

    if (const auto p0f = req.P0f(); !p0f.empty()) {
        rec.set_p0f(p0f.Data(), p0f.Size());
    }

    *rec.mutable_experiments_test_id() = ConvertExpHeaderToProto(req.ExperimentsHeader);

    if (const auto hodor = req.Hodor; !hodor.empty()) {
        rec.set_hodor(hodor.Data(), hodor.Size());
    }

    if (const auto hodorHash = req.HodorHash; !hodorHash.empty()) {
        rec.set_hodor_hash(hodorHash.Data(), hodorHash.Size());
    }

    rec.set_lcookie(req.LCookieUid);
    rec.set_jws_state(ToString(req.JwsState));
    rec.set_cacher_formula_result(rc.CacherFormulaResult);
    rec.set_ban_source_ip(rc.BanSourceIp);
    rec.set_mini_geobase_mask(req.UserAddr.MiniGeobaseMask());
    rec.set_jws_hash(HexEncode(TStringBuf(req.JwsHash.begin(), req.JwsHash.end())));
    rec.set_yandex_trust_state(ToString(req.YandexTrustState));
    rec.set_valid_spravka_hash(req.HasValidSpravkaHash);
    rec.set_valid_autoru_tamper(req.HasValidAutoRuTamper);
    rec.set_ban_fw_source_ip(rc.BanFWSourceIp);

    if (const auto uuid = req.Uuid; !uuid.empty()) {
        rec.set_uuid(uuid.Data(), uuid.Size());
    }

    return rec;
}

} // anonymous namespace


NThreading::TFuture<TResponse> TFullreqHandler::operator()(TRequestContext& rc) {
    TMeasureDuration dm(rc.Env.TimeStatsHandle);
    auto measureDuration = MakeAtomicShared<TMeasureDuration>(rc.Env.TimeStatsHandleByService.Stats.GetByService(HOST_OTHER));

    return HandleExceptions(&rc, [this, &rc, measureDuration] {
        auto fullreqContext = MakeAtomicShared<TRequestContext>(ExtractWrappedRequest(rc));
        rc.Env.SearchEngineRecognizer.ProcessRequest(*fullreqContext->Req);

        if (fullreqContext->Req->HasUnknownServiceHeader) {
            fullreqContext->Env.IncreaseUnknownServiceHeaders();
        }

        if (fullreqContext->Req->HasValidAutoRuTamper) {
            fullreqContext->Env.Counters.Inc(TEnv::ECounter::ValidAutoRuTampers);
        }

        return Handler(*fullreqContext).Apply(
            [rc = fullreqContext, measureDuration]
            (const NThreading::TFuture<TResponse>& future) mutable
                -> NThreading::TFuture<TResponse>
            {
                return HandleExceptions(&*rc, [rc, &measureDuration, &future] {
                    TResponse resp = future.GetValue();
                    measureDuration->ResetStats(rc->Env.TimeStatsHandleByService.Stats.GetByService(rc->Req->HostType));
                    measureDuration.Drop();
                    return AddBalancerHeaders(std::move(resp), *rc);
                });
            }
        );
    });
}


} // namespace NAntiRobot
