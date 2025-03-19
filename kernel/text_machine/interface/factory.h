#pragma once

#include "text_machine.h"

#include <util/generic/map.h>
#include <util/generic/singleton.h>

namespace NTextMachine {
    enum class ECoreType {
        Production,
        ProductionExp,
        ProductionL2,
        OnotoleBest,
        Onotole,
        ExperimentalBest,
        ExperimentalFull
    };

    class IHeapCoreFactory {
    public:
        virtual TTextMachinePtr GetCoreInHeap() const = 0;
    };

    class IPoolCoreFactory {
    public:
        virtual TTextMachinePoolPtr GetCoreInPool(TMemoryPool& pool) const = 0;
    };

    class ICoreFactory
        : public IHeapCoreFactory
        , public IPoolCoreFactory
    {
    public:
        virtual TTextMachinePtr GetCore() const = 0;
        virtual TTextMachinePoolPtr GetCore(TMemoryPool& pool) const = 0;

        TTextMachinePtr GetCoreInHeap() const final {
            return GetCore();
        }
        TTextMachinePoolPtr GetCoreInPool(TMemoryPool& pool) const final {
            return GetCore(pool);
        }
    };

    class TCoreFactoryHolder {
    public:
        const ICoreFactory* GetFactory(ECoreType type) const {
            if (auto ptr = FactoryByType.FindPtr(type)) {
                return *ptr;
            }
            return nullptr;
        }
        void SetFactory(ECoreType type, const ICoreFactory* factory) {
            Y_ENSURE(!FactoryByType.contains(type), "register of core second time:" << type);
            FactoryByType[type] = factory;
        }

    private:
        TMap<ECoreType, const ICoreFactory*> FactoryByType; // no ownership
    };

    template <typename FactoryHolderType>
    inline TTextMachinePtr GetCore(ECoreType type) {
        if (auto factory = Singleton<FactoryHolderType>()->GetFactory(type)) {
            return factory->GetCore();
        }
        return nullptr;
    }
    template <typename FactoryHolderType>
    inline TTextMachinePtr GetCoreSafe(ECoreType type) {
        TTextMachinePtr res = GetCore<FactoryHolderType>(type);
        Y_ENSURE(
            !!res,
            "failed to obtain text machine core " << type << "; missing PEERDIR?"
        );
        return res;
    }
    template <typename FactoryHolderType>
    inline TTextMachinePoolPtr GetCore(ECoreType type, TMemoryPool& pool) {
        if (auto factory = Singleton<FactoryHolderType>()->GetFactory(type)) {
            return factory->GetCore(pool);
        }
        return nullptr;
    }
    template <typename FactoryHolderType>
    inline TTextMachinePoolPtr GetCoreSafe(ECoreType type, TMemoryPool& pool) {
        TTextMachinePoolPtr res = GetCore<FactoryHolderType>(type, pool);
        Y_ENSURE(
            !!res,
            "failed to obtain text machine core " << type << "; missing PEERDIR?"
        );
        return res;
    }
    template <typename FactoryHolderType>
    inline void RegisterCoreFactory(ECoreType type, const ICoreFactory* factory) {
        Y_ASSERT(factory);
        Singleton<FactoryHolderType>()->SetFactory(type, factory);
    }
} // NTextMachine
