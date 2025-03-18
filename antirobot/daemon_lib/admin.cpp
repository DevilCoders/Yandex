#include "admin.h"

#include "blocker.h"
#include "captcha_descriptor.h"
#include "captcha_key.h"
#include "environment.h"
#include "eventlog_err.h"
#include "night_check.h"
#include "request_context.h"
#include "request_params.h"
#include "robot_set.h"
#include "unified_agent_log.h"
#include "user_reply.h"

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/enum.h>
#include <antirobot/lib/json.h>
#include <antirobot/lib/spravka.h>
#include <antirobot/lib/stats_writer.h>

#include <library/cpp/iterator/concatenate.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/datetime/base.h>
#include <util/generic/cast.h>
#include <util/network/address.h>
#include <util/stream/str.h>
#include <util/string/escape.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/string/type.h>

#include <array>

namespace NAntiRobot {

const TStringBuf ADMIN_ACTION("action");

namespace {
    const TResponse TO_USER_OK = TResponse::ToUser(HTTP_OK).SetContent("OK\r\n", "text/plain"_sb);

    constexpr std::array<EHostType, 8> DEAD_HOST_TYPES = {
        HOST_AUTO,
        HOST_HILIGHTER,
        HOST_KPAPI,
        HOST_MARKETAPI_BLUE,
        HOST_MARKETRED,
        HOST_TECH,
        HOST_SLOVARI,
        HOST_RCA,
    };

    NThreading::TFuture<TResponse> HandleStatsImpl(TRequestContext& rc) {
        try {
            TStringStream stats;

            {
                TStatsWriter statsOutput(&stats);
                if (rc.Req->CgiParams.Get("service"_sb).empty()) {
                    rc.Env.PrintStats(statsOutput);
                }
            }

            return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(stats.Str()));
        } catch (yexception&) {
            return NThreading::MakeFuture(TResponse::ToUser(HTTP_BAD_REQUEST).SetContent(CurrentExceptionMessage()));
        }
    }

    NThreading::TFuture<TResponse> HandleStatsLWImpl(TRequestContext& rc) {
        try {
            TStringStream stats;
            {
                TStatsWriter statsOutput(&stats);

                const TStringBuf reqService = rc.Req->CgiParams.Get(TStringBuf("service"));
                if (reqService.empty()) {
                    for (int i = HOST_OTHER; i < HOST_NUMTYPES; ++i) {
                        const auto service = static_cast<EHostType>(i);
                        const auto servicePrefix = TStringBuilder() << "service_type=" << service << ";";
                        const auto stat = HostTypeToStatType(service);

                        if (IsIn(DEAD_HOST_TYPES, service)) {
                            // экономим сигналы на этих сервисах
                            continue;
                        }

                        auto serviceStats = statsOutput.WithPrefix(servicePrefix);
                        rc.Env.PrintStatsLW(serviceStats, stat);
                    }
                }
            }
            return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(stats.Str()));
        } catch (yexception&) {
            return NThreading::MakeFuture(TResponse::ToUser(HTTP_BAD_REQUEST).SetContent(CurrentExceptionMessage()));
        }
    }
}

NThreading::TFuture<TResponse> HandlePing(TRequestContext& rc) {
    return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).AddHeader("RS-Weight", rc.Env.RSWeight.load()).SetContent("pong"));
}

NThreading::TFuture<TResponse> HandleVer(TRequestContext& /*rc*/) {
    return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(PROGRAM_VERSION));
}

bool TIsFromLocalHost::operator ()(TRequestContext& rc) {
    static const TStringBuf LOCALHOST_IP_4 = "127.0.0.1";
    static const TStringBuf LOCALHOST_IP_6 = "::1";

    return EqualToOneOf(rc.Req->RequesterAddr, LOCALHOST_IP_4, LOCALHOST_IP_6);
}

NThreading::TFuture<TResponse> HandleUniStats(TRequestContext& rc) {
    return HandleStatsImpl(rc);
}

NThreading::TFuture<TResponse> HandleUniStatsLW(TRequestContext& rc) {
    return HandleStatsLWImpl(rc);
}

NThreading::TFuture<TResponse> HandleReloadData(TRequestContext& rc) {
    const auto startTime = TInstant::Now();

    rc.Env.LoadIpLists();
    EVLOG_MSG << "HandleReloadData after LoadIpLists duration: " << (TInstant::Now() - startTime);

    rc.Env.LoadLKeychain();
    EVLOG_MSG << "HandleReloadData after LoadLKeychain duration: " << (TInstant::Now() - startTime);

    rc.Env.LoadKeyRing();
    EVLOG_MSG << "HandleReloadData after LoadKeyRing duration: " << (TInstant::Now() - startTime);

    rc.Env.ReloadableData.GeoChecker.Set(TGeoChecker(ANTIROBOT_DAEMON_CONFIG.GeodataBinPath));
    EVLOG_MSG << "HandleReloadData after GeoChecker duration: " << (TInstant::Now() - startTime);

    return NThreading::MakeFuture(TO_USER_OK);
}

