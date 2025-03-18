#pragma once

#include "telephonoid.h"
#include "tel_schemes.h"

struct IPhoneProcessor {
    virtual void ProcessTelephonoid(const TTelephonoid& telephonoid, const TAreaScheme& s) = 0;
};
