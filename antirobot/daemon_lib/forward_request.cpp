#include "forward_request.h"

#include "cache_sync.h"
#include "environment.h"
#include "eventlog_err.h"
#include "instance_hashing.h"
#include "request_context.h"

#include <antirobot/idl/cache_sync.pb.h>
#include <antirobot/lib/addr.h>
#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/host_addr.h>
#include <antirobot/lib/http_helpers.h>

#include <library/cpp/neh/multiclient.h>
#include <library/cpp/threading/future/future.h>

#include <utility>


namespace NAntiRobot {


namespace {
    NCacheSyncProto::TMessage CreateCacheSyncMessage(const TRequestContext& rc) {
        NCacheSyncProto::TMessage cacheSyncMessage;

        auto* pbRequest = cacheSyncMessage.MutableRequest();
        rc.Req->SerializeTo(*pbRequest);
        pbRequest->SetIsTraining(rc.IsTraining);
        pbRequest->SetCacherHost(ShortHostName());
        pbRequest->SetRedirectType(static_cast<int>(rc.RedirectType));
        pbRequest->SetFirstArrivalTime(rc.FirstArrivalTime.MicroSeconds());
        pbRequest->SetWasBlocked(rc.WasBlocked);
        pbRequest->SetBlockReason(rc.BlockReason);
        pbRequest->SetRobotnessHeaderValue(rc.RobotnessHeaderValue);
        pbRequest->SetSuspiciousness(rc.Suspiciousness);
        pbRequest->SetCbbWhitelist(rc.CbbWhitelist);
        pbRequest->SetSpravkaIgnored(rc.Req->SpravkaIgnored);
        pbRequest->SetSuspiciousBan(rc.SuspiciousBan);
        pbRequest->SetCatboostWhitelist(rc.CatboostWhitelist);
        pbRequest->SetDegradation(rc.Degradation);
        pbRequest->SetUniqueKey(rc.Req->UniqueKey);
        *pbRequest->MutableExperimentsTestId() = ConvertExpHeaderToProto(rc.Req->ExperimentsHeader);
        pbRequest->SetCacherFormulaResult(rc.CacherFormulaResult);
        pbRequest->SetBanSourceIp(rc.BanSourceIp);
        pbRequest->SetCatboostBan(rc.CatboostBan);
        pbRequest->SetApiAutoVersionBan(rc.ApiAutoVersionBan);
        pbRequest->SetBanFWSourceIp(rc.BanFWSourceIp);

        WriteBlockSyncResponse(*rc.Env.Blocker, rc.Req->Uid, cacheSyncMessage.MutableBlocks());
        rc.Env.Robots->WriteSyncResponse(rc.Req->Uid, {}, cacheSyncMessage.MutableBans());

        return cacheSyncMessage;
    }
}


void ForwardRequestAsync(const TRequestContext& rc, const TString& location) {
    try {
        NCacheSyncProto::TMessage cacheSyncMessage = CreateCacheSyncMessage(rc);

        auto request = HttpPost("", location).SetContent(cacheSyncMessage.SerializeAsString());
        const auto future = rc.Env.BackendSender.Send(
            &rc.Env.CustomHashingMap,
            rc.Req->UserAddr,
            std::move(request),
            rc.Env.DisablingFlags.IsStopDiscoveryForAll()
        );

        future.Subscribe([rc] (const NThreading::TFuture<TErrorOr<TString>>& future) {
            try {
                TString responseStr;
                if (TError err = future.GetValue().PutValueTo(responseStr); err.Defined()) {
                    EVLOG_MSG << EVLOG_ERROR << *rc.Req << "Forward failure " << err->what();
                    rc.Env.RequestProcessingQueue->Add(rc);
                    return;
                }

                NCacheSyncProto::TMessage response;
                if (!response.ParseFromString(responseStr)) {
                    EVLOG_MSG << EVLOG_ERROR << *rc.Req << "Forward failure Failed to parse response message";
                    rc.Env.RequestProcessingQueue->Add(rc);
                    return;
                }

                Y_UNUSED(rc.Env.ProcessorResponseApplyQueue->AddFunc([
                    blocker = rc.Env.Blocker,
                    robots = rc.Env.Robots,
                    response = std::move(response)
                ] () {
                    ApplyBlockSyncRequest(response.GetBlocks(), &*blocker);
                    robots->ApplySyncRequest(response.GetBans());
                }));
            } catch (...) {
                EVLOG_MSG << EVLOG_ERROR << *rc.Req << "Forward failure " << CurrentExceptionMessage();
                rc.Env.RequestProcessingQueue->Add(rc);
            }
        });
    } catch (...) {
        EVLOG_MSG << EVLOG_ERROR << *rc.Req << "Forward failure " << CurrentExceptionMessage();
        rc.Env.RequestProcessingQueue->Add(rc);
    }
}


void ForwardCaptchaInputAsync(const TCaptchaInputInfo& captchaInputInfo, const TString& location) {
    const auto& rc = captchaInputInfo.Context;
    const bool isCorrect = captchaInputInfo.IsCorrect;

    try {
        NCacheSyncProto::TCaptchaInput message;
        rc.Req->SerializeTo(*message.MutableRequest());
        message.MutableRequest()->SetSpravkaIgnored(rc.Req->SpravkaIgnored);
        message.SetIsCorrect(isCorrect);

        auto request = HttpPost("", location).SetContent(message.SerializeAsString());
        const auto future = rc.Env.BackendSender.Send(
            &rc.Env.CustomHashingMap,
            rc.Req->UserAddr,
            std::move(request),
            rc.Env.DisablingFlags.IsStopDiscoveryForAll()
        );

        future.Subscribe([rc = rc, isCorrect](const NThreading::TFuture<TErrorOr<TString>>& future) mutable {
            try {
                TString result;
                if (TError err = future.GetValue().PutValueTo(result); err.Defined()) {
                    EVLOG_MSG << EVLOG_ERROR << *rc.Req << "Captcha input forward failure " << err->what();
                    rc.Env.CaptchaInputProcessingQueue->Add({rc, isCorrect});
                }
            } catch (...) {
                EVLOG_MSG << EVLOG_ERROR << *rc.Req << "Captcha input forward failure " << CurrentExceptionMessage();
                rc.Env.CaptchaInputProcessingQueue->Add({rc, isCorrect});
            }
        });
    } catch (...) {
        EVLOG_MSG << EVLOG_ERROR << *rc.Req << "Captcha input forward failure " << CurrentExceptionMessage();
        rc.Env.CaptchaInputProcessingQueue->Add({rc, isCorrect});
    }
}


} // namespace NAntiRobot
