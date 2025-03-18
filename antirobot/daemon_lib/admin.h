#pragma once
#include "conditional_handler.h"

#include <antirobot/lib/antirobot_response.h>

#include <util/generic/strbuf.h>

namespace NAntiRobot {

struct TRequestContext;

extern const TStringBuf ADMIN_ACTION;

NThreading::TFuture<TResponse> HandlePing(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleVer(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleReloadData(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleReloadLKeys(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleAmnesty(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleShutdown(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleMemStats(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleUniStats(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleUniStatsLW(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleBlockStats(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleDumpCfg(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleLogLevel(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleDumpCache(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleSetCbb(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleWorkMode(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleGetSpravka(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleUnknownAdminAction(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleForceBlock(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleGetCbbRules(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleStartChinaRedirect(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleStopChinaRedirect(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleStartChinaUnauthorized(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleStopChinaUnauthorized(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleEnableNeverBlock(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleDisableNeverBlock(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleEnableNeverBan(TRequestContext& rc);
NThreading::TFuture<TResponse> HandleDisableNeverBan(TRequestContext& rc);

struct TIsFromLocalHost {
    bool operator()(TRequestContext& rc);
};

template<class THandler>
TConditionalHandler<TIsFromLocalHost, THandler> FromLocalHostOnly(THandler handler) {
    return ConditionalHandler(TIsFromLocalHost()).IfTrue(handler);
}

}
