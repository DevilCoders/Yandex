// Manual: http://wiki.yandex-team.ru/BaseSearch/StaticFeaturesDescriptionLanguage

#include "common.h"

#include <kernel/relevfml/relev_fml.h>
#include <kernel/web_factors_info/factor_names.h>

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/string/split.h>
#include <util/string/printf.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

class TFactorMap {
    THashMap<TString, int> Name2Index;
    TVector<TString> Names;

public:
    TFactorMap() {
        for (size_t i = 0; i < GetFactorCount(); i++) {
            Names.push_back(GetFactorInternalName(i));
            Name2Index[Names[i]] = i;
        }
    }

    int Size() const {
        return Names.size();
    }

    bool HasFactorIndex(int i) const {
        return i < (int)Names.size();
    }

    bool HasFactor(const TString& name) const {
        return Name2Index.contains(name);
    }

    const TString& GetFactorName(int i) {
        Y_VERIFY(HasFactorIndex(i), "factor %d doesn't exist", i);
        return Names[i];
    }

    int GetFactorIndex(const TString& name) {
        Y_VERIFY(HasFactor(name), "factor %s doesn't exist", name.c_str());
        return Name2Index[name];
    }

    bool IsStaticFactor(int i) const {
        Y_VERIFY(HasFactorIndex(i), "factor %d doesn't exist", i);
        return ::IsStaticFactor(i);
    }

    bool IsLocalizedFactor(int i) const {
        Y_VERIFY(HasFactorIndex(i), "factor %d doesn't exist", i);
        return ::IsLocalizedFactor(i);
    }

    bool IsDeprecatedFactor(int i) const {
        Y_VERIFY(HasFactorIndex(i), "factor %d doesn't exist", i);
        return  GetWebFactorsInfo()->IsDeprecatedFactor(i);
    }

    bool IsUnimplementedFactor(int i) const {
        Y_VERIFY(HasFactorIndex(i), "factor %d doesn't exist", i);
        return  GetWebFactorsInfo()->IsUnimplementedFactor(i);
    }

    bool IsStaticReginfoFactor(int i) const {
        Y_VERIFY(HasFactorIndex(i), "factor %d doesn't exist", i);
        return ::IsStaticReginfoFactor(i);
    }
};

static TStringStream& CerrBuffer() {
    static TStringStream buffer;
    return buffer;
}

static void AbordWithDump() {
    Cerr << CerrBuffer().Str();
    Cerr.Flush();
    std::terminate();
}

TBlock* TState::FindBlock(const TString& name) {
    return Name2Block.contains(name) ? Name2Block[name] : nullptr;
}

TBlock* TState::GetBlock(const TString& name) {
    TBlock* block = FindBlock(name);
    if (block == nullptr) {
        CerrBuffer() << "Unknown block " << name << Endl;
        AbordWithDump();
    }
    return block;
}

void TState::AddBlock(TBlock* block) {
    if (Name2Block.contains(block->Id)) {
        CerrBuffer() << "Error: duplicated block " << block->Id << Endl;
        AbordWithDump();
    }
    Blocks.push_back(block);
    block->Index = Blocks.size() - 1;
    Name2Block[block->Id] = block;
}

TState::~TState() {
    for (size_t i = 0; i < Blocks.size(); i++)
        delete Blocks[i];
}

// Finds all transitive dependencies of a block, detects dependency cycles
class TClosureCalculator {
    const TVector<TBlock*>& Blocks;
    TVector<int> Seen;

    void DFS(const TBlock* block) {
        size_t i1 = block->Index;
        Y_VERIFY(block != nullptr && i1 < Seen.size(), ".");

        if (Seen[i1] == 0) {
            Seen[i1] = 1;
            for (size_t j = 0; j < block->DepBlocks.size(); j++)
                DFS(block->DepBlocks[j]);
            Seen[i1] = 2;
        } else if (Seen[i1] == 1) {
            CerrBuffer() << "Circular dependencies among:";
            for (size_t i2 = 0; i2 < Blocks.size(); i2++) {
                if (Seen[i2] == 1) {
                    CerrBuffer() << " " << Blocks[i2]->Id;
                }
            }
            CerrBuffer() << Endl;
            AbordWithDump();
        }
    }

public:
    TClosureCalculator(TVector<TBlock*>& blocks)
        : Blocks(blocks)
    {
    }

