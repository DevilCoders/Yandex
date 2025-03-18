#include "select_in_word.h"

#include <util/generic/strbuf.h>
#include <util/system/cpu_id.h>
#include <util/system/platform.h>

#ifdef USE_BMI
namespace {
    bool IsAmd() {
        ui32 buffer[12];
        return TStringBuf{CpuBrand(buffer)}.StartsWith("AMD");
    }
    // The pdep instruction on AMD has a terrible latency.
    // See https://mobile.twitter.com/InstLatX64/status/1209095219087585281
    const bool USE_BMI2 = NX86::HaveBMI1() && NX86::HaveBMI2() && !IsAmd();
}
#endif

ui64 SelectInWord(ui64 x, int k) noexcept {
#ifdef USE_BMI
    if (USE_BMI2) {
        return SelectInWordBmi2(x, k);
    }
#endif
    return SelectInWordX86(x, k);
}

