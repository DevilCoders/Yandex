#pragma once

#include <kernel/text_machine/module/interfaces.h>
#include <kernel/text_machine/metadata/tm_metadata.pb.h>
#include <kernel/proto_codegen/codegen.h>

#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/builder.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/generic/list.h>
#include <util/generic/maybe.h>
#include <util/generic/xrange.h>
#include <util/generic/map.h>
#include <util/generic/typetraits.h>

namespace Ngp = ::google::protobuf;

enum EMethodType {
    MT_BASIC = 0x0,
    MT_INLINE = 0x1,
    MT_STATIC = 0x2
};

class TMethodGuard {
public:
    TMethodGuard(IOutputStream& hdr,
        IOutputStream& cpp,
        ui32 methodMask,
        TString baseText,
        TString prefixText,
        TString nameAndArgsText)
        : IsInline(methodMask & MT_INLINE)
        , IsStatic(methodMask & MT_STATIC)
        , Body(IsInline ? hdr : cpp)
        , Indent(IsInline ? "        " : "    ")
    {
        hdr << "\n"
            << "    "
            << (IsInline ? "Y_FORCE_INLINE " : "")
            << (IsStatic ? "static " : "")
            << (prefixText.empty() ? TString() : prefixText + " ")
            << nameAndArgsText;

        if (IsInline) {
            hdr << " {" "\n";
        } else {
            hdr << ";" "\n";
            cpp << "\n"
                << (prefixText.empty() ? TString() : prefixText + " ")
                << baseText << "::" << nameAndArgsText << " { " "\n";
        }
    }

    IOutputStream& Line() {
        return Body << Indent;
    }

    ~TMethodGuard() {
        if (IsInline) {
            Body << "    }" "\n";
        } else {
            Body << "}" "\n";
        }
    }

public:
    bool IsInline;
    bool IsStatic;
    IOutputStream& Body;
    TString Indent;
};

struct TBlockGuard {
    IOutputStream& Out;
    TString Indent;

    TBlockGuard(IOutputStream& out,
        const TString& indent)
        : Out(out)
        , Indent(indent)
    {
        Out << Indent << "{" "\n";
    }
    ~TBlockGuard() {
        Out << Indent << "}" "\n";
    }
};

template <typename TInterfaces, bool AlwaysInline = true>
class TTextMachinePartsCodegen : public NModule::ICodegenHelpers {
private:
    using TCodeGenInput = NTextMachineParts::TCodeGenInput;
    using TMachineDescriptor = TCodeGenInput::TMachineDescriptor;
    using TFeatureDescriptor = TCodeGenInput::TFeatureDescriptor;
    using TUnitDescriptor = TCodeGenInput::TUnitDescriptor;
    using TUnitReference = TCodeGenInput::TUnitReference;
    using TArgDescriptor = TCodeGenInput::TArgDescriptor;
    using TFeatureLevelDescriptor = TCodeGenInput::TFeatureLevelDescriptor;

    const char* Indent = "    ";
    const char* Indent2x = "        ";
    const char* Indent3x = "            ";

    TVector<TString> PreambleLines;
    TVector<TString> Namespaces;
    TVector<TString> UsingNamespaces;

    TVector<TString> MachineDomains;
    TVector<TString> MachineNames;
    THashMap<TString, TMachineDescriptor> Machine2Descr;

    struct TFeatureBundleGuard {
    private:
        template <typename ContType>
        static TString MakeList(const ContType& cont,
            const TString& prefix)
        {
            TString res;
            for (const auto& str : cont) {
                if (!res.empty()) {
                    res += ", ";
                }
                res += prefix + str;
            }
            return res;
        }

        template <typename ContType>
        void ProcessMultiple(const ContType& cont, const TString& prefix,
            THolder<TBlockGuard>& guardHolder, const TString& argName, const TString& indentStep)
        {
            if (cont.size() == 1) {
                ArgsList.push_back(TStringBuilder{} << prefix << *cont.begin());
            } else if (cont.size() > 0) {
                Out << Indent << "for (const auto " << argName << ": {" << MakeList(cont, prefix) << "})" "\n";
                guardHolder.Reset(new TBlockGuard(Out, Indent));
                Indent += indentStep;
                ArgsList.push_back(argName);
           }
       }

    public:
        TFeatureBundleGuard(IOutputStream& out,
            const TFeatureDescriptor& feature,
            const TString& indentStart,
            const TString& indentStep)
            : Out(out)
        {
            Indent = indentStart;

            ProcessMultiple(feature.GetExpansion(), "::NTextMachine::TExpansion::", ExpansionGuard, "expansion", indentStep);
            ProcessMultiple(feature.GetFilter(), "::NTextMachine::TFilter::", FilterGuard, "filter", indentStep);
            ProcessMultiple(feature.GetAccumulator(), "::NTextMachine::TAccumulator::", AccumulatorGuard, "accumulator", indentStep);
            ProcessMultiple(feature.GetNormalizer(), "::NTextMachine::TNormalizer::", NormalizerGuard, "normalizer", indentStep);

            if (feature.HasStream()) {
                ArgsList.push_back(TStringBuilder{} << "::NTextMachine::TStream::" << feature.GetStream());
            }
            if (feature.HasStreamSet()) {
                ArgsList.push_back(TStringBuilder{} << "::NTextMachine::TStreamSet::" << feature.GetStreamSet());
            }
            if (feature.HasStreamValue()) {
                ArgsList.push_back(TStringBuilder{} << "::NTextMachine::TStreamValue::" << feature.GetStreamValue());
            }
            if (feature.HasAlgorithm()) {
                ArgsList.push_back(TStringBuilder{} << "::NTextMachine::TAlgorithm::" << feature.GetAlgorithm());
            }
            if (feature.HasKValue()) {
                ArgsList.push_back(TStringBuilder{} << "::NTextMachine::TKValue(" << feature.GetKValue() << ")");
            }
            if (feature.HasNormValue()) {
                ArgsList.push_back(TStringBuilder{} << "::NTextMachine::TNormValue(" << feature.GetNormValue() << ")");
            }
        }