    TVector<TBlock*> GetTransitiveDependencies(const TBlock* block) {
        Seen.assign(Blocks.size(), 0);
        DFS(block);

        TVector<TBlock*> res;
        for (size_t j = 0; j < Blocks.size(); j++) {
            if (Seen[j] && j != block->Index) {
                res.push_back(Blocks[j]);
            }
        }
        return res;
    }
};

void Enrich(TState* state) {
    TFactorMap* factorMap = Singleton<TFactorMap>();
    bool errors = false;

    // Compute indexes of factors provided by each block,
    // fill Factor2Block and AvailableFactorIndexes
    for (size_t i = 0; i < state->Blocks.size(); i++) {
        TBlock* block = state->Blocks[i];

        for (size_t j = 0; j < block->Factors.size(); j++) {
            const TString& name = block->Factors[j];
            if (!factorMap->HasFactor(name)) {
                CerrBuffer() << "Error: factor name " << name << " from input file is unknown" << Endl;
                errors = true;
                continue;
            }

            int index = factorMap->GetFactorIndex(name);
            block->FactorIndexes.push_back(index);

            if (!factorMap->IsStaticFactor(index)) {
                CerrBuffer() << "Error: factor " << name << " (" << index << ") from input file was not marked static" << Endl;
                errors = true;
            }

            if (state->Factor2Block.contains(index) && state->Factor2Block[index] != block) {
                CerrBuffer() << "Error: factor " << name << " is computed by multiple blocks" << Endl;
                errors = true;
            } else {
                state->Factor2Block[index] = block;
                state->AvailableFactorsIndexes.push_back(index);
            }
        }

        Sort(block->FactorIndexes.begin(), block->FactorIndexes.end());
    }

    // Fill DepBlocks with direct dependencies
    for (size_t i1 = 0; i1 < state->Blocks.size(); i1++) {
        TBlock* block = state->Blocks[i1];

        for (size_t j = 0; j < block->Deps.size(); j++) {
            TString dep = block->Deps[j];

            // strip field name for <struct>.<field>
            size_t i2 = dep.find('.');
            if (i2 != TString::npos)
                dep = dep.substr(0, i2);

            TBlock *b = state->FindBlock(dep);
            if (b != nullptr) {
                block->DepBlocks.push_back(b);
            } else if (dep.StartsWith("FI_") && factorMap->HasFactor(dep)) {
                int factor = factorMap->GetFactorIndex(dep);
                if (state->IsFactorAvailable(factor)) {
                    block->DepBlocks.push_back(state->Factor2Block[factor]);
                } else {
                    CerrBuffer() << "Error: block " << block->Id << " depends on unavailable factor " << dep << Endl;
                    errors = true;
                }
            } else {
                CerrBuffer() << "Error: block " << block->Id << " has an unknown dependency " << b->Id << Endl;
                errors = true;
            }
        }
    }

    // compute all factors which can be calced based on block data
    for (size_t i = 0; i < state->Blocks.size(); i++) {
        TBlock* block = state->Blocks[i];

        if (block->DepBlocks.size() != 1)
            continue;

        for (size_t j = 0 ; j < block->DepBlocks.size() ; ++j) {
            TBlock& parentBlock = *block->DepBlocks[j];

            for (size_t k = 0 ; k < block->FactorIndexes.size() ; ++k) {
                parentBlock.ChildFactorIndexes.push_back(block->FactorIndexes[k]);
            }
        }
    }

    // compute transitive closure, add all dependencies to DepBlocks
    for (size_t i = 0; i < state->Blocks.size(); i++) {
        TBlock* block = state->Blocks[i];
        block->DepBlocks = TClosureCalculator(state->Blocks).GetTransitiveDependencies(block);
    }

    // get indexes of factors mentioned in fastrank_factors
    for (size_t i = 0; i < state->FastRankFactors.size(); i++) {
        if (!factorMap->HasFactor(state->FastRankFactors[i])) {
            CerrBuffer() << "Error: unknown factor " << state->FastRankFactors[i] << " in fastrank_factors directive" << Endl;
            errors = true;
        } else {
            int factor = factorMap->GetFactorIndex(state->FastRankFactors[i]);
            if (!state->IsFactorAvailable(factor)) {
                CerrBuffer() << "Error: factor " << state->FastRankFactors[i] << " is not defined in " << state->InputFile << Endl;
                errors = true;
            } else {
                state->FastRankFactorsIndexes.push_back(factorMap->GetFactorIndex(state->FastRankFactors[i]));
            }
        }
    }

    // get indexes of factors mentioned in external_factors
    for (size_t i = 0; i < state->ExternalFactors.size(); i++) {
        if (factorMap->HasFactor(state->ExternalFactors[i])) {
            state->ExternalFactorsIndexesHash.insert(factorMap->GetFactorIndex(state->ExternalFactors[i]));
        } else {
            CerrBuffer() << "Warning: unknown factor " << state->ExternalFactors[i] << " in external_factors directive" << Endl;
        }
    }

    // check for unaccounted static factors
    bool unaccountedBegin = true;
    size_t unaccountedCount = 0;
    for (int i = 0; i < factorMap->Size(); i++) {
        if (factorMap->IsStaticFactor(i) &&
            !factorMap->IsDeprecatedFactor(i) &&
            !factorMap->IsUnimplementedFactor(i) &&
            !factorMap->IsLocalizedFactor(i) &&
            !factorMap->IsStaticReginfoFactor(i) &&
            !state->IsFactorAvailable(i) &&
            !state->ExternalFactorsIndexesHash.contains(i))
        {
            if (unaccountedBegin) {
                CerrBuffer() << "Warning: following static factors are not accounted for in " << state->InputFile.Quote() << Endl;
                unaccountedBegin = false;
            }
            CerrBuffer() << "(" << RightPad(i, 4) << ") " << RightPad(factorMap->GetFactorName(i), 50);
            ++unaccountedCount;

            if (unaccountedCount % 2 == 0)
                CerrBuffer() << Endl;
        }
    }
    if (unaccountedCount % 2)
        CerrBuffer() << Endl;

    if (errors) {
        CerrBuffer() << "Errors have been encountered during processing of " << state->InputFile << Endl;
        AbordWithDump();
    }
}

