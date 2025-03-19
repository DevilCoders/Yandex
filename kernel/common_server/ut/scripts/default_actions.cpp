#include "default_actions.h"
#include <kernel/common_server/rt_background/state.h>
#include <kernel/common_server/rt_background/manager.h>
#include <kernel/common_server/rt_background/tasks/db/object.h>
#include <kernel/common_server/library/tvm_services/abstract/request/direct.h>

namespace NServerTest {

    void TRebuildCaches::DoExecute(ITestContext& /*context*/) {
        SendGlobalMessage<NFrontend::TCacheRefreshMessage>();
    }

    void TWaitCommonConditionedActionImpl::DoExecute(ITestContext& context) {
        Init(context);

        const TInstant deadline = Now() + GetTimeout();
        bool startCorrectSerial = false;
        do {
            if (Check(context)) {
                if (FullWaitingForNegative && !GetExpectOK()) {
                    UNIT_ASSERT(!startCorrectSerial);
                } else {
                    UNIT_ASSERT(GetExpectOK());
                    return;
                }
            } else if (!GetExpectOK()) {
                startCorrectSerial = true;
            }
            Sleep(TDuration::Seconds(1));
        } while (Now() < deadline);
        ERROR_LOG << "Cannot waiting more for condition" << Endl;
        if (GetExpectOK()) {
            CHECK_WITH_LOG(false);
        } else {
            CHECK_WITH_LOG(startCorrectSerial);
        }
    }

    void TAPIAction::DoExecute(ITestContext& context) {
        auto request = context.CreateRequest(GetUri(context));
        {
            auto postData = GetPostData(context);
            if (postData) {
                request.SetPostData(postData);
            }
        }

        auto reply = context.AskServer(request, Now() + TDuration::Seconds(3));
        if (GetExpectOK()) {
            UNIT_ASSERT(!reply.IsNull());
        }
        CheckReply(context, reply);
        ProcessReply(context, reply);
    }

    void TCommonChecker::DoExecute(ITestContext& context) {
        SendGlobalMessage<NFrontend::TCacheRefreshMessage>();
        Checker(context);
    }

    bool TWaitRTRobotExecution::Check(ITestContext& context) const {
        auto& server = context.GetServer<IBaseServer>();
        TRTBackgroundProcessStateContainer processState;
        TRTBackgroundProcessStateContainer::TStates states;
        CHECK_WITH_LOG(server.GetRTBackgroundManager()->RefreshState(states, RobotName));
        processState = server.GetRTBackgroundManager()->GetProcessState(states, RobotName);
        ExecInstants.emplace(processState.GetLastExecution());
        if (ExecInstants.size() >= 3) {
            auto tasksManager = server.GetRTBackgroundManager()->GetTasksManager("test");
            if (!tasksManager) {
                return true;
            }
            TVector<NCS::NBackground::TRTQueueTaskContainer> tasks;
            CHECK_WITH_LOG(tasksManager->RestoreOwnerTasks("", RobotName, tasks));
            TMaybe<ui32> maxSequenceId;
            TMaybe<ui32> minSequenceId;
            for (auto&& i : tasks) {
                if (i->GetOwnerId() != RobotName) {
                    continue;
                }
                auto task = i.GetVerifiedPtrAs<NCS::NBackground::TDBBackgroundTask>();
                if (!maxSequenceId || *maxSequenceId < task->GetSequenceTaskId()) {
                    maxSequenceId = task->GetSequenceTaskId();
                }
                if (!minSequenceId || *minSequenceId > task->GetSequenceTaskId()) {
                    minSequenceId = task->GetSequenceTaskId();
                }
            }
            if (!BorderSequenceId && !maxSequenceId) {
                return true;
            } else if (!BorderSequenceId) {
                BorderSequenceId = *maxSequenceId;
            } else if (!maxSequenceId) {
                return true;
            } else if (*BorderSequenceId < *minSequenceId) {
                return true;
            }
        }
        return false;
    }

    void TRTRobotEnabled::DoExecute(ITestContext& context) {
        auto& server = context.GetServer<IBaseServer>();
        CHECK_WITH_LOG(server.GetRTBackgroundManager()->GetStorage().SetBackgroundEnabled({ RobotName }, EnabledFlag, "change_rt_robot"));
        if (!IsEnabled()) {
            CHECK_WITH_LOG(server.GetRTBackgroundManager()->WaitObjectInactive(RobotName, Now() + TDuration::Seconds(20)));
        }
    }

    void TSetSetting::DoExecute(ITestContext& context) {
        context.GetServer<IBaseServer>().GetSettings().SetValue(Key, Value, "tester");
        if (Waiting) {
            const TInstant start = Now();
            while (context.GetServer<IBaseServer>().GetSettings().GetValueDef<TString>(Key, "") != Value && Now() - start < TDuration::Seconds(15)) {
                Sleep(TDuration::Seconds(1));
            }
            CHECK_WITH_LOG(context.GetServer<IBaseServer>().GetSettings().GetValueDef<TString>(Key, "") == Value);
        }
    }

    void TSendRequest::DoExecute(ITestContext& context) {
        auto sender = context.GetServer<IBaseServer>().GetSenderPtr(SenderAPI);
        CHECK_WITH_LOG(sender);
        NExternalAPI::TServiceApiHttpDirectRequest request(Request);
        auto reply = sender->SendRequest(request);
        if (ExpectedCode) {
            CHECK_WITH_LOG(reply.GetReply().Code() == *ExpectedCode) << "got: " << reply.GetReply().Code()
                << ", expected: " << *ExpectedCode << Endl;
        }
        if (ExpectedContent) {
            TStringBuf sb(reply.GetReply().Content().data(), reply.GetReply().Content().size());
            while (sb.EndsWith('\n')) {
                sb.Chop(1);
            }
            CHECK_WITH_LOG(sb == *ExpectedContent) << reply.GetReply().Content() << Endl;
        }
        if (JsonExpectations.size()) {
            NJson::TJsonValue jsonInfo;
            CHECK_WITH_LOG(NJson::ReadJsonFastTree(reply.GetReply().Content(), &jsonInfo));
            for (auto&& i : JsonExpectations) {
                auto* jsonValue = jsonInfo.GetValueByPath(i.first);
                CHECK_WITH_LOG(!!jsonValue || i.second == "$NULL");
                if (jsonValue) {
                    CHECK_WITH_LOG(jsonValue->GetStringRobust() == i.second) << i.first << " / " << i.second << " / " << jsonValue->GetStringRobust() << Endl;
                }
            }
        }
    }

}