        IOutputStream& Line() {
            return Out << Indent;
        }
        const TVector<TString>& GetArgsList() const {
            return ArgsList;
        }
        TString GetId() const {
            return "::NTextMachine::TFFId(" + MakeList(ArgsList, "") + ")";
        }

    public:
        IOutputStream& Out;
        TString Indent;
        THolder<TBlockGuard> ExpansionGuard;
        THolder<TBlockGuard> FilterGuard;
        THolder<TBlockGuard> AccumulatorGuard;
        THolder<TBlockGuard> NormalizerGuard;
        TVector<TString> ArgsList;
    };

public:
    void LoadDefinitions(const TCodeGenInput& input) {
        for (size_t machineIndex = 0; machineIndex != input.MachineSize(); ++machineIndex) {
            const TMachineDescriptor& machine = input.GetMachine(machineIndex);
            TInterfaces::ProcessMachineDefinition(machine);
        }
    }

    void SetupNamespaces(const TVector<TString>& namespaces) {
        Namespaces = namespaces;
    }

    void GenCode(TCodeGenInput& input, TCodegenParams& params) {
        PreambleLines.clear();
        CollectPreambleLines(input, PreambleLines);

        UsingNamespaces.clear();
        CollectUsingNamespaces(input, UsingNamespaces);

        MachineNames.clear();
        Machine2Descr.clear();
        CollectMachines(input, MachineDomains, MachineNames, Machine2Descr);
        TransformDescriptions();

        FillTop(params);

        FillUsingDecl(params);

        OpenNamespaces(params);

        FillForwardDecl(params);
        FillMachineDefs(params);

        CloseNamespaces(params);

        FillBottom(params);
    }

private:
    // Utility functions
    //
    static void CollectPreambleLines(const TCodeGenInput& input, TVector<TString>& lines) {
        for (const auto& line : input.GetPreamble().GetLine()) {
            lines.push_back(line);
        }
    }

    static void ExtendMachine(TMachineDescriptor& machine,
        THashMap<TString, TMachineDescriptor>& machineDescr)
    {
        if (machine.GetExtends().empty()) {
            machine.ClearExtends();
            return;
        }

        const auto& baseName = machine.GetExtends();
        Y_ENSURE(machineDescr.contains(baseName), "unknown machine: " << baseName);
        auto& baseDescr = machineDescr.find(baseName)->second;
        Y_ENSURE(baseDescr.GetDomain() == machine.GetDomain(), "base machine should be in same domain");

        ExtendMachine(baseDescr, machineDescr);
        Y_VERIFY(!baseDescr.HasExtends(),
            "(internal error) unexpected field \"Extends\" after preprocessing for machine %s",
            baseDescr.GetName().data());

        TMachineDescriptor fullDescr = baseDescr;
        fullDescr.SetName(machine.GetName());
        fullDescr.MutableUnit()->MergeFrom(machine.GetUnit());
        fullDescr.MutableFeature()->MergeFrom(machine.GetFeature());

        machine.Swap(&fullDescr);
    }

    static void CollectMachines(const TCodeGenInput& input,
        TVector<TString>& machineDomains,
        TVector<TString>& machineNames,
        THashMap<TString, TMachineDescriptor>& machineDescr)
    {
        for (size_t machineIndex = 0; machineIndex != input.MachineSize(); ++machineIndex) {
            const TMachineDescriptor& machine = input.GetMachine(machineIndex);
            Y_ENSURE(!machineDescr.contains(machine.GetName()), "machine name redefined: " << machine.GetName());
            machineNames.push_back(machine.GetName());
            machineDescr[machine.GetName()] = machine;

            const TString domain = machine.GetDomain();
            if (Find(machineDomains.begin(), machineDomains.end(), domain) == machineDomains.end()) {
                machineDomains.push_back(domain);
            }
        }

        for (auto& nameAndDescr : machineDescr) {
            ExtendMachine(nameAndDescr.second, machineDescr);
        }
    }

    void TransformDescriptions() {
        for (auto& nameAndDescr : Machine2Descr) {
            TransformFeatureDescription(nameAndDescr.second);
            TransformFeatureLevelsDescriptions(nameAndDescr.second);
        }
    }

    void CheckFeatureDescriptionsForConflict(const TFeatureDescriptor& a, const TFeatureDescriptor& b) {
        TString conflict;

        (a.HasAlgorithm()   && b.HasAlgorithm()   && (conflict = "Algorithm")) ||
        (a.HasUnit()        && b.HasUnit()        && (conflict = "Unit")) ||
        (a.HasCppName()     && b.HasCppName()     && (conflict = "CppName")) ||
        (a.HasStream()      && b.HasStream()      && (conflict = "Stream")) ||
        (a.HasStreamSet()   && b.HasStreamSet()   && (conflict = "StreamSet")) ||
        (a.HasStreamValue() && b.HasStreamValue() && (conflict = "StreamValue")) ||
        (a.HasKValue()      && b.HasKValue()      && (conflict = "KValue")) ||
        (a.HasNormValue()   && b.HasNormValue()   && (conflict = "NormValue"));

        Y_ENSURE(conflict.empty(),
            "field \"" << conflict << "\" is redefined in feature description: "
               << a.DebugString() << "\n===\n" << b.DebugString());
    }