template<typename TIter>
TString CompressIntegerRange(TIter begin, TIter end) {
    TVector<int> v(begin, end);
    Sort(v.begin(), v.end());
    v.erase(v.end(), Unique(v.begin(), v.end()));

    TString res;
    int n = v.size();
    for (int i = 0; i < n;) {
        int j = i + 1;
        while (j < n && v[j] == v[i] + j - i)
            j++;
        if (j - i == 2)
            j = i + 1;
        if (i != 0)
            res += " ";
        if (j == i + 1)
            res += ToString(v[i]);
        else
            res += Sprintf("%d-%d", v[i], v[j-1]);
        i = j;
    }

    return res;
}

struct TBlocksCmp {
    bool operator()(const TBlock* a, const TBlock* b) const {
        if (a->Rank != b->Rank)
            return a->Rank < b->Rank;

        int firstRequiredA = a->RequiredFactorIndexes.size() == 0 ? INT_MAX : a->RequiredFactorIndexes[0];
        int firstRequiredB = b->RequiredFactorIndexes.size() == 0 ? INT_MAX : b->RequiredFactorIndexes[0];

        if (firstRequiredA != firstRequiredB)
            return firstRequiredA < firstRequiredB;

        return a->Index < b->Index;
    }
};

void Emit(TBlock* block) {
    if (block == nullptr || block->Emitted)
        return;

    // first process not yet emitted dependencies in order of their rank
    TVector<std::pair<int, TBlock*> > deps;
    for (size_t i = 0; i < block->DepBlocks.size(); i++) {
        TBlock* b = block->DepBlocks[i];
        if (!b->Emitted)
            deps.push_back(std::pair<int, TBlock*>(b->Rank, b));
    }

    if (!deps.empty()) {
        Sort(deps.begin(), deps.end());
        for (size_t i = 0; i < deps.size(); i++)
            Emit(deps[i].second);
    }

    // emit code, add comments with factor indexes

    if (block->Type == TBlock::USER_BLOCK && !block->RequiredFactorIndexes.empty()) {
        Cout << "    //";
        for (size_t i = 0; i < block->RequiredFactorIndexes.size(); i++) {
            int factor = block->RequiredFactorIndexes[i];
            Cout << " " << factor;
        }
        Cout << Endl;
    }

    if (!block->Code.StartsWith(' ') && !block->Code.StartsWith("\n"))
        Cout << "    ";

    Cout << block->Code;

    if (block->Type == TBlock::SIMPLE_FACTOR || block->Type == TBlock::EXPR_FACTOR) {
        Cout << "  // " + ToString(block->FactorIndexes[0]) << "\n";
    } else if (!block->Code.EndsWith("\n")) {
        Cout << "\n";
    }

    block->Emitted = true;
}

