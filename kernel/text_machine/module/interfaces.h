#pragma once

#include "module.h"
#include <kernel/text_machine/metadata/tm_metadata.pb.h>

#include <util/stream/str.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/list.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/singleton.h>
#include <util/generic/set.h>
#include <util/generic/xrange.h>
#include <util/generic/algorithm.h>
#include <util/string/join.h>
#include <util/string/subst.h>

namespace NModule {
    class TArgsArray {
        struct TArg {
            TString Type;
            TString Name;
            bool Multi;
            TString Forward;
        };

        TVector<TArg> Args;

        explicit TArgsArray(TVector<TArg>&& args)
            : Args(std::move(args))
        {}

    public:
        TArgsArray() = default;

        void AddArg(const TString& argType, const TString& argName) {
            Args.push_back(TArg{argType, argName, false, ""});
        }

        void AddMultiArg(const TString& argType, const TString& argName) {
            Args.push_back(TArg{argType, argName, true, ""});
        }

        bool Empty() const {
            return Args.empty();
        }

        void SetForward(const TString& forwardStr) {
            Args.back().Forward = forwardStr;
        }

        TString GetSignature(bool needArgNames = true) const {
            TStringStream out;

            bool first = true;
            for (const auto& arg : Args) {
                if (!first) {
                    out << ", ";
                }

                out << arg.Type
                    << (arg.Multi ? "..." : "")
                    << (needArgNames && arg.Name ? " " + arg.Name : TString());

                first = false;
            }

            return out.Str();
        }

        TString GetCall() const {
            TStringStream out;

            bool first = true;
            for (const auto& arg : Args) {
                if (!first) {
                    out << ", ";
                }

                if (!!arg.Forward) {
                    out << arg.Forward;
                } else {
                    out << arg.Name << (arg.Multi ? "..." : "");
                }
                first = false;
            };

            return out.Str();
        }

        TArgsArray GetInstantiation(TString& removedName) const {
            Y_ASSERT(!Empty());
            removedName = Args[0].Name;
            return TArgsArray(TVector<TArg>(Args.begin() + 1, Args.end()));
        }
    };

    /*
        TProcessorMethodInstantiator generates instantiations of templates like the following:
        void SomeFunction<int|enum x, ...>(...) {
            Unit1.AnotherFunction<x>(...);
            Unit2.AnotherFunction<x>(...);
        }
        by the first template arg.
        TInstantiateData keeps the necessary static data.

        All units should be registered in Singleton<TUnitRegistry> beforehand,
        providing the table (unit name) -> (list of values for x);
        an unit can also always-forward the call meaning it accepts any value of x.

        The result consists of:
            list of always-forward units
                for the default (non-overridden) implementation of SomeFunction;
            map of instantiations. For every instantiation,
                there is the value of x and the list of units that handle this value;
                always-forward units are included to every list.
    */
    struct TInstantiateData {
        TString InstantiatedName; // "AnotherFunction" from the comment above
        TVector<TString> InstantiatedTemplateArgs;
        TVector<TString> InstantiatedMethodArgs;

        explicit TInstantiateData(TString instantiatedName)
            : InstantiatedName(std::move(instantiatedName))
        {
        }

        TInstantiateData& UseTypeArg(TString argValue) {
            InstantiatedTemplateArgs.emplace_back(std::move(argValue));
            return *this;
        }

        TInstantiateData& UseArg(TString argValue) {
            InstantiatedMethodArgs.emplace_back(std::move(argValue));
            return *this;
        }
    };

    class TProcessorMethod;

    class TProcessorMethodInstantiator {
    public:
        using TValue = int;
        using TUnitIndex = size_t;
        using TUnitName = TString;

    private:
        const TProcessorMethod* Parent = nullptr;
        const TInstantiateData* Data = nullptr;
        TString TemplateArgName;
        TArgsArray OtherTemplateArgs;
        const NLingBoost::IIntegralType* TypeDescriptor = nullptr;
        TVector<TString> UnknownUnits;
        TVector<TUnitIndex> AlwaysForward;
        TMap<TValue, TVector<TUnitIndex>> Instantiations;

        TString InstantiateNestedTemplates(const TString& call, TValue value) const {
            // "TStreamSelector<Stream>" -> "TStreamSelector<::NLingBoost::TStream::Title>"
            TString substFrom = TString::Join("<", TemplateArgName, ">");
            TString substTo = TString::Join("<", TypeDescriptor->GetValueScopePrefix() + TypeDescriptor->GetValueLiteral(value), ">");
            TString result = call;
            SubstGlobal(result, substFrom, substTo);
            return result;
        }

