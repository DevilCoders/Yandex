#pragma once

#include <kernel/text_machine/interface/text_machine.h>

namespace NTextMachine {
    TTextMachinePtr GetBestCore();
    TTextMachinePoolPtr GetBestCore(TMemoryPool& pool);

    TTextMachinePtr GetFullCore();
    TTextMachinePoolPtr GetFullCore(TMemoryPool& pool);
}
