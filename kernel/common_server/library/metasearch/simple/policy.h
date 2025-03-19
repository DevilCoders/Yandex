#pragma once

#include <search/meta/scatter/wait_policy.h>

class TCgiParameters;

namespace NSimpleMeta {
    class TConfig;

    class TWaitPolicy: public NScatter::IWaitPolicy {
        bool IsAccepted(const NScatter::ITask& /*task*/) final {
            return true;
        }
    };

    NScatter::IWaitPolicyRef CreateWaitPolicy(const TCgiParameters& cgi, const NSimpleMeta::TConfig& config);
}
