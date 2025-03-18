#pragma once

#include "check_phone_proc.h"
#include "phone.h"

class TPhoneCollector: public TCheckPhoneProcessor {
protected:
    TFoundPhones& FoundPhones;

public:
    TPhoneCollector(TFoundPhones& phones)
        : FoundPhones(phones)
    {
    }

protected:
    void ProcessInternal(const TTelephonoid& telephonoid, const TAreaScheme& s) override;
};