    void CheckSimpleFeatureDescription(const TFeatureDescriptor& a) {
        Y_ENSURE(a.ExpansionSize() == 0
            && a.FilterSize() == 0
            && a.AccumulatorSize() == 0
            && a.NormalizerSize() == 0,
            "unexpected field in feature description: " << a.ShortDebugString());

        Y_ENSURE(a.HasUnit() && a.HasAlgorithm(),
            "missing field in feature description: " << a.ShortDebugString());
    }

    template<typename T>
    void FillSet(TSet<TString>& set, T& src) {
        set.insert(src.begin(), src.end());
        if (set.empty()) {
            set.insert(TString {});
        }
    }

    template <typename ContType>
    void SplitFeatureGroupByNormalizers(const TFeatureDescriptor& group, ContType& cont) {
        TVector<TString> normalizers;
        bool needSplit = false;

        for (TString normalizer : group.GetNormalizer()) {
            if (normalizer.empty()) {
                needSplit = true;
            } else {
                normalizers.push_back(normalizer);
            }
        }

        if (!needSplit) {
            *cont.Add() = group;
            return;
        }

        if (normalizers.size() > 0) {
            TFeatureDescriptor groupX = group;
            groupX.ClearNormalizer();
            Copy(normalizers.begin(), normalizers.end(),
                ::google::protobuf::RepeatedFieldBackInserter(groupX.MutableNormalizer()));

            *cont.Add() = groupX;
        }

        TFeatureDescriptor groupY = group;
        groupY.ClearNormalizer();

        *cont.Add() = groupY;
    }

    template <typename ContType>
    void CollapseFeatureGroups(const TFeatureDescriptor& group, ContType& cont) {
        for (const auto& feature: group.GetFeature()) {
            TFeatureDescriptor newFeature = group;
            newFeature.ClearFeature();
            newFeature.MergeFrom(feature);
            SplitFeatureGroupByNormalizers(newFeature, cont);
        }

        for (const auto& child : group.GetFeatureGroup()) {
            TFeatureDescriptor newGroup = group;
            newGroup.ClearFeatureGroup();
            newGroup.ClearFeature();
            CheckFeatureDescriptionsForConflict(newGroup, child);
            newGroup.MergeFrom(child);
            CollapseFeatureGroups(newGroup, cont);
        }
    }

    template <typename ContType>
    void CollapseFeatureDimensions(const TFeatureDescriptor& feature, ContType& cont) {
        TFeatureDescriptor featureTemplate = feature;
        featureTemplate.ClearExpansion();
        featureTemplate.ClearFilter();
        featureTemplate.ClearAccumulator();
        featureTemplate.ClearNormalizer();

        TSet<TString> expansions, filters, accumulators, normalizers;

        FillSet(expansions, feature.GetExpansion());
        FillSet(filters, feature.GetFilter());
        FillSet(accumulators, feature.GetAccumulator());
        FillSet(normalizers, feature.GetNormalizer());

        for (auto exp : expansions)
            for (auto filt : filters)
                for (auto acc : accumulators)
                    for (auto norm : normalizers) {
                        auto& simple = *cont.Add();
                        simple = featureTemplate;

                        if (exp) {
                            *simple.MutableExpansion()->Add() = exp;
                        }
                        if (filt) {
                            *simple.MutableFilter()->Add() = filt;
                        }
                        if (acc) {
                            *simple.MutableAccumulator()->Add() = acc;
                        }
                        if (norm) {
                            *simple.MutableNormalizer()->Add() = norm;
                        }
                    }
    }

    void TransformFeatureDescription(TMachineDescriptor& machine) {
        ::google::protobuf::RepeatedPtrField<TFeatureDescriptor> tempFeatures;
        for (auto& group : machine.GetFeatureGroup()) {
            CollapseFeatureGroups(group, tempFeatures);
        }
        for (auto& feature : tempFeatures) {
            CollapseFeatureDimensions(feature, *machine.MutableFeature());
        }

        for (auto& feature : machine.GetFeature()) {
            CheckSimpleFeatureDescription(feature);
        }
    }

    void TransformFeatureLevelsDescriptions(TMachineDescriptor& machine) {
        TSet<TString> knownLevels;

        for (auto& level : *machine.MutableFeatureLevel()) {
            for (TString includedLevelName : level.GetInclude()) {
                Y_ENSURE(includedLevelName != level.GetName(),
                   "recursive include for level \"" << level.GetName() << "\"");

                Y_ENSURE(knownLevels.contains(includedLevelName),
                   "include for unknown level in \"" << level.GetName() << "\""
                   << " (forward references are not supported): "
                   << "\"" << includedLevelName << "\"" << Endl);
            }

            for (auto& group : level.GetFeatureGroup()) {
                CollapseFeatureGroups(group, *level.MutableFeature());
            }

            knownLevels.insert(level.GetName());
        }
    }

    static void CollectUsingNamespaces(const TCodeGenInput& input,
        TVector<TString>& namespaces)
    {
        for (size_t usingIndex = 0; usingIndex != input.UsingSize(); ++usingIndex) {
            namespaces.push_back(input.GetUsing(usingIndex).GetCppNamespace());
        }
    }

    static TString GetUnitCppName(const TString& domain, const TString& name) {
        return GetMachineNamespacePrefix(domain) + "T" + name + "Unit";
    }

    static TString GetUnitFamilyCppName(const TString& domain, const TString& name) {
        return GetMachineNamespacePrefix(domain) + "T" + name + "Family";
    }

    static TString GetUnitStubCppName(const TString& domain, const TString& name) {
        return GetMachineNamespacePrefix(domain) + "T" + name + "Stub";
    }

    static TString GetMachineCppName(const TString& name) {
        return "T" + name + "Machine";
    }

    static TString GetMachineImplNamespace(const TString& name) {
        return "N" + name + "MachinePrivate";
    }

    static TString GetMachineBaseCppName(const TString& name) {
        return "T" + name + "MachineBase";
    }