    public:
        TProcessorMethodInstantiator(const TProcessorMethod* parent, const TInstantiateData* data);
        void RegisterUnit(TStringBuf domainName, TUnitIndex unitIndex, const TUnitName& unitName) {
            const IUnitInfo* unitInfo = TUnitRegistry::Instance().GetUnitInfo(domainName, unitName);
            if (!unitInfo) { // unknown unit: fatal error, will throw in PrepareInstantiations()
                UnknownUnits.push_back(unitName);
                return;
            }
            const TUnitMethodInfo* methodInfo = unitInfo->GetMethodInfo(Data->InstantiatedName);
            if (!methodInfo) {
                return; // the unit does not provide the method, just ignore this unit
            }
            const TMethodTemplateArgInfo* argInfo = methodInfo->GetTemplateArgInfo(TemplateArgName);
            Y_VERIFY(argInfo, "no argument info for %s in %s::%s()", TemplateArgName.data(), unitName.data(), Data->InstantiatedName.data());
            if (TypeDescriptor && TypeDescriptor != &argInfo->GetDescr()) {
                TString errorMessage = TString::Join(
                    "wrong type of argument ",
                    TemplateArgName,
                    " in ", unitName, "::", Data->InstantiatedName, "()",
                    ": ", TypeDescriptor->GetTypeCppName(), " != ", argInfo->GetDescr().GetTypeCppName());
                Y_VERIFY(false, "%s", errorMessage.data());
            }
            TypeDescriptor = &argInfo->GetDescr();
            if (argInfo->GetAlwaysForward()) {
                AlwaysForward.push_back(unitIndex);
            } else {
                for (TValue enumValue : argInfo->GetInstantiations())
                    Instantiations[enumValue].push_back(unitIndex);
            }
        }
        void PrepareInstantiations() {
            Y_VERIFY(UnknownUnits.empty(), "unknown units: %s", JoinSeq(", ", UnknownUnits).data());
            if (!AlwaysForward.empty()) {
                for (auto& it : Instantiations) {
                    it.second.insert(it.second.end(), AlwaysForward.begin(), AlwaysForward.end());
                }
            }
        }
        const TVector<TUnitIndex>& AlwaysForwardUnits() const {
            return AlwaysForward;
        }
        const TMap<TValue, TVector<TUnitIndex>>& GetInstantiations() {
            return Instantiations;
        }
        TString GetInlineSignatureInstantiated(TValue value) const;
        TString GetCallForwarded() const {
            TStringStream out;
            if (!Data->InstantiatedTemplateArgs.empty()) {
                out << "template ";
            }
            out << Data->InstantiatedName;
            if (!Data->InstantiatedTemplateArgs.empty()) {
                out << '<' << JoinSeq(", ", Data->InstantiatedTemplateArgs) << '>';
            }
            out << '(' << JoinSeq(", ", Data->InstantiatedMethodArgs) << ')';
            return out.Str();
        }
        TString GetCallInstantiated(TValue value) const {
            TStringStream out;
            if (!Data->InstantiatedTemplateArgs.empty()) {
                out << "template ";
            }
            out << Data->InstantiatedName;
            if (!Data->InstantiatedTemplateArgs.empty()) {
                TString name = TypeDescriptor->GetValueScopePrefix() + TypeDescriptor->GetValueLiteral(value);
                out << '<';
                bool first = true;
                for (const TString& argName : Data->InstantiatedTemplateArgs) {
                    if (!first) {
                        out << ", ";
                    }
                    first = false;
                    out << (argName == TemplateArgName ? name : InstantiateNestedTemplates(argName, value));
                }
                out << '>';
            }
            out << '(' << InstantiateNestedTemplates(JoinSeq(", ", Data->InstantiatedMethodArgs), value) << ')';
            return out.Str();
        }
    };

    class ICodegenHelpers {
    public:
        virtual ~ICodegenHelpers() {}
        virtual TString GetUnitProcType(const TString& domain, const NTextMachineParts::TCodeGenInput::TUnitDescriptor& unit, size_t argSize) const = 0;
    };

    class IMachineDefinitionListener {
    public:
        virtual void ProcessMachineDefinition(const NTextMachineParts::TCodeGenInput::TMachineDescriptor& machine) = 0;
        virtual ~IMachineDefinitionListener() {}
    };

