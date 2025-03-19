#include "abstract.h"
#include <kernel/common_server/library/logging/events.h>

namespace NExternalAPI {

    TVector<NCS::NLogging::TBaseLogRecord> IServiceApiHttpRequest::TuneLogEvent(const NCS::NLogging::TBaseLogRecord& event) const {
        return { event };
    }

    TSignalTagsSet IServiceApiHttpRequest::BuildSignalTags() const {
        return TSignalTagsSet();
    }

    TString IServiceApiHttpRequest::GetBodyForLog(const NNeh::THttpRequest& request) const {
        return TString(request.GetPostData().AsStringBuf());
    }

    NCS::NObfuscator::TObfuscatorKeyMap IServiceApiHttpRequest::GetObfuscatorKey() const {
        return NCS::NObfuscator::TObfuscatorKeyMap();
    }

}