void Generate(TState* state, TString funcname, TVector<int>* requested_factors, bool zeroize) {
    TFactorMap* factorMap = Singleton<TFactorMap>();

    for (size_t i = 0; i < state->Blocks.size(); i++) {
        TBlock* block = state->Blocks[i];
        block->Required = 0;
        block->Emitted = false;
    }

    bool all_factors = requested_factors == nullptr;
    if (all_factors)
        requested_factors = &state->AvailableFactorsIndexes;

    THashSet<int> required_factors_set;
    required_factors_set.insert(requested_factors->begin(), requested_factors->end());

    // process dependencies, mark required block and factors.
    // we might have to compute more factors than requested.
    for (size_t i = 0; i < requested_factors->size(); i++) {
        int factor = requested_factors->at(i);
        if (!state->IsFactorAvailable(factor)) {
            CerrBuffer() << "Error: requested factor " << factor << " ("
                 << (factorMap->HasFactorIndex(factor) ? factorMap->GetFactorName(factor) : "?")
                 << ") is not available in " << state->InputFile << Endl;
            AbordWithDump();
        }

        TBlock* block = state->Factor2Block[factor];
        block->Required = true;

        for (size_t j = 0; j < block->DepBlocks.size(); j++) {
            TBlock *dep = block->DepBlocks[j];
            dep->Required = true;

            // ok, it might be an overkill to require all factors in a block just
            // because some requested factor depends on some some factor (out of several)
            // in that block, but at least this guarantees correctness
            required_factors_set.insert(dep->FactorIndexes.begin(), dep->FactorIndexes.end());
        }
    }

    // mark prologue/epilogue blocks as required, if they are present
    for (int i = 0; i < 2; i++) {
        TBlock* block = state->FindBlock(i == 0 ? "prologue" : "epilogue");
        if (block != nullptr) {
            block->Required = true;
            if (!block->DepBlocks.empty() || !block->FactorIndexes.empty()) {
                CerrBuffer() << "Error: block " << block->Id << " cannot have dependencies or compute any factor" << Endl;
                AbordWithDump();
            }
        }
    }

    // report to caller factors which we are ultimately going to compute
    if (!all_factors) {
        requested_factors->clear();
        requested_factors->insert(requested_factors->end(), required_factors_set.begin(), required_factors_set.end());
    }

    // rank required blocks as follows:
    //   prologue
    //   structures
    //   factor blocks ordered by first required factor index,
    //   non-factor blocks ordered by appearance in the input
    //   epilogue
    TVector<TBlock*> ranked;
    {
        for (size_t i = 0; i < state->Blocks.size(); i++) {
            TBlock* block = state->Blocks[i];
            if (!block->Required)
                continue;

            if (block->Id == "prologue")
                block->Rank = 0;
            else if (block->Id == "epilogue")
                block->Rank = 9;
            else if (block->Type == TBlock::STRUCTURE_BLOCK)
                block->Rank = 1;
            else if (!block->FactorIndexes.empty())
                block->Rank = 2;
            else
                block->Rank = 3;

            block->RequiredFactorIndexes.clear();
            for (size_t j = 0; j < block->FactorIndexes.size(); j++) {
                int factor = block->FactorIndexes[j];
                if (required_factors_set.contains(factor))
                    block->RequiredFactorIndexes.push_back(factor);
            }

            ranked.push_back(block);
        }

        Sort(ranked.begin(), ranked.end(), TBlocksCmp());

        for (size_t i = 0; i < ranked.size(); i++)
            ranked[i]->Rank = i;
    }

    Cout << "// Computes " << required_factors_set.size() << " factors: ";
    Cout << CompressIntegerRange(required_factors_set.begin(), required_factors_set.end()) << Endl;

    Cout << "template <class TErfManagerType>\n";
    Cout << "void " + funcname + "(" << state->ArgList << ")" << " {\n";

    // emit NEED_<factor> constants
    for (int factor = 0; factor < factorMap->Size(); factor++) {
        if (!state->IsFactorAvailable(factor))
            continue;

        TString name = factorMap->GetFactorName(factor);
        TString need_name = "NEED_" + name;

        // only emit actually used constants
        bool has_need = false;
        for (size_t i = 0; i < ranked.size() && !has_need; i++)
            has_need |= ranked[i]->Code.find(need_name) != TString::npos;
        if (!has_need)
            continue;

        TString s = "    static const bool " + need_name + " = ";
        s += required_factors_set.contains(factor) ? "true" : "false";
        s += ";";
        while (s.size() < 70)
            s += " ";
        Cout << s << "  // " << factor << "\n";
    }
    Cout << "\n";

    Emit(state->FindBlock("prologue"));

    for (size_t i = 0; i < ranked.size(); i++)
        if (ranked[i]->Type == TBlock::STRUCTURE_BLOCK)
            Emit(ranked[i]);

    Cout << "\n";
    if (zeroize)
        Cout << "    factor.Clear();\n\n";

    for (size_t i = 0; i < ranked.size(); i++)
        Emit(ranked[i]);

    Cout << "}" << Endl << Endl;

    // TODO: implement the following optimization:
    // if (em.GetErfCount() == 0) {
    //    compute non-erf factors (tfdoc factors, pageranks, FI_LONG)
    //    set FI_DIFFERENT_INTERNAL_LINKS, FI_IS_NOT_RU and FI_URL_HAS_NO_DIGITS to 1.0
    //    return
    // }
}

