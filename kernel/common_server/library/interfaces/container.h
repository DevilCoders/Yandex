#pragma once
#include <library/cpp/yconf/conf.h>
#include <util/stream/output.h>
#include <kernel/common_server/util/json_processing.h>
#include <util/generic/cast.h>
#include <util/system/type_name.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <kernel/common_server/util/algorithm/ptr.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/common/scheme.h>

class TDefaultInterfaceContainerConfiguration {
public:
    static TString GetDefaultClassName() {
        return "fake";
    }
    static TString GetClassNameField() {
        return "class_name";
    }
    static TString GetConfigClassNameField() {
        return "Type";
    }
    static TString GetSpecialSectionForType(const TString& /*className*/) {
        return "";
    }
    template <class TProto>
    static TString GetClassNameFromProto(const TProto& proto) {
        return proto.GetClassName();
    }
    template <class TProto>
    static void SetClassNameToProto(TProto& proto, const TString& className) {
        proto.SetClassName(className);
    }
};

template <class TInterface, class TConfiguration = TDefaultInterfaceContainerConfiguration>
class TBaseInterfaceContainer {
protected:
    typename TInterface::TPtr Object;

public:
    using TConfigurationClass = TConfiguration;

    TBaseInterfaceContainer() = default;

    template <class T, typename... Args>
    T& RetObject(Args... args) {
        InitObject<T>(args...);
        return VGet<T>();
    }

    template <class T, typename... Args>
    TBaseInterfaceContainer& InitObject(Args... args) {
        Object = new T(args...);
        return *this;
    }

    TBaseInterfaceContainer& SetObject(typename TInterface::TPtr object) {
        Object = object;
        return *this;
    }

    TString GetClassName() const {
        return Object ? Object->GetClassName() : "__undefined";
    }

    template <class... TArgs>
    void Init(const TYandexConfig::Section* section, TArgs... args) {
        const TString type = section ? section->GetDirectives().Value<TString>(TConfiguration::GetConfigClassNameField(), TConfiguration::GetDefaultClassName()) : TConfiguration::GetDefaultClassName();
        Object = TInterface::TFactory::Construct(type);
        CHECK_WITH_LOG(!!Object) << "Incorrect interface type for container: " << type.data();
        if (section) {
            Object->Init(section, std::forward<TArgs>(args)...);
        } else {
            TAnyYandexConfig ayConfig;
            TStringBuilder sb;
            sb << TConfiguration::GetConfigClassNameField() << ": " << TConfiguration::GetDefaultClassName() << Endl;
            CHECK_WITH_LOG(ayConfig.ParseMemory(sb));
            Object->Init(ayConfig.GetRootSection(), std::forward<TArgs>(args)...);
        }
    }

    void ToString(IOutputStream& os) const {
        os << TConfiguration::GetConfigClassNameField() << ": " << GetClassName() << Endl;
        if (Object) {
            Object->ToString(os);
        }
    }

    TBaseInterfaceContainer(typename TInterface::TPtr object)
        : Object(object)
    {
    }

    template <class T>
    TBaseInterfaceContainer(TAtomicSharedPtr<T> object)
        : Object(object)
    {
        static_assert(std::is_base_of<TInterface, T>::value, "Derived not derived from BaseClass");
    }

    TBaseInterfaceContainer(TInterface* object)
        : Object(object)
    {
    }

    TBaseInterfaceContainer(THolder<TInterface>&& object)
        : Object(object.Release())
    {
    }

    TBaseInterfaceContainer& operator=(typename TInterface::TPtr object) {
        Object = object;
    }

    TBaseInterfaceContainer& operator=(TInterface* object) {
        Object = object;
    }

    template <class T>
    const T* GetAs() const {
        return dynamic_cast<const T*>(Object.Get());
    }

    template <class T>
    T* GetAs() {
        return dynamic_cast<T*>(Object.Get());
    }

    template <class T>
    T* MutableAs() const {
        return dynamic_cast<T*>(Object.Get());
    }

