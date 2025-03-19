#include "object.h"

namespace NCS {
    namespace NBackground {

        TSignalTagsSet IRTQueueTask::GetSignalTags() const {
            TSignalTagsSet result;
            result("owner_id", OwnerId)("queue_id", QueueId);
            for (auto&& i : GetCustomTags()) {
                result(i.first, i.second);
            }
            return result;

        }

        TMap<TString, TString> IRTQueueTask::GetCustomTags() const {
            TMap<TString, TString> result = Tags;
            DoFillCustomTags(result);
            return result;
        }

        NCS::NLogging::TBaseLogRecord IRTQueueTask::GetLogRecord() const {
            NCS::NLogging::TBaseLogRecord result;
            result
                .Add("&owner_id", OwnerId)
                .Add("&queue_id", QueueId)
                .Add("internal_task_id", InternalTaskId).Add("start_instant", StartInstant);
            for (auto&& i : GetCustomTags()) {
                result.Add("&" + i.first, i.second);
            }
            return result;
        }
        namespace {
            const TVector<double> TaskDurations = { 0, 1, 2, 3, 5, 10, 20, 30, 50, 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 20000, 30000, 50000, 100000 };
        }

        bool IRTQueueAction::Execute(const IRTQueueTask& task, const IBaseServer& server) const noexcept {
            bool resultFlag = false;
            TString exceptionReason;
            TString code = "success";
            const TInstant startInstant = Now();
            try {
                resultFlag = DoExecute(task, server);
                if (!resultFlag) {
                    code = "fail";
                }
            } catch (...) {
                exceptionReason = CurrentExceptionMessage();
                code = "exception";
                resultFlag = false;
            }
            const TDuration dAction = Now() - startInstant;
            const TDuration dTask = Now() - task.GetStartInstant();
            if (!resultFlag) {
                TFLEventLog::Error("cannot execute queued task")("exception_reason", exceptionReason)
                    .Signal("rt_queue_task_action")("&code", code);
                TCSSignals::HSignal("rt_queue_action_failed_duration", TaskDurations)(TFLRecords::CollectSignalTags())(dAction.MilliSeconds());
                TCSSignals::HSignal("rt_queue_task_failed_duration", TaskDurations)(TFLRecords::CollectSignalTags())(dTask.MilliSeconds());
            } else {
                TFLEventLog::JustSignal("rt_queue_task_action")("&code", code);
                TCSSignals::HSignal("rt_queue_action_success_duration", TaskDurations)(TFLRecords::CollectSignalTags())(dAction.MilliSeconds());
                TCSSignals::HSignal("rt_queue_task_success_duration", TaskDurations)(TFLRecords::CollectSignalTags())(dTask.MilliSeconds());
            }
            return resultFlag;
        }

    }
}