struct TFastRankInfo {
    TString Name;
    TVector<int> Factors;
};

void GenerateFast(TState* state, const TVector<TFastRankInfo>& fastranks) {
    TFactorMap* factorMap = Singleton<TFactorMap>();

    for (size_t i = 0; i < fastranks.size(); i++) {
        const TString& name = fastranks[i].Name;
        Cout << "class TFastStaticFeaturesCalculator_" << name << " : public IFastStaticFeaturesCalculator {\n";
        Cout << "public:\n";
        Cout << "   void PreFastRank(" << state->ArgList << ") const;\n";
        Cout << "   void PostFastRank(" << state->ArgList << ") const;\n";
        Cout << "   const char* GetName() {\n";
        Cout << "       return \"" << name << "\";\n";
        Cout << "   }\n";
        Cout << "};\n";
        Cout << "static TFastStaticFeaturesCalculator_" << name << " FastStaticFeaturesCalculator_" << name << ";\n\n";
    }

    Cout << "#include <cstring>\n";
    Cout << "IFastStaticFeaturesCalculator* IFastStaticFeaturesCalculator::GetInstance(const char* formula) {\n";
    Cout << "    if (formula == NULL) return NULL;\n";
    for (size_t i = 0; i < fastranks.size(); i++) {
        const TString& name = fastranks[i].Name;
        Cout << "    if (strcmp(formula, \"" << name << "\") == 0)\n";
        Cout << "        return &FastStaticFeaturesCalculator_" << name << ";\n";
    }
    if (fastranks.empty())
        Cout << "    Y_UNUSED(formula);\n";
    Cout << "    return NULL;\n";
    Cout << "}\n\n";

    for (size_t i = 0; i < fastranks.size(); i++) {
        const TString& name = fastranks[i].Name;
        TVector<int> factors = fastranks[i].Factors;
        factors.insert(factors.end(), state->FastRankFactorsIndexes.begin(), state->FastRankFactorsIndexes.end());

        // filter factors that aren't described in the input file
        {
            TVector<int> tmp;
            for (size_t k = 0; k < factors.size(); k++) {
                if (state->IsFactorAvailable(factors[k]))
                    tmp.push_back(factors[k]);
            }
            factors = tmp;
        }

        {
            TString funcName = "TFastStaticFeaturesCalculator_" + name + "_PreFastRank";
            Generate(state, funcName, &factors, true);
            Cout << "void TFastStaticFeaturesCalculator_" + name + "::PreFastRank" + "(" << state->ArgList << ") const" << " {\n";
            Cout << "    " << funcName << "(docId, factor, em, csfp);" << Endl;
            Cout << "}" << Endl << Endl;
        }

        // invert factors selection
        {
            THashSet<int> computed(factors.begin(), factors.end());
            TVector<int> tmp;
            for (int k = 0; k < factorMap->Size(); k++) {
                if (state->IsFactorAvailable(k) && !computed.contains(k))
                    tmp.push_back(k);
            }
            factors = tmp;
        }

        {
            TString funcName = "TFastStaticFeaturesCalculator_" + name + "_PostFastRank";
            Generate(state, funcName, &factors, false);
            Cout << "void TFastStaticFeaturesCalculator_" + name + "::PostFastRank" + "(" << state->ArgList << ") const" << " {\n";
            Cout << "    " << funcName << "(docId, factor, em, csfp);" << Endl;
            Cout << "}" << Endl << Endl;
        }
    }
}