    template <class T>
    TAtomicSharedPtr<T> GetPtrAs() const {
        return std::dynamic_pointer_cast<T>(Object);
    }

    template <class T>
    TAtomicSharedPtr<T> GetVerifiedPtrAs() const {
        auto result = std::dynamic_pointer_cast<T>(Object);
        CHECK_WITH_LOG(!!result) << "Cast failed for " << TypeName<T>() << Endl;
        return result;
    }

    template <class T>
    const T& VGet() const {
        return *VerifyDynamicCast<const T*>(Object.Get());
    }

    template <class T>
    T& VGet() {
        return *VerifyDynamicCast<T*>(Object.Get());
    }

    template <class T>
    bool Is() const {
        return GetAs<T>();
    }

    typename TInterface::TPtr GetPtr() const {
        return Object;
    }

    const TInterface* operator->() const {
        Y_ASSERT(!!Object);
        return Object.Get();
    }

    const TInterface& operator*() const {
        Y_ASSERT(!!Object);
        return *Object;
    }

    TInterface* operator->() {
        Y_ASSERT(!!Object);
        return Object.Get();
    }

    bool operator!() const {
        return !Object;
    }

    operator bool() const {
        return !!Object;
    }

    template <class TProto>
    Y_WARN_UNUSED_RESULT bool DeserializeFromProto(const TProto& proto) {
        const TString& className = TConfiguration::GetClassNameFromProto(proto);
        if (className == "__undefined") {
            Object = nullptr;
            return true;
        }
        auto object = std::dynamic_pointer_cast<TInterface>(TInterface::TFactory::MakeHolder(className));
        if (!object) {
            TFLEventLog::Error("cannot construct object")("class_name", className)("type", TypeName<TInterface>())("raw_data", proto.DebugString());
            return false;
        }
        if (!object->DeserializeFromProto(proto)) {
            TFLEventLog::Error("cannot parse object")("raw_data", proto.DebugString());
            return false;
        }
        Object = object.Release();
        return true;
    }

    template <class TProto>
    TProto ToProto() const {
        TProto result;
        SerializeToProto(result);
        return result;
    }

    template <class TProto>
    TProto SerializeToProto() const {
        TProto result;
        SerializeToProto(result);
        return result;
    }

    template <class TProto>
    void SerializeToProto(TProto& result) const {
        if (!Object) {
            TConfiguration::SetClassNameToProto(result, "__undefined");
        } else {
            TConfiguration::SetClassNameToProto(result, Object->GetClassName());
            Object->SerializeToProto(result);
        }
    }

    template <class TServer>
    static NFrontend::TScheme GetScheme(const TServer& server) {
        NFrontend::TScheme result;
        result.Add<TFSWideVariants>("class_name").InitVariants<TInterface>(server).SetCustomStructureId(TConfiguration::GetSpecialSectionForType(""));
        return result;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo, const TString& className, const TMaybe<TString>& externalDataSection = Default<TMaybe<TString>>()) {
        auto gLogging = TFLRecords::StartContext()("raw_data", jsonInfo);
        THolder<TInterface> object(TInterface::TFactory::Construct(className));
        if (!object) {
            TFLEventLog::Error("incorrect_class_name")("type", TypeName<TInterface>())("class_name", className);
            return false;
        }
        const TString& specialSection = externalDataSection ? *externalDataSection : TConfiguration::GetSpecialSectionForType(className);
        if (!specialSection) {
            if (!object->DeserializeFromJson(jsonInfo)) {
                TFLEventLog::Error("cannot parse object");
                return false;
            }
        } else {
            if (!object->DeserializeFromJson(jsonInfo[specialSection])) {
                TFLEventLog::Error("cannot parse object");
                return false;
            }
        }
        Object = object.Release();
        return true;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo, const bool nullable = false) {
        TString className;
        if (jsonInfo.IsNull()) {
            if (!nullable) {
                TFLEventLog::Error("json is null but its denied");
                return false;
            } else {
                Object = nullptr;
            }
        }
        if (!jsonInfo.Has(TConfiguration::GetClassNameField())) {
            className = TConfiguration::GetDefaultClassName();
        } else if (!jsonInfo[TConfiguration::GetClassNameField()].GetString(&className)) {
            TFLEventLog::Error("no_class_name_info")("type", TypeName<TInterface>());
            return false;
        }
        if (className == "__undefined") {
            Object = nullptr;
            return true;
        }
        return DeserializeFromJson(jsonInfo, className);
    }

