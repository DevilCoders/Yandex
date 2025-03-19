#include "policy.h"
#include "config.h"

#include <search/meta/waitinfo/waitinfo.h>
#include <search/meta/scatter/task.h>

#include <library/cpp/cgiparam/cgiparam.h>

NScatter::IWaitPolicyRef NSimpleMeta::CreateWaitPolicy(const TCgiParameters& cgi, const TConfig& config) {
    NScatter::IWaitPolicyRef policy = MakeIntrusive<TWaitPolicy>();
    policy->WaitOptions.TasksCheckInterval = config.GetTasksCheckInterval();
    if (cgi.Has("timeout")) {
        policy->WaitOptions.RequestTimeout = TDuration::MicroSeconds(
            FromString<ui64>(cgi.Get("timeout"))
        );
    }
    if (cgi.Has("waitall", "da")) {
        policy->WaitOptions.WaitAll = true;
    }
    return policy;
}
