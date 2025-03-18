#pragma once

#include "phone_proc.h"

class TCheckPhoneProcessor: public IPhoneProcessor {
public:
    void ProcessTelephonoid(const TTelephonoid& telephonoid, const TAreaScheme& s) override;

protected:
    virtual bool Accept(const TTelephonoid& telephonoid, const TAreaScheme& s) const;

    virtual void ProcessInternal(const TTelephonoid& telephonoid, const TAreaScheme& s) = 0;
};