    template <class... TArgs>
    NJson::TJsonValue SerializeToJson(const TArgs&... args) const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        if (!Object) {
            result.InsertValue(TConfiguration::GetClassNameField(), "__undefined");
            return result;
        }
        const TString& specialSection = TConfiguration::GetSpecialSectionForType(Object->GetClassName());
        if (!specialSection) {
            result = Object->SerializeToJson(args...);
            ASSERT_WITH_LOG(result.IsMap());
        } else {
            result.InsertValue(specialSection, Object->SerializeToJson(args...));
        }
        Y_ASSERT(!result.Has(TConfiguration::GetClassNameField()));
        result.InsertValue(TConfiguration::GetClassNameField(), Object->GetClassName());
        return result;
    }

    template <class... TArgs>
    NJson::TJsonValue GetJsonReport(const TArgs&... args) const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        if (!Object) {
            result.InsertValue(TConfiguration::GetClassNameField(), "__undefined");
            return result;
        }
        const TString& specialSection = TConfiguration::GetSpecialSectionForType(Object->GetClassName());
        if (!specialSection) {
            result = Object->GetJsonReport(args...);
            ASSERT_WITH_LOG(result.IsMap());
        } else {
            result.InsertValue(specialSection, Object->GetJsonReport(args...));
        }
        Y_ASSERT(!result.Has(TConfiguration::GetClassNameField()));
        result.InsertValue(TConfiguration::GetClassNameField(), Object->GetClassName());
        return result;
    }
};

template <class TInterface>
class TInterfacesContainer {
private:
    using TObjects = TVector<TAtomicSharedPtr<TInterface>>;
    CSA_DEFAULT(TInterfacesContainer, TObjects, Objects);

public:
    TInterfacesContainer() = default;

    template <class T>
    TInterfacesContainer& Reset(TVector<TBaseInterfaceContainer<TInterface, T>>&& objects) {
        for (auto&& i : objects) {
            Objects.emplace_back(i.GetPtr());
        }
        return *this;
    }

    TInterfacesContainer(const TVector<TAtomicSharedPtr<TInterface>>& objects)
        : Objects(objects)
    {
    }

    TInterfacesContainer(TVector<TAtomicSharedPtr<TInterface>>&& objects)
        : Objects(std::move(objects))
    {
    }

    template <class T>
    TInterfacesContainer(const TVector<TBaseInterfaceContainer<TInterface, T>>& objects) {
        for (auto&& i : objects) {
            Objects.emplace_back(i.GetPtr());
        }
    }

    size_t size() const {
        return Objects.size();
    }

    typename TObjects::const_iterator begin() const {
        return Objects.begin();
    }

    typename TObjects::const_iterator end() const {
        return Objects.end();
    }

    template <class T = TDefaultInterfaceContainerConfiguration>
    TVector<TBaseInterfaceContainer<TInterface, T>> CastToContainersVector() const {
        TVector<TBaseInterfaceContainer<TInterface, T>> result;
        for (auto&& i : Objects) {
            result.emplace_back(i);
        }
        return result;
    }

    template <class T>
    TInterfacesContainer<T> CastTo() const {
        TInterfacesContainer<T> result;
        for (auto&& i : Objects) {
            auto newObj = std::dynamic_pointer_cast<T>(i);
            if (newObj) {
                result.MutableObjects().emplace_back(newObj);
            }
        }
        return result;
    }

    template <class T>
    TAtomicSharedPtr<T> GetFirstOf() const {
        for (auto&& i : Objects) {
            auto newObj = std::dynamic_pointer_cast<T>(i);
            if (newObj) {
                return newObj;
            }
        }
        return nullptr;
    }
};
