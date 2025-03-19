#include "basic_core.h"
#include "tm_full_internals.h"

#include <kernel/text_machine/interface/cores.h>

#include <kernel/text_machine/basic_core/tm_full_gen.cpp>

namespace {
    using namespace NTextMachine;

    class TWebFullCoreFactory
        : public ICoreFactory
    {
        TTextMachinePtr GetCore() const override {
            return GetFullCore();
        }
        TTextMachinePoolPtr GetCore(TMemoryPool& pool) const override {
            return GetFullCore(pool);
        }
    };

    struct TRegisterWebFullCore {
        TWebFullCoreFactory Factory;

        TRegisterWebFullCore() {
            RegisterCoreFactory<TWebCores>(ECoreType::ExperimentalFull, &Factory);
        }
    } staticInitWebFullCore;
} // namespace

namespace NTextMachine {
    TTextMachinePtr GetFullCore() {
        return THolder(new NBasicCore::TFullCoreMachine());
    }

    TTextMachinePoolPtr GetFullCore(TMemoryPool& pool) {
        return TTextMachinePoolPtr(pool.New<NBasicCore::TFullCoreMachine>());
    }
} // NTextMachine