void GenerateCalcFuncForStructBlocks(TState* state) {
    for (size_t i = 0 ; i < state->Blocks.size() ; ++i) {
        TBlock& block = *state->Blocks[i];
        if (block.Type != TBlock::STRUCTURE_BLOCK)
            continue;
        Generate(state, "CalcStaticFeatures_" + block.Id, &block.ChildFactorIndexes, true);
    }
}

int main(int argc, char *argv[]) {
    NLastGetopt::TOpts opts;
    opts.AddHelpOption('?');
    opts.AddCharOption('i', "factors description file. \"-\" denotes stdin.").RequiredArgument("file").DefaultValue("-");
    opts.AddCharOption('a', "arcadia root directory").RequiredArgument("file").Required();

    const NLastGetopt::TOptsParseResult optsres(&opts, argc, argv);
    TString arcadia = optsres.Get('a');
    TString filename = optsres.Get('i');

    TState* state = ParseFile(filename);
    Enrich(state);

    Generate(state, "CalcStaticFeaturesAutoGenerated", nullptr, true);
    Generate(state, "CalcStaticFeaturesAutoGeneratedNoClear", nullptr, false);

    TVector<TFastRankInfo> fastranks;

    for (size_t i1 = 0; i1 < state->FastRankFiles.size(); i1++) {
        TUnbufferedFileInput in(arcadia + "/" + state->FastRankFiles[i1]);
        TString line;

        while (in.ReadLine(line)) {
            if (line.empty())
                continue;
            if ('#' == line[0])
                continue;

            TVector<TString> fields;
            StringSplitter(line).Split(' ').SkipEmpty().Collect(&fields);
            Y_ASSERT(fields.size() >= 4);

            TString name = fields[1];
            TString fml = fields[2];
            int nFactors = FromString<int>(fields[3]);
            SRelevanceFormula formula;
            Decode(&formula, fml, nFactors);

            TVector<TVector<int> > monomials;
            TVector<float> weights;
            formula.GetFormula(&monomials, &weights);

            THashSet<int> factors_set;
            for (size_t i2 = 0; i2 < monomials.size(); i2++)
                factors_set.insert(monomials[i2].begin(), monomials[i2].end());

            fastranks.resize(fastranks.size() + 1);
            fastranks.back().Name = name;
            fastranks.back().Factors = TVector<int>(factors_set.begin(), factors_set.end());
        }
    }

    GenerateFast(state, fastranks);

    GenerateCalcFuncForStructBlocks(state);

    if (!filename.Contains("dummy_arcadia") && !CerrBuffer().Str().empty()) {
        AbordWithDump();
    }

    return 0;
}
