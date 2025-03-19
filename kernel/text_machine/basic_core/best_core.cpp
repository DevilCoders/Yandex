#include "basic_core.h"
#include "tm_best_internals.h"

#include <kernel/text_machine/interface/cores.h>

#include <kernel/text_machine/basic_core/tm_best_gen.cpp>

namespace {
    using namespace NTextMachine;

    class TWebBestCoreFactory
        : public ICoreFactory
    {
        TTextMachinePtr GetCore() const override {
            return GetBestCore();
        }
        TTextMachinePoolPtr GetCore(TMemoryPool& pool) const override {
            return GetBestCore(pool);
        }
    };

    struct TRegisterWebBestCore {
        TWebBestCoreFactory Factory;

        TRegisterWebBestCore() {
            RegisterCoreFactory<TWebCores>(ECoreType::ExperimentalBest, &Factory);
        }
    } staticInitWebBestCore;
} // namespace

namespace NTextMachine {
    TTextMachinePtr GetBestCore() {
        return MakeHolder<NBasicCore::TBestCoreMachine>();
    }

    TTextMachinePoolPtr GetBestCore(TMemoryPool& pool) {
        return TTextMachinePoolPtr(pool.New<NBasicCore::TBestCoreMachine>());
    }
} // NTextMachine
