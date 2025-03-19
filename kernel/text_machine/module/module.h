#pragma once

#include "save_to_json.h"

#include <kernel/lingboost/constants.h>

#include <util/system/compiler.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/map.h>

#include <initializer_list>


namespace NModule {
    template <typename T>
    struct TSelectorArg {};

    // Base classes for unit/family states
    // Define id tokens (TId)
    //
    template <typename Id>
    struct TStateIdHolder {
        using TId = Id;
    };

    template <typename State>
    struct TStateBase
        : public TStateIdHolder<TStateBase<State>>
        , public TNonCopyable
    {
        mutable const char* StringId = nullptr;

        void SaveToJson(NJson::TJsonValue&) const {
        }
    };

    template <typename UnitFamily>
    struct TFamilyStateBase
        : public TStateIdHolder<typename UnitFamily::TId>
        , public TNonCopyable
    {
        mutable const char* StringId = nullptr;

        void SaveToJson(NJson::TJsonValue&) const {
        }
    };

    // NOTE. Following two type aliases
    // help to avoid compiler error in MSVC2013
    //
    template <typename State>
    using TStateIdAlias = typename TStateBase<State>::TId;

    template <typename UnitFamily>
    using TFamilyStateIdAlias = typename TFamilyStateBase<UnitFamily>::TId;

    // Null state and processor
    //
    struct TNullState : public TStateBase<TNullState> {};

    class TNullProcessor
        : public TJsonSerializable
    {
    public:
        using TProcessorState = TNullState;
        using TId = TProcessorState::TId;

        void SaveToJson(NJson::TJsonValue&) const {
        }
    };

    struct TNullUnit {
        using TState = TNullState;
        using TId = TState::TId;
    };

    template <bool value, typename UnitType>
    using TUnitIf = std::conditional_t<value, UnitType, TNullUnit>;

    // Scatter helpers
    //
    template <typename T>
    class TStaticScatterTag;

    template <typename T>
    class TScatterTag;

    template <typename T>
    class TStaticScatterTagBase {
    public:
        static TStaticScatterTag<T> Upcast() {
            return TStaticScatterTag<T>();
        }
    };

    template <typename T>
    class TStaticScatterTag : public TStaticScatterTagBase<T> {};

    template <typename T>
    class TScatterTagBase {
    public:
        static TScatterTag<T> Upcast() {
            return TScatterTag<T>();
        }
    };

    template <typename T>
    class TScatterTag : public TScatterTagBase<T> {};

    // Dependency check helpers
    //
    template <typename Req, typename... Args>
    struct TRequireChecker;

    template <typename Req>
    struct TRequireChecker<Req> {
        enum { Result = 0 };
    };

    template <typename Req, typename Arg, typename... Args>
    struct TRequireChecker<Req, Arg, Args...> {
        enum {
            Result = std::is_same<Req, TNullUnit>::value
                || std::is_same<typename Req::TId, typename Arg::TId>::value
                || TRequireChecker<Req, Args...>::Result
        };
    };

    template <typename... Reqs>
    struct TRequireList {};

    template <typename Req, typename... Args>
    struct TRequireOne {
        static_assert(TRequireChecker<Req, Args...>::Result, "required unit dependency is missing");
    };

    template <typename ReqsList, typename... Args>
    struct TRequireAllOf;

    template <typename... Args>
    struct TRequireAllOf<TRequireList<>, Args...> {};

    template <typename Req, typename... Reqs, typename... Args>
    struct TRequireAllOf<TRequireList<Req, Reqs...>, Args...>
        : public TRequireOne<Req, Args...>
        , public TRequireAllOf<TRequireList<Reqs...>, Args...>
    {};

    // Unit traits
    //
    template <typename M, typename P, typename A, size_t Id>
    struct TUnitTraits {
        using TMachine = M;
        using TStub = P;
        using TAccess = A;
        enum { UnitId = Id };
    };

    // Unit info
    //
    inline TStringBuf GetNameByUnitCppName(TStringBuf cppName) {
        Y_VERIFY(cppName.SkipPrefix(TStringBuf("T")));
        Y_VERIFY(cppName.ChopSuffix(TStringBuf("Unit")));
        return cppName;
    }

    inline TStringBuf GetNameByFamilyCppName(TStringBuf cppName) {
        Y_VERIFY(cppName.SkipPrefix(TStringBuf("T")));
        Y_VERIFY(cppName.ChopSuffix(TStringBuf("Family")));
        return cppName;
    }

    template <typename T>
    class TInfoByName {
    public:
        template <typename... Args>
        T& Update(TStringBuf name, Args&&... args) {
            return InfoByName.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(name),
                std::forward_as_tuple(name, std::forward<Args>(args)...)).first->second;
        }

