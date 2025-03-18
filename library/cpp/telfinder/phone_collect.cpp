#include "phone_collect.h"

#include "telfinder.h"

void TPhoneCollector::ProcessInternal(const TTelephonoid& telephonoid, const TAreaScheme& s) {
    FoundPhones.push_back(TTelFinder::CreateFoundPhone(telephonoid, s));
}
