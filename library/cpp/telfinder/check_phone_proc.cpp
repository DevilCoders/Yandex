#include "check_phone_proc.h"

void TCheckPhoneProcessor::ProcessTelephonoid(const TTelephonoid& telephonoid, const TAreaScheme& s) {
    if (Accept(telephonoid, s)) {
        ProcessInternal(telephonoid, s);
    }
}

bool TCheckPhoneProcessor::Accept(const TTelephonoid& /*telephonoid*/, const TAreaScheme& s) const {
    return !s.Weak; // By default, don't accept weak schemes
}