    static TString GetMachineProcessorStubCppName(const TString& name) {
        return "T" + name + "MachineProcessorStub";
    }

    static TString GetMachineProcessorAccessCppName(const TString& name) {
        return "T" + name + "MachineProcessorAccess";
    }

    static TString GetUnitTraitsCppName(const TString& name) {
        return "T" + name + "UnitTraits";
    }

    static TString GetMachineNamespace(const TString& domain) {
        return "N" + domain + "Parts";
    }

    static TString GetMachineNamespacePrefix(const TString& domain) {
        return (domain ? GetMachineNamespace(domain) + "::" : TString());
    }

    TString GetArg(const TArgDescriptor& arg) const {
        if (arg.HasName()) {
            return GetMachineCppName(arg.GetName());
        } else if (arg.HasCppName()) {
            return arg.GetCppName();
        }
        ythrow yexception() << "arg definition is absent";
    }

    TString GetUnitRef(const TString& domain, const TUnitDescriptor& unit, const TString& indent = "") const {
        TString className;

        if (unit.ArgSize() == 0) {
            className = GetUnitCppName(domain, unit.GetName());
        } else {
            className = GetUnitStubCppName(domain, unit.GetName());
        }

        Y_VERIFY(!!className, "(internal error) empty class name for unit %s", unit.GetName().data());
        if (unit.FamilyArgSize() + unit.ArgSize() == 0) {
            return className;
        }

        TStringStream out;
        out << className << "<" "\n";

        if (unit.FamilyArgSize() > 0) {
            out << indent << Indent << "// Family args" "\n";
        }

        for (size_t argIndex = 0; argIndex != unit.FamilyArgSize(); ++argIndex) {
            out << indent << Indent << unit.GetFamilyArg(argIndex);
            if (argIndex + 1 < unit.FamilyArgSize() + unit.ArgSize()) {
                out << "," "\n";
            }
        }

        if (unit.ArgSize() > 0) {
            out << indent << Indent << "// Structural args" "\n";
        }

        for (size_t argIndex = 0; argIndex != unit.ArgSize(); ++argIndex) {
            out << indent << Indent << GetArg(unit.GetArg(argIndex));
            if (argIndex + 1 < unit.ArgSize()) {
                out << "," "\n";
            }
        }

        out << "\n"
            << indent << ">";
        return out.Str();
    }

    template <typename T>
    TString GetUnitFamilyRef(const TString& domain, const T& unit, size_t argSize) const {
        Y_VERIFY(unit.HasName(), "(internal error) unit has no name");
        TStringStream out;

        if (argSize + unit.FamilyArgSize() == 0) {
            out << GetUnitCppName(domain, unit.GetName());
        } else {
            out << GetUnitFamilyCppName(domain, unit.GetName());
            if (unit.FamilyArgSize() > 0) {
                out << "<";
                for (size_t argIndex = 0; argIndex != unit.FamilyArgSize(); ++argIndex) {
                    if (argIndex > 0) {
                        out << ", ";
                    }
                    out << unit.GetFamilyArg(argIndex);
                }
                out << ">";
            }
        }

        return out.Str();
    }

    template <typename T>
    TString GetUnitVarRef(const TString& domain, const T& unit, size_t argSize) const {
        TStringStream out;
        out << "Vars<" << GetUnitFamilyRef(domain, unit, argSize) << ">()";
        return out.Str();
    }

    template <typename T>
    TString GetUnitProcRef(const TString& domain, const T& unit, size_t argSize) const {
        TStringStream out;
        out << "Proc<" << GetUnitFamilyRef(domain, unit, argSize) << ">()";
        return out.Str();
    }

    template <typename T>
    TString GetUnitVarType(const TString& domain, const T& unit, size_t argSize) const {
        TStringStream out;
        out << "TGetState<" << GetUnitFamilyRef(domain, unit, argSize) << ">";
        return out.Str();
    }

    TString GetUnitProcType(const TString& domain, const TUnitDescriptor& unit, size_t argSize) const override {
        TStringStream out;
        out << "TGetProc<" << GetUnitFamilyRef(domain, unit, argSize) << ">";
        return out.Str();
    }

    void ValidateMachineUnits(const TMachineDescriptor& machine) const
    {
        THashMap<TString, TUnitDescriptor> units;

        for (size_t unitIndex = 0; unitIndex != machine.UnitSize(); ++unitIndex) {
            const auto& unit = machine.GetUnit(unitIndex);

            if (units.contains(unit.GetName())) {
                Y_ENSURE(unit.ArgSize() == units[unit.GetName()].ArgSize(),
                    "two instances of unit " << unit.GetName() << " have different number of structural args");
                Y_ENSURE(unit.FamilyArgSize() == units[unit.GetName()].FamilyArgSize(),
                    "two instances of unit " << unit.GetName() << " have different number of family args");
            } else {
                units[unit.GetName()] = unit;
            }
        }
    }

    TString GetMachineInheritanceString(const TMachineDescriptor& machine,
        const TString& traitsName, const TString& indent = "") const
    {
        TStringStream out;
        for (size_t unitIndex = 0; unitIndex != machine.UnitSize(); ++unitIndex) {
            const auto& unit = machine.GetUnit(unitIndex);
            TString unitRef = GetUnitRef(machine.GetDomain(), unit, indent + Indent);

            out << indent << (0 == unitIndex ? ": " : ", ") << "public " << unitRef
                << "::TProcessor<" << traitsName << "<" << unitIndex << ">>" << "\n";
        }
        return out.Str();
    }