    class IMethodCustomGenerator {
    public:
        virtual void Generate(IOutputStream& out, const NTextMachineParts::TCodeGenInput::TMachineDescriptor& machine, ICodegenHelpers& helpers) const = 0;
        virtual IMachineDefinitionListener* GetListener() {
            return nullptr;
        }
        virtual ~IMethodCustomGenerator() {}
    };

    class TProcessorMethod {
    private:
        TString Name;
        bool Const = false;
        bool Static = false;
        bool Guarded = false;
        bool OutOfLineInstantiated = false;
        TArgsArray TemplateArgs;
        TArgsArray MethodArgs;
        TMaybe<TInstantiateData> InstantiateData; // THolder would delete copy constructor, so TMaybe
        TAtomicSharedPtr<IMethodCustomGenerator> CustomGenerator;

        friend class TProcessorMethodInstantiator;

    public:
        explicit TProcessorMethod(const TString& name)
            : Name(name)
        {}

        TProcessorMethod& SetConst() {
            Const = true;
            return *this;
        }

        TProcessorMethod& SetStatic() {
            Static = true;
            return *this;
        }

        TProcessorMethod& SetOutOfLineInstantiated() {
            OutOfLineInstantiated = true;
            return *this;
        }

        TProcessorMethod& SetGuarded() {
            Guarded = true;
            return *this;
        }

        TProcessorMethod& AddTypeArg(const TString& argType, const TString& argName) {
            TemplateArgs.AddArg(argType, argName);
            return *this;
        }

        TProcessorMethod& AddTypeMultiArg(const TString& argType, const TString& argName) {
            TemplateArgs.AddMultiArg(argType, argName);
            return *this;
        }

        TProcessorMethod& AddArg(const TString& argType, const TString& argName) {
            MethodArgs.AddArg(argType, argName);
            return *this;
        }

        TProcessorMethod& AddMultiArg(const TString& argType, const TString& argName) {
            MethodArgs.AddMultiArg(argType, argName);
            return *this;
        }

        TProcessorMethod& SetForward(const TString& forwardStr) {
            MethodArgs.SetForward(forwardStr);
            return *this;
        }

        TProcessorMethod& InstantiateAs(const TInstantiateData& data) {
            InstantiateData = data;
            return *this;
        }

        TProcessorMethod& SetCustomGenerator(TAtomicSharedPtr<IMethodCustomGenerator>&& generator) {
            Y_ASSERT(generator);
            CustomGenerator = std::move(generator);
            return *this;
        }

        bool HasTypeArgs() const {
            return !TemplateArgs.Empty();
        }

        TString GetSignature(bool needArgNames = true) const {
            return GetSignatureWithNamePrefix("", true, needArgNames);
        }

        TString GetInlineSignature(bool needArgNames = true) const {
            return GetSignatureWithNamePrefix("", true, needArgNames, true);
        }

        TString GetOutOfLineSignature(const TString& className, bool needArgNames = true) const {
            return GetSignatureWithNamePrefix(className + "::", false, needArgNames);
        }

        TString GetCall() const {
            TStringStream out;
            out << Name << "(" << MethodArgs.GetCall() << ")";
            return out.Str();
        }

        bool IsOutOfLineInstatiated() const {
            return OutOfLineInstantiated;
        }

        bool IsGuarded() const {
            return Guarded;
        }

        // Note: methods of the returned TProcessorMethodInstantiator can be only called while TProcessorMethod *this is alive.
        THolder<TProcessorMethodInstantiator> CreateExplicitInstantiator() const {
            if (InstantiateData)
                return MakeHolder<TProcessorMethodInstantiator>(this, InstantiateData.Get());
            return {};
        }

        const IMethodCustomGenerator* GetCustomGenerator() const {
            return CustomGenerator.Get();
        }

        IMachineDefinitionListener* GetDefinitionListener() {
            return CustomGenerator ? CustomGenerator->GetListener() : nullptr;
        }

    private:
        TString GetSignatureWithNamePrefix(const TString& namePrefix, bool isInClass,
            bool needArgNames = true, bool forceInline = false) const
        {
            TStringStream out;
            if (!TemplateArgs.Empty()) {
                out << "template<" << TemplateArgs.GetSignature() << "> ";
            }
            if (forceInline) {
                out << "Y_FORCE_INLINE ";
            }
            if (isInClass) {
                out << (Static ? "static " : "");
            }

            out << "void " << namePrefix << Name << "(" << MethodArgs.GetSignature(needArgNames) << ")"
                << (Const ? " const" : "");
            return out.Str();
        }
    };

