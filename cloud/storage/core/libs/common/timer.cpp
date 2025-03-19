#include "timer.h"

namespace NCloud {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TWallClockTimer final
    : public ITimer
{
public:
    TInstant Now() override
    {
        return TInstant::Now();
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

ITimerPtr CreateWallClockTimer()
{
    return std::make_shared<TWallClockTimer>();
}

}   // namespace NCloud
