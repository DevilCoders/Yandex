#include "models_archive_ut_helper.h"

#include <quality/relev_tools/dependency_graph/lib/graph.h>
#include <search/formula_chooser/mappings_io.h>
#include <search/formula_chooser/ruleset_applier.h>


using namespace NFactorDependencies;

const THashSet<NFactor::ETag> UNALLOWED_TAGS = {
    NFactor::ETag::TG_UNUSED,
    NFactor::ETag::TG_REMOVED,
    NFactor::ETag::TG_DEPRECATED,
    NFactor::ETag::TG_UNIMPLEMENTED,
    NFactor::ETag::TG_REUSABLE,
};


void NModelsArchiveValidators::CheckForFactorsByTag(const NFactor::ETag& tagToCheck, const THashMap<NFormulaChooser::NProto::EVariable,
    TVector<NFactor::ETag>>& exclusiveRules, const THashSet<TString>& skipModelIds, bool onlyDefaultMapping) noexcept(false)
{
    const NFactorDependencies::TDependencyGraph& graph = GetDependencyGraph();
    const auto& mappings = NFormulaChooser::GetDefaultMappingsPtr()->GetMapping();

    auto iter = std::find_if(
        mappings.begin(), mappings.end(),
        [&](const NFormulaChooser::NProto::TMapping& m) {return m.GetIsDefault();}
    );
    Y_ENSURE(iter != mappings.end(), "failed to find default mapping");
    ui32 defaultMappingId = iter->GetId();

    ui32 countIncoorectFormulas = 0;
    TStringStream out;
    TGraphTraversalParams params;
    params.DontContinueAfterTags = UNALLOWED_TAGS;

    for (const auto& [vertex, _] : graph.GetAdjacencyList()) {

        if (vertex->VertexType != EVertexType::VT_FORMULA) {
            continue;
        }
        TIntrusivePtr<TFormulaVertex> formula = dynamic_cast<TFormulaVertex*>(vertex.Get());

        if (skipModelIds.contains(formula->FmlId)) {
            continue;
        }

        for (const auto& [role, mappingIds] : formula->FormulaInfo.RolesWithMappings) {

            if (mappingIds.back() < defaultMappingId || (onlyDefaultMapping && AllOf(mappingIds, [&](ui32 id){return id != defaultMappingId;}))) {
                continue;
            }

            bool roleFailed = false;
            TStringStream formulaErrors;

            for (const auto& [vertex, _] : graph.FindDependsOn(formula, params)) {
                if (vertex->VertexType != EVertexType::VT_FACTOR) {
                    continue;
                }

                auto tags = NFactorDependencies::GetTagsFromFactorVertex(vertex);

                bool taggedFactorFound = tags.contains(tagToCheck);
                bool permissionToUseFound = false;

                if (exclusiveRules.contains(role)) {
                    for (const auto& tag : exclusiveRules.at(role)) {
                        if (tags.contains(tag)) {
                            permissionToUseFound = true;
                        }
                    }
                }

                if (taggedFactorFound && !permissionToUseFound) {
                    roleFailed = true;
                    TIntrusivePtr<TFactorVertex> factorVertex = dynamic_cast<TFactorVertex*>(vertex.Get());
                    formulaErrors << "Factor slice: " << ToString(factorVertex->Factor.Slice)
                        << ", idnex: " << factorVertex->Factor.Index << Endl;
                }
            }

            if (roleFailed) {
                out << "Formula (id: " << formula->FmlId << ") with role \"" << role << "\" depends on the factor with the tag "
                    << ToString(tagToCheck) << "\n"
                    << formulaErrors.Str() << Endl;
                ++countIncoorectFormulas;
            }
        }
    }

    Y_ENSURE(countIncoorectFormulas == 0, TString::Join("Failed. ",
             ToString(countIncoorectFormulas), " incorrect formulas.\n", out.Str()));

    return;
}


void NModelsArchiveValidators::CheckDependenciesOnDeprecated(const THashSet<NFactorSlices::EFactorSlice>& slicesWillSkip) noexcept(false) {
    auto tag = NFactor::ETag::TG_DEPRECATED;
    auto graph = NFactorDependencies::GetDependencyGraph();
    const auto adjacencyList = graph.GetAdjacencyList();

    ui32 countIncoorectFactors = 0;
    TStringStream out;

    for (const auto& [vertex, _] : adjacencyList) {
        if (vertex->VertexType != NFactorDependencies::EVertexType::VT_FACTOR) {
            continue;
        }

        if (NFactorDependencies::GetTagsFromFactorVertex(vertex).contains(tag)) {
            TVector<NFactorDependencies::TVertexPtr> incorrectVertexes;

            NFactorDependencies::TGraphTraversalParams params;
            params.DontEnterVertexTypes = {
                EVertexType::VT_FORMULA,
                EVertexType::VT_DSSM,
                EVertexType::VT_FPM
            };
            params.TrunkGraphMode = true;
            for (const auto& [visited, _] : graph.FindDependencies(vertex, params)) {
                if (visited == vertex ||
                    visited->VertexType != NFactorDependencies::EVertexType::VT_FACTOR ||
                    slicesWillSkip.contains(static_cast<TFactorVertex*>(visited.Get())->Factor.Slice)
                ) {
                    continue;
                }
                if (!NFactorDependencies::GetTagsFromFactorVertex(visited).contains(tag)) {
                    incorrectVertexes.push_back(visited);
                }
            }

            if (!incorrectVertexes.empty()) {
                ++countIncoorectFactors;
                TIntrusivePtr<TFactorVertex> factorVertex = static_cast<TFactorVertex*>(vertex.Get());

                out << "Factor {" << ToString(factorVertex->Factor.Slice) << ":" << factorVertex->Factor.Index << "} has "
                     << ToString(tag) << " tag, but the following factors depend on it without this tag: " << Endl;

                for (const auto& v : incorrectVertexes) {
                    auto vv  = static_cast<TFactorVertex*>(v.Get());
                    out << "\t{" << ToString(vv->Factor.Slice) << ":" << vv->Factor.Index << "} " <<
                        NFactorSlices::GetSlicesInfo()->GetFactorsInfo(vv->Factor.Slice)->GetFactorName(vv->Factor.Index) << Endl;
                }
            }
        }
    }

    Y_ENSURE(countIncoorectFactors == 0, "Incorrect vertices were found:\n" << out.Str());
}
