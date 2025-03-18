#include "fetch.h"

namespace NSteam {

void TBaseFetcher::Fail(TFetchTask& task, const TString& reason, int httpCode)
{
    TSimpleSharedPtr<TFetchedDoc> result(new TFetchedDoc());
    result->Failed = true;
    result->ErrorMessage = reason;
    result->HttpCode = httpCode;
    result->Url = task.Url;
    result->JobId = task.JobId;
    task.Sink->Schedule(result);
}

void TBaseFetcher::Add(const TFetchTask& task)
{
    TGuard guard(InGuard);
    InTasks.push_back(task);
}

void TBaseFetcher::SetInFlight(TFetchTask& task)
{
    InFlight.insert(std::make_pair(task.Url, task));
}

void TBaseFetcher::Complete(TFetchedDoc& result)
{
    TGuard guard(InGuard);
    const TInstant now = TInstant::Now();
    for (TTaskMap::iterator ii = InFlight.begin(), end = InFlight.end(); ii != end; ) {
        TFetchTask& task = ii->second;
        if (result.Url == task.Url) {
            TSimpleSharedPtr<TFetchedDoc> thisResult(new TFetchedDoc(result));
            thisResult->JobId = task.JobId;
            task.Sink->Schedule(thisResult);
        }
        else if (task.Deadline <= now) { // TODO: move this shit to externally invoked function
            Fail(task, "Timed out waiting for document to be fetched", HTTP_PROXY_REQUEST_TIME_OUT);
        }
        else {
            ++ii;
            continue;
        }
        InFlight.erase(ii++);
    }
}

}
