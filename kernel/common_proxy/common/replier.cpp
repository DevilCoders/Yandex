#include "replier.h"

namespace NCommonProxy {

    IReplier::~IReplier()
    {}

    bool IReplier::Canceled() const {
        return false;
    }

    void IReplier::AddTrace(const TString& /*comment*/) {
    }

    void IReplier::AddMessage(const TString& /*processorName*/, int /*code*/, const NJson::TJsonValue& /*msg*/) {
    }

    TInstant IReplier::GetStartTime() const {
        return StartTime;
    }

}
