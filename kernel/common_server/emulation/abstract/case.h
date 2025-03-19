#pragma once
#include <kernel/common_server/util/auto_actualization.h>
#include <kernel/common_server/library/interfaces/container.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/yconf/conf.h>

namespace NCS {
    class IEmulationCase {
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const = 0;
        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
    public:
        virtual ~IEmulationCase() = default;
        using TPtr = TAtomicSharedPtr<IEmulationCase>;
        using TFactory = NObjectFactory::TObjectFactory<IEmulationCase, TString>;
        virtual TString GetClassName() const = 0;
        virtual NJson::TJsonValue SerializeToJson() const final {
            return DoSerializeToJson();
        }
        Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) final {
            return DoDeserializeFromJson(jsonInfo);
        }
        void Init(const TYandexConfig::Section* section);
        void ToString(IOutputStream& os) const;
    };

    class TEmulationCaseContainer: public TBaseInterfaceContainer<IEmulationCase> {
    private:
        using TBase = TBaseInterfaceContainer<IEmulationCase>;
    public:
        using TBase::TBase;
    };

}
