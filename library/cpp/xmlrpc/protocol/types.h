#pragma once

#include <util/datetime/base.h>

namespace NXmlRPC {
    class TInstantWithoutTimezone : public TInstant {
    public:
        TInstantWithoutTimezone(const TInstant t)
            : TInstant(t)
        {
        }
    };
}