NThreading::TFuture<TResponse> HandleReloadLKeys(TRequestContext& rc) {
    rc.Env.LoadLKeychain();
    return NThreading::MakeFuture(TO_USER_OK);
}

NThreading::TFuture<TResponse> HandleAmnesty(TRequestContext& rc) {
    EVLOG_MSG << EVLOG_ERROR << "Perform amnesty";

    if (const auto hostTypeStrIt = rc.Req->CgiParams.Find("service"_sb); hostTypeStrIt != rc.Req->CgiParams.end()) {
        const auto& hostTypeStr = hostTypeStrIt->second;
        EHostType hostType;

        if (!TryFromString(hostTypeStr, hostType)) {
            auto message = TString::Join("Service '", hostTypeStr, "' not found");
            return NThreading::MakeFuture(TResponse::ToUser(HTTP_NOT_FOUND).SetContent(message));
        }

        rc.Env.Robots->Clear(hostType);
    } else {
        rc.Env.Robots->Clear();
    }

    rc.Env.Robots->LockedSave(*rc.Env.RobotUidsDumpFile.Access());

    return NThreading::MakeFuture(TO_USER_OK);
}

NThreading::TFuture<TResponse> HandleShutdown(TRequestContext& rc) {
    for (auto& server : rc.Env.Servers) {
        server->Shutdown();
    }
    EVLOG_MSG << EVLOG_ERROR << "Shutdown";
    return NThreading::MakeFuture(TO_USER_OK);
}

NThreading::TFuture<TResponse> HandleMemStats(TRequestContext& rc) {
    TStringStream statsOutput;
    rc.Env.UserBase.PrintMemStats(statsOutput);
    statsOutput << "num robots: " << rc.Env.Robots->Size() << Endl;
    statsOutput << "blocked ips: " << rc.Env.Blocker->GetCopy()->size() << Endl;

    return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(statsOutput.Str()));
}

NThreading::TFuture<TResponse> HandleBlockStats(TRequestContext& rc) {
    TStringStream out;

    out << "<?xml version=\"1.0\"?>"
        << "<blocklist>\n";

    for (const TBlockRecord& rec : *rc.Env.Blocker->GetCopy()) {
        out << R"(<host ip=")" << rec.Addr <<  R"(" )"
            << R"(uid=")" << rec.Uid<<  R"(" )"
            << R"(category=")" << rec.Category <<  R"(" )"
            << R"(status=")" << int(rec.Status) <<  R"(" )"
            << R"(expire_time=")" << rec.ExpireTime + GetMskAndUtcTimeDiff() <<  R"(" )"
            << R"(description=")" << rec.Description <<  R"(" )"
            << "/>\n";
    }

    out << "</blocklist>";

    return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(out.Str()));
}

NThreading::TFuture<TResponse> HandleDumpCfg(TRequestContext& rc) {
    const TString& valueStr = rc.Req->CgiParams.Get(TStringBuf("service"));
    EHostType service = HOST_OTHER;
    if (!valueStr.empty()) {
        if (!TryFromString(valueStr, service)) {
            TString message = TString::Join("Unknown service: '", valueStr, "'");
            return NThreading::MakeFuture(TResponse::ToUser(HTTP_NOT_FOUND).SetContent(message));
        }
    }
    TStringStream cfgOutput;
    ANTIROBOT_DAEMON_CONFIG.Dump(cfgOutput, service);

    return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(cfgOutput.Str()));
}

NThreading::TFuture<TResponse> HandleLogLevel(TRequestContext& rc) {
    const TString& valueStr = rc.Req->CgiParams.Get(TStringBuf("value"));

    if (!valueStr.empty()) {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LogLevel = FromString<ui32>(valueStr);
        return NThreading::MakeFuture(TO_USER_OK);
    } else {
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_BAD_REQUEST));
    }
}

NThreading::TFuture<TResponse> HandleDumpCache(TRequestContext& rc) {
    TStringStream cacheOutput;
    rc.Env.UserBase.DumpCache(cacheOutput);

    return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(cacheOutput.Str()));
}

NThreading::TFuture<TResponse> HandleWorkMode(TRequestContext& rc) {
    const TString& valueStr = rc.Req->CgiParams.Get(TStringBuf("value"));
    EWorkMode workMode = FromString<EWorkMode>(valueStr);
    ANTIROBOT_DAEMON_CONFIG_MUTABLE.WorkMode = workMode;
    EVLOG_MSG << EVLOG_ERROR << "Set work mode " << workMode << '(' << valueStr << ')';
    return NThreading::MakeFuture(TO_USER_OK);
}