        const T* FindPtr(TStringBuf name) const {
            return InfoByName.FindPtr(name);
        }

    private:
        TMap<TStringBuf, T> InfoByName;
    };

    class TValuesList {
    public:
        template <typename... Args>
        TValuesList(Args&&... args)
            : Values({args...})
        {
        }

        TValuesList(std::initializer_list<int> values)
            : Values(values)
        {
        }

        TVector<int> Values;
    };

    class TMethodTemplateArgInfo {
    public:
        TMethodTemplateArgInfo(
            TStringBuf name,
            const NLingBoost::IIntegralType* descr);

        TStringBuf GetName() const;
        const NLingBoost::IIntegralType& GetDescr() const;

        void SetInstantiations(TVector<int>&& values);
        const TVector<int>& GetInstantiations() const;

        void SetAlwaysForward(bool value);
        bool GetAlwaysForward() const;

    private:
        TStringBuf Name;
        const NLingBoost::IIntegralType* Descr = nullptr;
        TVector<int> InstantiatedValues;
        bool AlwaysForward = false;
    };

    class TUnitMethodInfo {
    public:
        TUnitMethodInfo(TStringBuf name);

        TStringBuf GetName() const;

        TMethodTemplateArgInfo& UpdateTemplateArg(
            TStringBuf argName,
            const NLingBoost::IIntegralType* descr)
        {
            return TemplateArgByName.Update(argName, descr);
        }
        const TMethodTemplateArgInfo* GetTemplateArgInfo(TStringBuf argName) const {
            return TemplateArgByName.FindPtr(argName);
        }

    private:
        TStringBuf Name;
        TInfoByName<TMethodTemplateArgInfo> TemplateArgByName;
    };

    using TUnitMethodInfoByName = TInfoByName<TUnitMethodInfo>;

    class IUnitInfo {
    public:
        virtual ~IUnitInfo() {}

        virtual TStringBuf GetDomainName() const = 0;
        virtual TStringBuf GetName() const = 0;
        virtual TStringBuf GetCppName() const = 0;
        virtual const TUnitMethodInfo* GetMethodInfo(TStringBuf methodName) const = 0;
    };

    inline void SetUnitMethodInfoArg(
        TUnitMethodInfoByName& methods,
        TStringBuf methodName,
        const NLingBoost::IIntegralType* typeDescr,
        TStringBuf argName,
        TVector<int>&& values)
    {
        TMethodTemplateArgInfo& info = methods.Update(methodName).UpdateTemplateArg(argName, typeDescr);
        info.SetInstantiations(std::move(values));
    }

    inline void ForwardUnitMethodInfoArg(
        TUnitMethodInfoByName& methods,
        TStringBuf methodName,
        const NLingBoost::IIntegralType* typeDescr,
        TStringBuf argName)
    {
        TMethodTemplateArgInfo& info = methods
            .Update(methodName)
            .UpdateTemplateArg(argName, typeDescr);

        info.SetAlwaysForward(true);
    }

    class TUnitInfoBase
        : public IUnitInfo
    {
    protected:
        TUnitMethodInfoByName Methods;

    public:
        ~TUnitInfoBase() override {}

        void SetNames(TStringBuf domainName, TStringBuf name, TStringBuf cppName) {
            DomainName = domainName;
            Name = name;
            CppName = cppName;
        }

        TStringBuf GetDomainName() const final {
            return DomainName;
        }
        TStringBuf GetName() const final {
            return Name;
        }
        TStringBuf GetCppName() const final {
            return CppName;
        }

        const TUnitMethodInfo* GetMethodInfo(TStringBuf methodName) const final {
            return Methods.FindPtr(methodName);
        }

    private:
        TStringBuf DomainName;
        TStringBuf Name;
        TStringBuf CppName;
    };

    class TUnitRegistry {
    public:
        static TUnitRegistry& Instance() {
            return *Singleton<TUnitRegistry>();
        }

        void RegisterUnit(THolder<IUnitInfo>&& info);
        const IUnitInfo* GetUnitInfo(TStringBuf domainName, TStringBuf unitName) const;

    private:
        using TUnitId = std::pair<TStringBuf, TStringBuf>;
        TMap<TUnitId, THolder<IUnitInfo>> UnitByName;
    };

    class TUnitRegistrator {
    public:
        TUnitRegistrator(
            THolder<TUnitInfoBase> info,
            TStringBuf domainName,
            TStringBuf unitName,
            TStringBuf unitCppName)
        {
            info->SetNames(domainName, unitName, unitCppName);
            TUnitRegistry::Instance().RegisterUnit(std::move(info));
        }
    };
} // NModule