   TString GetMachineAccessHelpersString(const TMachineDescriptor& machine,
        const TString& traitsName, const TString& indent = "") const
   {
        TStringStream out;

        if (machine.UnitSize() == 0) {
            out << Indent << "template <typename Id> class TProcById;" "\n";
            return out.Str();
        }

        for (size_t unitIndex = 0; unitIndex != machine.UnitSize(); ++unitIndex) {
            const auto& unit = machine.GetUnit(unitIndex);
            TString unitRef = GetUnitRef(machine.GetDomain(), unit, indent + Indent);
            TString unitAlias = "TChildUnit" + ToString(unitIndex);

            TString procRef = unitAlias + "::TProcessor<" + traitsName + "<" + ToString(unitIndex) + ">>";
            TString procAlias = "TChildProc" + ToString(unitIndex);

            out << indent << "using " << unitAlias << " = " << unitRef << ";" "\n";
            out << indent << "using " << procAlias << " = " << procRef << ";" "\n";
        }

        out << "\n";

        for (size_t unitIndex = 0; unitIndex != machine.UnitSize(); ++unitIndex) {
            TString unitAlias = "TChildUnit" + ToString(unitIndex);
            TString procAlias = "TChildProc" + ToString(unitIndex);

            out << indent << "static " << procAlias << "& ProcByIdHelper(const " << unitAlias << "::TId&);" "\n";
        }
        out << indent << "template <typename T> static ::NModule::TNullProcessor& ProcByIdHelper(const T&);" "\n";

        out << "\n"
            << Indent << "template <typename Id> using TProcById = std::remove_reference_t<decltype(ProcByIdHelper(Id()))>;" "\n";

        return out.Str();
    }

    // Codegen functions
    //
    void FillTop(TCodegenParams& params) {
        params.Hdr
            << "#pragma once" "\n"
            << "\n"
            << "#include <util/generic/bitmap.h>" "\n"
            << "\n"
            << "#include <kernel/text_machine/module/module_def.inc>" "\n"
            << "\n";

        for (const auto& line : PreambleLines) {
            params.Hdr << line << "\n";
        }
        params.Hdr << "\n";

        params.Cpp
            << "#include \"" << params.HeaderFileName << "\"" "\n"
            << "\n";

        for (TString &ns : Namespaces) {
            params.Cpp << "using namespace " << ns << ";" "\n";
        }
    }

    void FillBottom(TCodegenParams& params) {
        params.Hdr << "#include <kernel/text_machine/module/module_undef.inc>\n";
    }

    void OpenNamespaces(TCodegenParams& params) {
        if (Namespaces.empty()) {
            return;
        }

        params.Hdr << "\n";
        params.Cpp << "\n";

        for (auto it = Namespaces.begin(); it != Namespaces.end(); ++it) {
            params.Hdr << "namespace " << *it << " {" "\n";
            params.Cpp << "namespace " << *it << " {" "\n";
        }
    }

    void CloseNamespaces(TCodegenParams& params) {
        if (Namespaces.empty()) {
            return;
        }

        params.Hdr << "\n";
        params.Cpp << "\n";

        for (auto it = Namespaces.rbegin(); it != Namespaces.rend(); ++it) {
            params.Hdr << "} // namespace " << *it << "\n";
            params.Cpp << "} // namespace " << *it << "\n";
        }
    }

    void FillUsingDecl(TCodegenParams& params) {
        if (UsingNamespaces.empty()) {
            return;
        }

        params.Hdr << "\n";
        for (const TString& usingName : UsingNamespaces) {
            params.Hdr << "using namespace " << usingName << ";" "\n";
        }
    }

    void FillForwardDecl(TCodegenParams&) {
    }