NThreading::TFuture<TResponse> HandleGetSpravka(TRequestContext& rc) {
    size_t count = FromStringWithDefault<size_t>(rc.Req->CgiParams.Get("count"));
    if (0 == count) {
        count = 1;
    }

    TSpravka::TDegradation degradation;
    const TString& valueStr = rc.Req->CgiParams.Get("degradation"_sb);
    if (valueStr == "web") {
        degradation.Web = true;
    } else if (valueStr == "market") {
        degradation.Market = true;
    } else if (valueStr == "uslugi") {
        degradation.Uslugi = true;
    } else if (valueStr == "autoru") {
        degradation.Autoru = true;
    }

    TAddr addr(rc.Req->RequesterAddr);
    if (!rc.Env.ReloadableData.SpecialIps.Get()->Contains(addr)) {
        EVLOG_MSG << EVLOG_WARNING << "Get spravka request declined: count=" << count << ", requester=" << addr.ToString();
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_FORBIDDEN));
    }

    const TStringBuf spravkaDomain = rc.Req->CgiParams.Get(TStringBuf("domain"));
    TStringStream spravkaOutput;
    for (size_t i = 0; i < count; i++) {
        spravkaOutput << TSpravka::Generate(addr, spravkaDomain, degradation).ToString() << Endl;
    }
    EVLOG_MSG << EVLOG_ERROR << "Get spravka request: count=" << count << ", requester=" << addr.ToString();
    return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(spravkaOutput.Str()));
}

NThreading::TFuture<TResponse> HandleUnknownAdminAction(TRequestContext& rc) {
    TString str;
    TStringOutput stream(str);

    stream << "Bad admin command: /admin?" << rc.Req->CgiParams.Print() << Endl;
    EVLOG_MSG << EVLOG_ERROR << str;
    Cerr << str << Endl;

    auto message = TString::Join("Unknown action ", rc.Req->CgiParams.Get(ADMIN_ACTION));
    return NThreading::MakeFuture(TResponse::ToUser(HTTP_BAD_REQUEST).SetContent(message, TStringBuf("text/plain")));
}

NThreading::TFuture<TResponse> HandleForceBlock(TRequestContext& rc) {
    TAddr ip(rc.Req->CgiParams.Get(TStringBuf("ip")));
    if (!ip.Valid()) {
        const auto message = "CGI param 'ip' isn't present or invalid";
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_BAD_REQUEST).SetContent(message));
    }

    auto blockDuration = TDuration::Seconds(30);
    TryFromString(rc.Req->CgiParams.Get(TStringBuf("duration")), blockDuration);

    TBlockRecord blockRecord = {
        TUid::FromAddrOrSubnet(ip),
        BC_UNDEFINED,
        ip,
        "",
        EBlockStatus::Block,
        TInstant::Now() + blockDuration,
        "Force block from admin",
    };

    TVector<TFuture<void>> allBlocks;
    for (const auto category: AllBlockCategories()) {
        blockRecord.Category = category;
        auto blockResult = rc.Env.Blocker->AddForcedBlock(blockRecord);
        allBlocks.push_back(blockResult.IgnoreResult());
    }
    NThreading::WaitExceptionOrAll(allBlocks).Wait();

    return NThreading::MakeFuture(TO_USER_OK);
}

NThreading::TFuture<TResponse> HandleGetCbbRules(TRequestContext& rc) {
    try {
        const TStringBuf reqService = rc.Req->CgiParams.Get(TStringBuf("service"));
        EHostType service = HOST_OTHER;
        if (!TryFromString(reqService, service)) {
            auto message = TString::Join("Service '", reqService, "' not found");
            return NThreading::MakeFuture(TResponse::ToUser(HTTP_NOT_FOUND).SetContent(message));
        }
        // TString retval = JoinSeq("\n", rc.Env.ManagedBlockerByHosts.GetByService(service).Get()->GetCorrectTxtRules());
        TString retval = "unimpelemented";

        return NThreading::MakeFuture(TResponse::ToUser(HTTP_OK).SetContent(retval));
    } catch (...) {
        return NThreading::MakeFuture(TResponse::ToUser(HTTP_BAD_REQUEST).SetContent(CurrentExceptionMessage()));
    }
}

NThreading::TFuture<TResponse> HandleStartChinaRedirect(TRequestContext& rc) {
    rc.Env.EnableChinaRedirect();
    EVLOG_MSG << "China redirect enabled";
    return NThreading::MakeFuture(TO_USER_OK);
}

NThreading::TFuture<TResponse> HandleStopChinaRedirect(TRequestContext& rc) {
    rc.Env.DisableChinaRedirect();
    EVLOG_MSG << "China redirect disabled";
    return NThreading::MakeFuture(TO_USER_OK);
}

NThreading::TFuture<TResponse> HandleStartChinaUnauthorized(TRequestContext& rc) {
    rc.Env.EnableChinaUnauthorized();
    EVLOG_MSG << "China unauthorized enabled";
    return NThreading::MakeFuture(TO_USER_OK);
}

NThreading::TFuture<TResponse> HandleStopChinaUnauthorized(TRequestContext& rc) {
    rc.Env.DisableChinaUnauthorized();
    EVLOG_MSG << "China unauthorized disabled";
    return NThreading::MakeFuture(TO_USER_OK);
}

}
