#include "dummy.h"

#include <util/datetime/base.h>

extern "C" void F()
{
    Sleep(TDuration::MicroSeconds(100));
}