    void FillMachineDef(IOutputStream& hdr, IOutputStream& cpp, bool isInline,
        const TString& name, const TMachineDescriptor& machine)
    {
        ValidateMachineUnits(machine);

        THashMap<TString, TUnitDescriptor> units;

        for (size_t unitIndex = 0; unitIndex != machine.UnitSize(); ++unitIndex) {
            const auto& unit = machine.GetUnit(unitIndex);
            units[unit.GetName()] = unit;
        }

        TString machineDomain = machine.GetDomain();
        const NModule::TProcessorGenerator& procGen = TInterfaces::GetProcessor(machineDomain);

        TString implNamespace = GetMachineImplNamespace(machine.GetName());
        TString machineStubName = GetMachineProcessorStubCppName(machine.GetName());
        TString machineAccessName = GetMachineProcessorAccessCppName(machine.GetName());
        TString unitTraitsName = GetUnitTraitsCppName(machine.GetName());
        TString machineBaseName = GetMachineBaseCppName(machine.GetName());

        hdr << "\n"
            << "namespace " << implNamespace << " {" "\n"
            << "\n"
            << "using namespace " << GetMachineNamespace(machineDomain) << ";" "\n";

        hdr << "\n"
            << "class " << machineStubName << ";" "\n"
            << "class " << machineAccessName << ";" "\n"
            << "class " << machineBaseName << ";" "\n"
            << "template <size_t Id> using " << unitTraitsName << " = ::NModule::TUnitTraits<"
                << machineBaseName << ", " << machineStubName
                << ", " << machineAccessName + ", Id>;" << "\n";

        cpp << "\n"
            << "namespace " << implNamespace << " {" "\n";

        // Processor stub class
        //
        hdr << "\n"
            << "// Processor stub" "\n"
            << "//" "\n"
            << "class " << machineStubName << " {" "\n"
            << "protected:" "\n";

        bool hasGuardedMethods = false;
        for (auto iter = procGen.Begin(); iter != procGen.End(); ++iter) {
            hdr << Indent << iter->GetInlineSignature(false) << " {}" "\n";

            hasGuardedMethods = hasGuardedMethods || iter->IsGuarded();
        }

       if (hasGuardedMethods) {
           hdr << "\n"
               << Indent << "bool IsUnitEnabled() const {" "\n"
               << Indent << Indent << "return IsEnabled;" "\n"
               << Indent << "}" "\n"
               << "\n"
               << Indent << "void SetUnitEnabled(bool f) {" "\n"
               << Indent << Indent << "IsEnabled = f;" "\n"
               << Indent << "}" "\n"
               << "\n"
               << "private:" "\n"
               << Indent << "bool IsEnabled = true;" "\n";
        }

        hdr
            << "};" "\n";

        // Accessor class
        //
        hdr << "\n"
            << "// Processor access" "\n"
            << "//" "\n"
            << "class " << machineAccessName << " {" "\n"
            << "public:" "\n"
            << GetMachineAccessHelpersString(machine, unitTraitsName, Indent)
            << "};" "\n";

        TString machineIheritanceStr = GetMachineInheritanceString(machine, unitTraitsName, Indent);

        hdr << "\n"
            << "class " << machineBaseName << "\n"
            << machineIheritanceStr
            << Indent << (!!machineIheritanceStr ? ", " : ": ") << "public ::NModule::TJsonSerializable" "\n"
            << "{" "\n"
            << "public:" "\n"
            << Indent << "template <typename U> using TGetProc = "
                << machineAccessName << "::TProcById<typename U::TId>;" "\n"
            << Indent << "template <typename U> using TGetState = typename TGetProc<U>::TProcessorState;" "\n"
            << Indent << "template <typename U> using TGetStatic = typename TGetProc<U>::TProcessorStaticState;" "\n"
            << "\n"
            << Indent << "template <typename U> Y_FORCE_INLINE const TGetProc<U>& Proc() const {" "\n"
            << Indent << Indent << "return *this;" "\n"
            << Indent << "}" "\n"
            << "\n"
            << Indent << "template <typename U> Y_FORCE_INLINE TGetProc<U>& Proc() {" "\n"
            << Indent << Indent << "return *this;" "\n"
            << Indent << "}" "\n"
            << "\n"
            << Indent << "template <typename U> Y_FORCE_INLINE const TGetState<U>& Vars() const {" "\n"
            << Indent << Indent << "return *this;" "\n"
            << Indent << "}" "\n"
            << "\n"
            << Indent << "template <typename U> Y_FORCE_INLINE TGetState<U>& Vars() {" "\n"
            << Indent << Indent << "return *this;" "\n"
            << Indent << "}" "\n"
            << "\n"
            << Indent << "template <typename U> Y_FORCE_INLINE static const TGetStatic<U>& Static() {" "\n"
            << Indent << Indent << "return TGetProc<U>::Static();" "\n"
            << Indent << "}" "\n";

        // Unit access by id
        //
        hdr << "\n"
            << "public:" "\n"
            << Indent << "static constexpr size_t NumUnits = " << machine.UnitSize() << ";" "\n"
            << Indent << "using TUnitsMask = TBitMap<NumUnits>;" "\n";

        if (hasGuardedMethods) {
            hdr << "\n"
                << Indent << "Y_FORCE_INLINE void SetUnitsEnabled(const TUnitsMask& mask) {" "\n";

            for (size_t unitIndex : xrange(machine.UnitSize())) {
                const auto& unit = machine.GetUnit(unitIndex);
                const TString procType = GetUnitProcType(machine.GetDomain(), unit, unit.ArgSize());

                hdr << Indent2x << procType << "::SetUnitEnabled(mask.Test(" << unitIndex << "));" "\n";
            }

            hdr << Indent << "}" "\n";

            hdr << "\n"
                << Indent << "Y_FORCE_INLINE void SaveEnabledUnits(TUnitsMask& mask) {" "\n";

            for (size_t unitIndex : xrange(machine.UnitSize())) {
                const auto& unit = machine.GetUnit(unitIndex);
                const TString procType = GetUnitProcType(machine.GetDomain(), unit, unit.ArgSize());

                hdr << Indent2x << "if (" << procType << "::IsUnitEnabled()) mask.Set(" << unitIndex << "); else mask.Reset(" << unitIndex << ");" "\n";
            }

            hdr << Indent << "}" "\n";
        }

        // Main section
        //
        hdr << "\n"
            << "public:" "\n";

        // Ctor
        //
        {
            const ui32 methodType = MT_BASIC;

            TMethodGuard method(hdr, cpp, methodType,
                machineBaseName,
                "",
                machineBaseName + "()");

            method.Line() << "InitStringIds();" "\n";


            TVector<TString> procTypes(machine.UnitSize());

            for (size_t unitIndex : xrange(machine.UnitSize())) {
                const auto& unit = machine.GetUnit(unitIndex);
                const TString procType = GetUnitProcType(machine.GetDomain(), unit, unit.ArgSize());
                const TString procId = TStringBuilder{} << "typename " << procType << "::TId";
                const TString requiredVarName = TStringBuilder{} << "checkRequiredUnits_" << unitIndex;

                procTypes[unitIndex] = procType;

                method.Line() << "(void)(typename " << procType << "::TRequire<0" "\n";
                for (size_t unitIndex2 : xrange(unitIndex)) {
                    method.Line() << Indent << ", " << procTypes[unitIndex2] << "\n";
                }
                method.Line() << "> {});" "\n";
                // method.Line() << "Y_UNUSED(" << requiredVarName << ");" "\n";
            }
        }

        // Processor methods
        //
        for (auto iter = procGen.Begin(); iter != procGen.End(); ++iter) {
            const bool inlineMethod = (isInline && !iter->IsOutOfLineInstatiated()) || iter->HasTypeArgs();
            const bool guardedMethod = iter->IsGuarded();
            const bool hasUnits = machine.UnitSize() > 0;

            THolder<NModule::TProcessorMethodInstantiator> instantiator = iter->CreateExplicitInstantiator();
            if (instantiator) {
                for (size_t unitIndex : xrange(machine.UnitSize())) {
                    instantiator->RegisterUnit(machineDomain, unitIndex, machine.GetUnit(unitIndex).GetName());
                }
                instantiator->PrepareInstantiations();
                if (instantiator->AlwaysForwardUnits().empty()) {
                    hdr << "\n"
                        << Indent << iter->GetInlineSignature(false) << " {}\n";
                } else {
                    hdr << "\n"
                        << Indent << iter->GetInlineSignature(true) << " {\n";
                    for (size_t unitIndex : instantiator->AlwaysForwardUnits()) {
                        const auto& unit = machine.GetUnit(unitIndex);
                        const TString procType = GetUnitProcType(machine.GetDomain(), unit, unit.ArgSize());
                        hdr << Indent2x
                            << (guardedMethod ? TString("if (" + procType + "::IsUnitEnabled()) ") : TString())
                            << procType << "::" << instantiator->GetCallForwarded() << ";\n";
                    }
                    hdr << Indent << "}\n";
                }
                for (auto& inst : instantiator->GetInstantiations()) {
                    NModule::TProcessorMethodInstantiator::TValue instValue = inst.first;
                    const TVector<size_t>& instUnits = inst.second;
                    hdr << "\n"
                        << Indent << instantiator->GetInlineSignatureInstantiated(instValue) << " {\n";
                    for (size_t unitIndex : instUnits) {
                        const auto& unit = machine.GetUnit(unitIndex);
                        const TString procType = GetUnitProcType(machine.GetDomain(), unit, unit.ArgSize());
                        hdr << Indent2x
                            << (guardedMethod ? TString("if (" + procType + "::IsUnitEnabled()) ") : TString())
                            << procType << "::" << instantiator->GetCallInstantiated(instValue) << ";\n";
                    }
                    hdr << Indent << "}\n";
                }
                continue;
            }

            if (inlineMethod) {
                hdr << "\n"
                    << Indent << iter->GetInlineSignature(hasUnits) << " {" "\n";
            } else {
                hdr << "\n"
                    << Indent << iter->GetSignature(hasUnits) << ";" "\n";

                cpp << "\n"
                    << iter->GetOutOfLineSignature(machineBaseName, hasUnits) << " {" "\n";
            }

            const NModule::IMethodCustomGenerator* custom = iter->GetCustomGenerator();
            if (custom) {
                custom->Generate(inlineMethod ? hdr : cpp, machine, *this);
            } else {
                for (size_t unitIndex : xrange(machine.UnitSize())) {
                    const auto& unit = machine.GetUnit(unitIndex);
                    const TString procType = GetUnitProcType(machine.GetDomain(), unit, unit.ArgSize());

                    (inlineMethod ? hdr : cpp)
                        << (inlineMethod ? Indent2x : Indent)
                            << (guardedMethod ? TString("if (" + procType + "::IsUnitEnabled()) ") : TString())
                            << procType << "::" << iter->GetCall() << ";" << "\n";
                }
            }

            if (inlineMethod) {
                hdr << Indent << "}" "\n";
            } else {
                cpp << "}" "\n";
            }
        }

        // InitStringIds
        //
        {
            const ui32 methodType = MT_BASIC;

            TMethodGuard method(hdr, cpp, methodType,
                machineBaseName,
                "void",
                "InitStringIds()");

            for (size_t unitIndex : xrange(machine.UnitSize())) {
                const auto& unit = machine.GetUnit(unitIndex);
                TString varRef = GetUnitVarType(machine.GetDomain(), unit, unit.ArgSize());
                TString scopelessFamilyRef = GetUnitFamilyRef("", unit, unit.ArgSize());

                method.Line() << varRef << "::StringId = " << "\"" << scopelessFamilyRef << "\";" "\n";
            }
        }

        // GetNumFeatures
        //
        {
            const ui32 methodType = MT_STATIC | MT_INLINE;

            TMethodGuard method(hdr, cpp, methodType,
                machineBaseName,
                "size_t",
                "GetNumFeatures()");

            method.Line() << "return " << machine.FeatureSize() << ";" "\n";
        }

        // GetFeatureIds
        //
        {
            const ui32 methodType = MT_STATIC;

            TMethodGuard method(hdr, cpp, methodType,
                machineBaseName,
                "const TFFIds&",
                "GetFeatureIds()");

            method.Line() << "static const TFFIds ids = PrepareFeatureIds();" "\n";
            method.Line() << "return ids;" "\n";
        }

        // PrepareFeatureIds
        //
        {
            const ui32 methodType = MT_STATIC;

            TMethodGuard method(hdr, cpp, methodType,
                machineBaseName,
                "TFFIds",
                "PrepareFeatureIds()");

            method.Line() << "TFFIds ids;" "\n";
            method.Line() << "ids.resize(" << machine.FeatureSize() << ");" "\n";

            TSet<TString> knownIds;

            for (size_t featureIndex = 0; featureIndex != machine.FeatureSize(); ++featureIndex) {
                TFeatureBundleGuard feature(method.Body, machine.GetFeature(featureIndex), method.Indent, Indent);

                TString featureId = feature.GetId();
                Y_ENSURE(knownIds.insert(featureId).second,
                    "machine \"" << name << "\" has duplicate feature id: " << featureId);

                feature.Line() << "ids[" << featureIndex << "] = " << featureId << ";" "\n";
            }
            method.Line() << "return ids;" "\n";
        }

        // CalcFeatures
        //
        {
            const ui32 methodType = (isInline ? MT_INLINE : MT_BASIC);

            TMethodGuard method(hdr, cpp, methodType,
                machineBaseName,
                "void",
                "CalcFeatures(TVector<float>& features)");

            method.Line() << "features.resize(GetNumFeatures());" "\n";
            method.Line() << "TFloatsBuffer buffer(features.data(), features.size());" "\n";
            method.Line() << "CalcFeatures(buffer);" "\n";
        }

        // CalcFeatures to buffer
        //
        {
            const ui32 methodType = MT_BASIC;

            TMethodGuard method(hdr, cpp, methodType,
                machineBaseName,
                "void",
                "CalcFeatures(TFloatsBuffer& features)");

            method.Line() << "Y_ASSERT(features.Avail() >= GetNumFeatures());" "\n";

            for (size_t featureIndex : xrange(machine.FeatureSize())) {
                const auto& feature = machine.GetFeature(featureIndex);
                Y_ENSURE(feature.HasUnit(), "unit reference is missing in feature descriptor");
                const auto& unitRef = feature.GetUnit();
                Y_ENSURE(units.contains(unitRef.GetName()), "undefined unit: " << unitRef.GetName());
                const auto& unitDescr = units[unitRef.GetName()];
                Y_ENSURE(unitRef.FamilyArgSize() == unitDescr.FamilyArgSize(),
                    "unit and reference for " << unitRef.GetName() << " have different number of family args");
                Y_ENSURE(feature.HasAlgorithm(), "algorithm is missing in feature descriptor");

                method.Line()
                        << "// " << unitRef.GetName() << "::"
                        << (feature.HasAlgorithm() ? feature.GetAlgorithm() : feature.GetCppName())
                        << (feature.HasStream() ? " for stream " + feature.GetStream() : TString())
                        << (feature.HasStreamSet() ? " for stream set " + feature.GetStreamSet() : TString())
                        << (feature.HasStreamValue() ? " with value remap " + feature.GetStreamValue() : TString())
                        << "\n";


                TVector<TString> calcArgs;
                if (feature.HasKValue()) {
                    calcArgs.push_back(ToString(feature.GetKValue()));
                }
                if (feature.HasNormValue()) {
                    calcArgs.push_back(ToString(feature.GetNormValue()));
                }

                method.Line()
                        << "features.Add("
                        << GetUnitProcType(machine.GetDomain(), unitDescr, unitDescr.ArgSize())
                        << (feature.HasCppName() ? TString("::" + feature.GetCppName()) : TString("::Calc" + feature.GetAlgorithm()))
                        << "(" << JoinSeq(", ", calcArgs) << "));" "\n";
            }
        }

        // GetLevelFilterHelper
        //
        {
            const ui32 methodType = MT_STATIC;

            TMethodGuard method(hdr, cpp, methodType,
                                machineBaseName,
                                "THashSet<::NTextMachine::TFFId>",
                                "GetLevelFilterHelper(const TStringBuf& levelName)");

            method.Line() << "Y_UNUSED(levelName);" "\n"; //for empty FeatureFilter
            for (const auto& level : machine.GetFeatureLevel()) {
                TString levelSetName = "ids_" + level.GetName();
                method.Line() << "THashSet<::NTextMachine::TFFId> " << levelSetName << ";" "\n";

                for (const auto& include : level.GetInclude()) {
                    TString includedLevelSetName = "ids_" + include;
                    method.Line() << levelSetName << ".insert(" << includedLevelSetName << ".begin(), "
                        << includedLevelSetName << ".end());" "\n";
                }

                for (const auto& descr: level.GetFeature()) {
                    TFeatureBundleGuard feature(method.Body, descr, method.Indent, Indent);
                    feature.Line() << levelSetName << ".insert(" << feature.GetId() << ");" "\n";
                }

                method.Line() << "if (levelName == \"" << level.GetName() << "\") {" "\n";
                method.Line() << Indent << "return " << levelSetName << ";" "\n";
                method.Line() << "}" "\n";
            }

            method.Line() << "\n";
            method.Line() << "return THashSet<::NTextMachine::TFFId>();" "\n";
        }

        // GetLevelFilter
        //
        {
            const ui32 methodType = MT_STATIC;

            TMethodGuard method(hdr, cpp, methodType,
                machineBaseName,
                "TFeaturesFilter",
                "GetLevelFilter(TStringBuf levelName)");

            method.Line() << "static THashMap<TStringBuf, THashSet<::NTextMachine::TFFId>> idsByLevel = {" "\n";
            for (const auto& level : machine.GetFeatureLevel()) {
                bool isLast = (machine.GetFeatureLevel().rbegin()->GetName() == level.GetName());
                method.Line() << Indent << "{ \"" << level.GetName() << "\"sv, GetLevelFilterHelper(\"" << level.GetName() << "\") }"
                    << (isLast ? "" : ",") << "\n";
            }
            method.Line() << "};" "\n";

            method.Line() << "if (levelName == \"all\") {" "\n";
            method.Line() << Indent << "return MakeAcceptAllFeaturesFilter();" "\n";
            method.Line() << "}" "\n";

            method.Line() << "auto iter = idsByLevel.find(levelName);" "\n";
            method.Line() << "return (iter == idsByLevel.end() ? MakeAcceptNoneFeaturesFilter() : MakeFeaturesFilterFromSetRef(iter->second));" "\n";
        } // TFeaturesFilter GetLevelFilter(const TStringBuf& levelName)

        hdr << "};" "\n"
            << "\n"
            << "} // " << implNamespace << "\n";

        cpp << "\n"
            << "} // " << implNamespace << "\n";

        hdr << "\n"
            << "using " << GetMachineCppName(name) << " = " << GetMachineNamespacePrefix(machineDomain) << "TMotor<"
                << implNamespace << "::" << GetMachineBaseCppName(machine.GetName()) << ">;" "\n";
    }

    void FillMachineDefs(TCodegenParams& params) {
        for (const TString& name : MachineNames) {
            Y_VERIFY(Machine2Descr.contains(name), "(internal error) missing description for machine %s", name.data());
            FillMachineDef(params.Hdr, params.Cpp, AlwaysInline, name, Machine2Descr[name]);
        }
    }
};