    inline TProcessorMethodInstantiator::TProcessorMethodInstantiator(const TProcessorMethod* parent, const TInstantiateData* data)
        : Parent(parent)
        , Data(data)
    {
        Y_ASSERT(Parent);
        Y_ASSERT(Data);
        OtherTemplateArgs = Parent->TemplateArgs.GetInstantiation(TemplateArgName);
    }

    inline TString TProcessorMethodInstantiator::GetInlineSignatureInstantiated(TValue value) const {
         TStringStream out;
         if (!OtherTemplateArgs.Empty()) {
             out << "template<" << OtherTemplateArgs.GetSignature() << "> ";
         }
         out << "Y_FORCE_INLINE "
             << (Parent->Static ? "static " : "")
             << "void " << Parent->Name
             << "(" << InstantiateNestedTemplates(Parent->MethodArgs.GetSignature(true), value) << ")"
             << (Parent->Const ? " const" : "");
         return out.Str();
    }

    class TProcessorGenerator {
    public:
        using TConstIterator = TList<TProcessorMethod>::const_iterator;

    public:
        TProcessorGenerator() {
            AddMethod(TProcessorMethod("SaveToJson")
                .AddArg("NJson::TJsonValue&", "value")
                .SetConst()
                .SetOutOfLineInstantiated()
            );

            AddMethod(TProcessorMethod("ScatterStatic")
                .AddTypeArg("typename", "T")
                .AddTypeMultiArg("typename", "Args")
                .AddArg("const ::NModule::TStaticScatterTagBase<T>&", "tag").SetForward("tag.Upcast()")
                .AddMultiArg("const Args&", "args").SetStatic());

            AddMethod(TProcessorMethod("Scatter")
                .AddTypeArg("typename", "T")
                .AddTypeMultiArg("typename", "Args")
                .AddArg("const ::NModule::TScatterTagBase<T>&", "tag").SetForward("tag.Upcast()")
                .AddMultiArg("const Args&", "args"));
        }

        TProcessorGenerator& AddMethod(const TProcessorMethod& method) {
            Methods.push_back(method);
            return *this;
        }

        TConstIterator Begin() const {
            return Methods.begin();
        }

        TConstIterator End() const {
            return Methods.end();
        }

        void CreateListeners(TVector<IMachineDefinitionListener*>& listeners) {
            for (TProcessorMethod& method : Methods) {
                IMachineDefinitionListener* listener = method.GetDefinitionListener();
                if (listener) {
                    listeners.push_back(listener);
                }
            }
        }

    private:
        TList<TProcessorMethod> Methods;
    };

    class TMachineInterfaces {
    public:
        TMachineInterfaces& AddProcessor(const TString& name, const TProcessorGenerator& processor) {
            Procs[name] = processor;
            Procs[name].CreateListeners(Listeners);
            return *this;
        }

        bool HasProcessor(const TString& name) const {
            return Procs.contains(name);
        }

        const TProcessorGenerator& GetProcessor(const TString& name) const {
            Y_ENSURE(Procs.contains(name), "description not found for processor " << name);
            return Procs.find(name)->second;
        }

        void ProcessMachineDefinition(const NTextMachineParts::TCodeGenInput::TMachineDescriptor& machine) {
            for (IMachineDefinitionListener* listener : Listeners) {
                Y_ASSERT(listener);
                listener->ProcessMachineDefinition(machine);
            }
        }

    private:
        THashMap<TString, TProcessorGenerator> Procs;
        TVector<IMachineDefinitionListener*> Listeners;
    };

    template <typename TInterfaces>
    class TStaticMachineInterfaces {
    private:
        using TSelf = TStaticMachineInterfaces<TInterfaces>;

    public:
        static TMachineInterfaces& AddProcessor(const TString& name, const TProcessorGenerator& processor) {
            return Singleton<TSelf>()->Interfaces.AddProcessor(name, processor);
        }

        static bool HasProcessor(const TString& name) {
            return Singleton<TSelf>()->Interfaces.HasProcessor(name);
        }

        static const TProcessorGenerator& GetProcessor(const TString& name) {
            return Singleton<TSelf>()->Interfaces.GetProcessor(name);
        }

        static void ProcessMachineDefinition(const NTextMachineParts::TCodeGenInput::TMachineDescriptor& machine) {
            Singleton<TSelf>()->Interfaces.ProcessMachineDefinition(machine);
        }

    private:
        TInterfaces Interfaces;
    };
} // NModule
