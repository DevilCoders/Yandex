#include "relev_fml_codegen.h"

#include <kernel/relevfml/relev_fml.h>
#include <kernel/factors_info/factors_info.h>

#include <library/cpp/getopt/opt.h>

#include <util/digest/murmur.h>
#include <util/digest/sequence.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/generic/ymath.h>
#include <util/stream/file.h>
#include <util/string/printf.h>
#include <util/string/vector.h>
#include <util/string/split.h>

typedef TVector<int> TMonom;
typedef TVector<TMonom> TMonoms;
typedef TVector<float> TWeights;

struct TWeightedMonom {
    float Weight;
    TMonom Monom;
    float Value;

    TWeightedMonom() = default;

    TWeightedMonom(float weight, TMonom monom)
        : Weight(weight)
        , Monom(monom)
    {
    }

    bool operator<(const TWeightedMonom& monom) const {
        return fabs(Value) > fabs(monom.Value);
    }
};

class TFormulaOutput : TNonCopyable {
private:
    const bool TwoOutputs;
    const bool MultiOutputs;
    const TString Output2;
    const TString List;
    const TString Prefix;

    TStringStream SOut;
    TFixedBufferFileOutput FOut;
    TStringStream SOut2;
    THolder<TFixedBufferFileOutput> FOut2;
    typedef THashMap< TString, TSimpleSharedPtr<TStringStream> > TName2Stream;
    TName2Stream SOuts;

public:
    TFormulaOutput(bool twoOutputs, bool multiOutputs, const TString& output, const TString& output2, const TString& list, const TString& prefix)
        : TwoOutputs(twoOutputs)
        , MultiOutputs(multiOutputs)
        , Output2(output2)
        , List(list)
        , Prefix(prefix)
        , FOut(output)
    {
        if (TwoOutputs)
            FOut2.Reset(new TFixedBufferFileOutput(Output2));
    }

    ~TFormulaOutput() noexcept(false) {
        FOut << SOut.Str() << Endl;
        if (TwoOutputs) {
            *FOut2 << SOut2.Str();
        } else if (!MultiOutputs) {
            FOut << SOut2.Str();
        } else {
            TVector<TString> fromList;
            try {
                {
                    TFileInput fList(List);
                    TString line;
                    while (fList.ReadLine(line))
                        fromList.push_back(line);
                }
                Sort(fromList.begin(), fromList.end());
            } catch (...) {
                Cerr << "warn: " << CurrentExceptionMessage() << Endl;
            }

            TVector<TString> out;
            out.push_back("SRCS(");
            for (TName2Stream::const_iterator toNames = SOuts.begin(); toNames != SOuts.end(); ++toNames) {
                TString filename = Prefix + "." + toNames->first + ".cpp";
                out.push_back(filename);
                filename = Output2 + "." + toNames->first + ".cpp";
                TFixedBufferFileOutput fOut(filename);
                fOut << SOut2.Str() << Endl;
                fOut << toNames->second->Str() << Endl;
            }
            out.push_back(")");
            out.push_back("SET_SOURCE_FILES_PROPERTIES(");
            for (TName2Stream::const_iterator toNames = SOuts.begin(); toNames != SOuts.end(); ++toNames) {
                TString filename = Prefix + "." + toNames->first + ".cpp";
                out.push_back(filename);
            }
            out.push_back(" PROPERTIES GENERATED TRUE)");

            const TString outList = Output2 + ".list";
            {
                TFixedBufferFileOutput fList(outList);
                for (size_t i = 0; i < out.size(); ++i)
                    fList << out[i] << Endl;
            }
            Sort(out.begin(), out.end());

            if (!UncaughtException() && out != fromList)
                ythrow yexception() << "'" << List << "' != '" << outList << "'. Replace.";
        }
    }

    IOutputStream& StreamHeader() {
        return SOut;
    }

    IOutputStream& StreamImplCommon() {
        return SOut2;
    }

    IOutputStream& StreamImpl(const TString& name) {
        if (!MultiOutputs) {
            return SOut2;
        } else {
            TName2Stream::iterator toName = SOuts.find(name);
            if (toName == SOuts.end()) {
                SOuts.insert( TName2Stream::value_type(name, new TStringStream()) );
                toName = SOuts.find(name);
            }
            return *toName->second;
        }
    }
};

typedef TVector<TWeightedMonom> TWeightedMonoms;

static void ExplFormula(const TString& name, const TMonoms& monoms, const TWeights& weights, const IFactorsInfo* factorsInfo, TFormulaOutput& out) {
    out.StreamImpl(name) << "/* Explanation (" << name << "):" << Endl;
    TWeightedMonoms wMonoms;
    for (size_t i = 0; i < monoms.size(); ++i) {
        wMonoms.push_back( TWeightedMonom(weights[i], monoms[i]) );
        wMonoms.back().Value = weights[i];
    }
    Sort(wMonoms.begin(), wMonoms.end());
    for (size_t i = 0; i < wMonoms.size(); ++i) {
        out.StreamImpl(name) << Sprintf("%.6lf * ", wMonoms[i].Weight);
        for (size_t j = 0; j < wMonoms[i].Monom.size(); ++j)
            out.StreamImpl(name) << Sprintf("%s ", factorsInfo->GetFactorName(wMonoms[i].Monom[j]));
        out.StreamImpl(name) << Endl;
    }
    out.StreamImpl(name) << "*/" << Endl;
}

static int CalcMaxArg(const TMonoms &monoms) {
    typedef TMap<int, int> TCounts;
    TCounts counts;
    for (size_t i = 0; i < monoms.size(); ++i) {
        for (size_t j = 0; j < monoms[i].size(); ++j)
            ++counts[monoms[i][j]];
    }

    int maxVal = 0;
    int maxArg = -1;
    for (TCounts::const_iterator toCount = counts.begin(); toCount != counts.end(); ++toCount) {
        if (toCount->second > maxVal) {
            maxVal = toCount->second;
            maxArg = (int)toCount->first;
        }
    }
    return maxArg;
}

template<class TW>
static void SplitMonoms(const TMonoms& monoms, const TW& weights,
                        int maxArg,
                        TMonoms* m1, TW* w1, TMonoms* m2, TW* w2)
{
    for (size_t i = 0; i < monoms.size(); ++i) {
        size_t j = 0;
        while ( (j < monoms[i].size()) && (maxArg != monoms[i][j]) )
            ++j;
        if (j == monoms[i].size()) {
            m1->push_back(monoms[i]);
            w1->push_back(weights[i]);
        } else {
            TMonom newMonom;
            for (size_t k = 0; k < monoms[i].size(); ++k)
                if (k != j)
                    newMonom.push_back(monoms[i][k]);
            m2->push_back(newMonom);
            w2->push_back(weights[i]);
        }
    }
}

const size_t N_BINARY_OPT_MIN_COUNT = 5;

static TString GetFormula(const TMonoms& monoms, const TWeights& weights, const IFactorsInfo* factorsInfo, bool optimizeBinary, size_t level) {
    if (monoms.empty())
        return "";
    int maxArg = CalcMaxArg(monoms);

    const TString fill = TString(" ") * level;

    if (maxArg == -1) {
        float w = 0;
        for (size_t i = 0; i < monoms.size(); ++i) {
            if (monoms[i].empty())
                w += weights[i];
            else
                ythrow yexception() << "should have only empty monoms";
        }
        return Sprintf("%s + (%.20ff)\n", fill.data(), w);
    }

    TMonoms m1, m2;
    TWeights w1, w2;
    SplitMonoms(monoms, weights, maxArg, &m1, &w1, &m2, &w2);

    // printf("%d %d %d %d\n", max, (int)maxArg, (int)w1.size(), (int)w2.size());
    if (w2.size() > 1) {
        TString result;
        if (!w1.empty()) {
            result += GetFormula(m1, w1, factorsInfo, optimizeBinary, level);
            result += "\n";
        }

        if (optimizeBinary) {
            if (!factorsInfo->IsOftenZero(maxArg) || (w2.size() < N_BINARY_OPT_MIN_COUNT))
                result += Sprintf("%s + f[%d] /*%s %" PRISZT "*/ *(\n%s)\n", fill.data(), maxArg, factorsInfo->GetFactorName(maxArg), w2.size(), GetFormula(m2, w2, factorsInfo, optimizeBinary, level + 1).data());
            else
                result += Sprintf("%s + ((f[%d]) ? (f[%d] /*!%s %" PRISZT "*/ *(\n%s)) : 0.f\n)", fill.data(), maxArg, maxArg, factorsInfo->GetFactorName(maxArg), w2.size(), GetFormula(m2, w2, factorsInfo, optimizeBinary, level + 1).data());
        } else {
            result += Sprintf("%s + f[%d] /*%s*/*(\n%s)\n", fill.data(), maxArg, factorsInfo->GetFactorName(maxArg), GetFormula(m2, w2, factorsInfo, optimizeBinary, level + 1).data());
        }
        return result;
    } else {
        TString result = fill;
        for (size_t i = 0; i < monoms.size(); ++i) {
            if (i)
                result += "\n" + fill + "+";
            result += Sprintf( ((weights[i] > 0) ? "%.20ff" : "(%.20ff)") , weights[i]);
            for (size_t j = 0; j < monoms[i].size(); ++j) {
                result += Sprintf("*f[%d]/*%s*/", monoms[i][j], factorsInfo->GetFactorName(monoms[i][j]));
            }
        }
        return result;
    }
}

/******************* SSE *******************/

typedef TVector<int> TWeightsIdx;

class T4FormulaFeaturePrinter {
private:
    int Index;

public:
    T4FormulaFeaturePrinter(int index)
        : Index(index)
    {
    }

    TString Print() const {
        return Sprintf("_mm_set_ps1(f[%d])", Index);
    }

    static TString KoefPrint(int index) {
        return Sprintf("koef[%d]", index);
    }
};

class TFormulaFeaturePrinter {
private:
    int Index;

public:
    TFormulaFeaturePrinter(int index)
        : Index(index)
    {
    }

    TString Print() const {
        return Sprintf("feat%d", Index);
    }

    static TString KoefPrint(int index) {
        return Sprintf("koef%d", index);
    }
};

template<typename TFeaturePrinter>
static TString GenSSEMonom(const TMonom &m, int wIdx, const IFactorsInfo* factorsInfo, size_t level)
{
    TString result = TFeaturePrinter::KoefPrint(wIdx);
    for (size_t k = 0; k < m.size(); ++k) {
        TFeaturePrinter printer(m[k]);
        result = Sprintf("_mm_mul_ps(%s, %s /*%s*/)", result.data(), printer.Print().data(), factorsInfo->GetFactorName(m[k]));
    }
    result = (TString("  ") * level) + result;
    return result.data();
}

template<typename TFeaturePrinter>
static TString GenSSESum(const TMonoms& monoms, const TWeightsIdx &weights, const IFactorsInfo* factorsInfo, size_t level)
{
    if (weights.empty())
        ythrow yexception() << "empty weights";
    if (weights.size() == 1)
        return GenSSEMonom<TFeaturePrinter>(monoms[0], weights[0], factorsInfo, level);

    TMonoms m1, m2;
    TWeightsIdx w1, w2;
    for (size_t i = 0; i < monoms.size(); ++i) {
        if (i < monoms.size() / 2) {
            m1.push_back(monoms[i]);
            w1.push_back(weights[i]);
        } else {
            m2.push_back(monoms[i]);
            w2.push_back(weights[i]);
        }
    }
    const TString fill = TString("  ") * level;
    TString result = fill + "_mm_add_ps(\n";
    result += GenSSESum<TFeaturePrinter>(m1, w1, factorsInfo, level + 1) + ",\n";
    result += GenSSESum<TFeaturePrinter>(m2, w2, factorsInfo, level + 1) + ")";
    return result;
}

template<typename TFeaturePrinter>
static TString Get4FormulaSSE(const TMonoms& monoms, const TWeightsIdx &weights, const IFactorsInfo* factorsInfo, bool optimizeBinary, size_t level)
{
    if (monoms.empty())
        return "";
    int maxArg = CalcMaxArg(monoms);

    const TString fill = TString("  ") * level;

    if (maxArg == -1) {
        Y_ASSERT(monoms.size() == 1);
        if (monoms.size() != 1)
            ythrow yexception() << "Formula has duplicate monom";
        return Sprintf("%s%s", fill.data(), TFeaturePrinter::KoefPrint(weights[0]).data());
    }

    TMonoms m1, m2;
    TWeightsIdx w1, w2;
    SplitMonoms(monoms, weights, maxArg, &m1, &w1, &m2, &w2);

    if (w2.size() > 1) {
        TString result;

        size_t intLevel = level;
        TString intFill = fill;
        if (!w1.empty()) {
            result += fill + "_mm_add_ps(\n";
            result += Get4FormulaSSE<TFeaturePrinter>(m1, w1, factorsInfo, optimizeBinary, level + 1);
            result += ",\n";
            intLevel = level + 1;
            intFill += "  ";
        }
        TFeaturePrinter printer(maxArg);
        if (optimizeBinary && factorsInfo->IsOftenZero(maxArg) && w2.size() >= N_BINARY_OPT_MIN_COUNT) {
            result += Sprintf("%s((*(i32*)(f+%d) != 0) ? (_mm_mul_ps(%s /*!%s %" PRISZT "*/,\n"
                "%s)) : _mm_set_ps1(0.f))",
                intFill.data(), maxArg, printer.Print().data(), factorsInfo->GetFactorName(maxArg), w2.size(),
                Get4FormulaSSE<TFeaturePrinter>(m2, w2, factorsInfo, optimizeBinary, intLevel + 1).data());
        } else {
            result += Sprintf("%s_mm_mul_ps(%s /*%s %" PRISZT "*/,\n%s)",
                intFill.data(), printer.Print().data(), factorsInfo->GetFactorName(maxArg), w2.size(),
                Get4FormulaSSE<TFeaturePrinter>(m2, w2, factorsInfo, optimizeBinary, intLevel + 1).data());
        }
        if (!w1.empty())
            result += ")";
        return result;
    } else {
        return GenSSESum<TFeaturePrinter>(monoms, weights, factorsInfo, level);
    }
}

static void CollapseSame(TMonoms *monoms, TWeights *weights)
{
    TMonoms res;
    TWeights resW;
    typedef THashMap<TVector<int>, int, TContiguousHash<TMurmurHash<>>> THash;
    THash hh;
    for (size_t k = 0; k < monoms->size(); ++k) {
        TVector<int> fac = (*monoms)[k];
        float w = (*weights)[k];
        std::sort(fac.begin(), fac.end());
        THash::iterator z = hh.find(fac);
        if (z != hh.end()) {
            resW[z->second] += w;
        } else {
            int idx = (int)res.size();
            hh[fac] = idx;
            res.push_back(fac);
            resW.push_back(w);
        }
    }
    monoms->swap(res);
    weights->swap(resW);
}

class TSimpleCodeGen {
private:
    TFormulaOutput& Out;
    const IFactorsInfo* FactorsInfo;
    bool OptimizeBinary;

public:
    TSimpleCodeGen(TFormulaOutput& out, const IFactorsInfo* factorsInfo, bool optimizeBinary)
        : Out(out)
        , FactorsInfo(factorsInfo)
        , OptimizeBinary(optimizeBinary)
    {

    }

    static TString Shortify(const TString& fml) {
        TString splitted;
        size_t begin = 0;
        while (begin < fml.size()) {
            if (begin)
                splitted += " + ";
            const size_t PART_LEN = 10000;
            splitted += Sprintf("TString(\"%s\")", fml.substr(begin, PART_LEN).data());
            begin += PART_LEN;
        }
        return splitted;
    }

    virtual ~TSimpleCodeGen() {
    }

    void OnBegin() {

    }

    void OnLine(const SRelevanceFormula& formula, const TString& name, int nFactors, bool sse = false, bool checkRemovedFactors = true) {
        TMonoms monoms;
        TWeights weights;
        formula.GetFormula(&monoms, &weights);

        if (checkRemovedFactors) {
            for (size_t k = 0; k < monoms.size(); ++k) {
                for (size_t l = 0; l < monoms[k].size(); ++l) {
                    if (FactorsInfo->IsRemovedFactor(monoms[k][l])) {
                        ythrow yexception() << "Removed factor " << monoms[k][l] << " used in a formula\n";
                    }
                }
            }
        }

        ExplFormula(name, monoms, weights, FactorsInfo, Out);

        Out.StreamHeader() << "#include <library/cpp/sse/sse.h>\n";
        Out.StreamHeader() << "#include <kernel/relevfml/relev_fml.h>\n";
        Out.StreamHeader() << "extern SRelevanceFormula fml" << name << ";\n";
        Out.StreamHeader() << "float " << name << "(const float* f);\n";
        Out.StreamImpl(name) << "static float " << name << "Impl(const float* f);\n";
        Out.StreamImpl(name) << "float " << name << "(const float* f) {\n";
        Out.StreamImpl(name) << "\treturn " << name << "Impl(f);\n";
        Out.StreamImpl(name) << "}\n";
        Out.StreamImpl(name) << "static float " << name << "Impl(const float* f) {\n"
            << "\treturn\n"
            << GetFormula(monoms, weights, FactorsInfo, OptimizeBinary, 0) << ";\n}\n\n";

        Out.StreamImpl(name) << "SRelevanceFormula fml" << name << ";\n"
            << "static const TString s" << name << " = " << Shortify(Encode(formula)) << ";\n";
        if (sse)
            Out.StreamImpl(name) << "static void " << name << "SSEImpl(const float* const* factors, float* result);\n";
        Out.StreamImpl(name) << "static TPolynomDescrStatic " << name
            << "Descr("
            << name << "Impl, "
            << (sse ? name + "SSEImpl" : "NULL") << ", "
            << "&fml" << name << ", "
            << "\"" << name << "\");"
            << Endl;
        Out.StreamImpl(name)
            << "struct T" << name << " {\n\tT" << name << "() {\n"
            << "\t\tDecode(&fml" << name << ", s" << name << ".data(), " << nFactors << ");\n"
            << "\t\tRelevanceFormulaRegistry()->Add(&" << name << "Descr);\n"
            << "\t}\n"
            << "} init" << name << ";\n"
            << "\n";
    }

    void OnEnd(const TVector<TString>&) {

    }
};

class TSimpleSSECodeGen {
private:
    TFormulaOutput& Out;
    const IFactorsInfo* FactorsInfo;
    TSimpleCodeGen Simple;
    typedef TVector<size_t> TCounts;
    typedef TVector<float> TCountWeights;
    TCounts Counts;
    TCountWeights Weights;

public:
    TSimpleSSECodeGen(TFormulaOutput& out, const IFactorsInfo* factorsInfo)
        : Out(out)
        , FactorsInfo(factorsInfo)
        , Simple(out, factorsInfo, true)
    {
    }

    virtual ~TSimpleSSECodeGen() {
    }

    void OnBegin() {
    }

    void OnLine(const SRelevanceFormula& formula, const TString& name, int nFactors, bool assertArguments, bool checkRemovedFactors) {
        Simple.OnLine(formula, name, nFactors, true, false);

        const bool isExp = name.find("Matrix") != TString::npos;

        if (nFactors >= (int)Counts.size()) {
            Counts.resize(nFactors);
            Weights.resize(nFactors);
        }

        TMonoms monoms;
        TWeights weights;
        formula.GetFormula(&monoms, &weights);
        CollapseSame(&monoms, &weights);

        Out.StreamHeader() << "void " << name << "SSE(const float* const* factors, float* result);\n";
        Out.StreamImpl(name) << "static void " << name << "SSEImpl(const float* const* factors, float* result);\n";
        Out.StreamImpl(name) << "void " << name << "SSE(const float* const* factors, float* result) {\n";
        Out.StreamImpl(name) << "\t" << name << "SSEImpl(factors, result);\n";
        Out.StreamImpl(name) << "}\n\n";
        Out.StreamImpl(name) << "static void " << name << "SSEImpl(const float* const* factors, float* result) {\n";
        Out.StreamImpl(name) << "const float *f0 = factors[0];\n";
        Out.StreamImpl(name) << "const float *f1 = factors[1];\n";
        Out.StreamImpl(name) << "const float *f2 = factors[2];\n";
        Out.StreamImpl(name) << "const float *f3 = factors[3];\n";

        TWeightsIdx weightsIdx;
        for (int k = 0; k < (int)monoms.size(); ++k)
            weightsIdx.push_back(k);

        Out.StreamImpl(name) << "   Y_ASSERT(result);\n";
        Out.StreamImpl(name) << "   Y_ASSERT(factors);\n";
        for (size_t k = 0; k < 4; ++k)
            Out.StreamImpl(name) << "   Y_ASSERT(factors[" << k << "]);\n";

        for (size_t k = 0; k < monoms.size(); ++k) {
            TString coeff;
            for (size_t i = 0; i < 4; ++i) {
                if (i)
                    coeff += ", ";
                coeff += Sprintf("%.20lff", weights[k]);
            }
            Out.StreamImpl(name) << "static const __m128 koef" << k << " = {" << coeff << "};\n";
        }

        TVector<bool> usedFactors;
        for (size_t k = 0; k < monoms.size(); ++k)
            for (size_t l = 0; l < monoms[k].size(); ++l) {
                if (usedFactors.ysize() <= monoms[k][l])
                    usedFactors.resize(monoms[k][l] + 1, false);
                usedFactors[monoms[k][l]] = true;
                if (checkRemovedFactors && FactorsInfo->IsRemovedFactor(monoms[k][l])) {
                    ythrow yexception() << "Removed factor " << monoms[k][l] << " used in a formula\n";
                }
                if (isExp) {
                    ++Counts[monoms[k][l]];
                    Weights[monoms[k][l]] += weights[k];
                }
            }

        if (assertArguments) {
            for (int k = 0; k < nFactors && k < usedFactors.ysize(); ++k) {
                if (usedFactors[k] && !FactorsInfo->IsNot01Factor(k)) {
                    for (size_t i = 0; i < 4; ++i) {
                        Out.StreamImpl(name) << "    Y_ASSERT(factors[" << i << "][" << k << "] >= 0.f);\n";
                        Out.StreamImpl(name) << "    Y_ASSERT(factors[" << i << "][" << k << "] <= 1.f);\n";
                    }
                }
            }
        }

         for (int k = 0; k < nFactors; ) {
            if ((k < usedFactors.ysize()) && (usedFactors[k])) {
                bool dumped = false;
                if (k + 4 < nFactors) {
                    int count = 0;
                    for (int j = k; j < k + 4; ++j)
                        if ((j < usedFactors.ysize()) && (usedFactors[j]))
                            ++count;
                    if (count > 2) {
                        dumped = true;
                        Out.StreamImpl(name) << Sprintf("__m128 feat%d = _mm_loadu_ps(f0 + %d);\n",k + 0, k);
                        Out.StreamImpl(name) << Sprintf("__m128 feat%d = _mm_loadu_ps(f1 + %d);\n",k + 1, k);
                        Out.StreamImpl(name) << Sprintf("__m128 feat%d = _mm_loadu_ps(f2 + %d);\n",k + 2, k);
                        Out.StreamImpl(name) << Sprintf("__m128 feat%d = _mm_loadu_ps(f3 + %d);\n",k + 3, k);
                        Out.StreamImpl(name) << Sprintf("_MM_TRANSPOSE4_PS(feat%d, feat%d, feat%d, feat%d);\n",k + 0, k + 1, k + 2, k + 3);
                        k += 4;
                    }
                }

                if (!dumped) {
                    Out.StreamImpl(name) << Sprintf("const __m128 feat%d = {f0[%d], f1[%d], f2[%d], f3[%d]};\n", k, k, k, k, k);
                    ++k;
                }
            } else {
                ++k;
            }
         }

        Out.StreamImpl(name) << "  union {\n"
             << "    __m128 M;\n"
             << "    float F[4];\n"
             << "  } temp;\n"
             << "  temp.M =\n";

        Out.StreamImpl(name) << Get4FormulaSSE<TFormulaFeaturePrinter>(monoms, weightsIdx, FactorsInfo, false, 0) << ";\n";
        for (size_t k = 0; k < 4; ++k)
            Out.StreamImpl(name) << "result[" << k << "] = temp.F[" << k << "];\n";
        Out.StreamImpl(name) << "}\n\n";
    }

    void OnEnd(const TVector<TString>&) {
    }

    template<typename T>
    struct TCountPair {
        size_t Index;
        T Count;

        bool operator<(const TCountPair& cnt) const {
            return Count > cnt.Count;
        }

        TCountPair(size_t index, T count)
            : Index(index)
            , Count(count)
        {

        }

        TCountPair() = default;
    };

    typedef TVector< TCountPair<size_t> > TCountPairs;
    typedef TVector< TCountPair<double> > TWeightPairs;

    void OutStat() {
        {
            Out.StreamImplCommon() << "/* Stat:" << Endl;
            TCountPairs counts;
            for (size_t i = 0; i < Counts.size(); ++i)
                counts.push_back( TCountPair<size_t>(i, Counts[i]) );
            for (size_t i = 0; i < counts.size(); ++i) {
                Out.StreamImplCommon() << FactorsInfo->GetFactorName(counts[i].Index) << "\t" << counts[i].Count << Endl;
            }
            Out.StreamImplCommon() << "*/" << Endl;
            Out.StreamImplCommon() << "/* StatOrdered:" << Endl;
            Sort(counts.begin(), counts.end());
            for (size_t i = 0; i < counts.size(); ++i) {
                Out.StreamImplCommon() << FactorsInfo->GetFactorName(counts[i].Index) << "\t" << counts[i].Count << Endl;
            }
            Out.StreamImplCommon() << "*/" << Endl;
        }
        {
            Out.StreamImplCommon() << "/* StatWeight:" << Endl;
            TWeightPairs weights;
            for (size_t i = 0; i < Counts.size(); ++i)
                weights.push_back( TCountPair<double>(i, Weights[i]) );
            Sort(weights.begin(), weights.end());
            for (size_t i = 0; i < weights.size(); ++i) {
                Out.StreamImplCommon() << FactorsInfo->GetFactorName(weights[i].Index) << "\t" << weights[i].Count << Endl;
            }
            Out.StreamImplCommon() << "*/" << Endl;
        }
    }
};

TString PolynomNameFromFileName(const TString& fileName) {
    const size_t slash = fileName.find_last_of("/\\");
    const size_t dot = fileName.rfind('.');
    const size_t begin = (slash == TString::npos ? 0 : slash + 1);
    const size_t end = (dot > begin ? dot : TString::npos);

    return fileName.substr(begin,end-begin);
}

void GenerateFromText(IInputStream& input, TSimpleCodeGen& simpleCodeGen, TSimpleSSECodeGen& simpleSSECodeGen, size_t maxFactors, bool checkRemovedFactors) {
    TString line;
    while (input.ReadLine(line)) {
        if (line.empty())
            continue;
        if ('#' == line[0])
            continue;

        TVector<TString> parsed;
        StringSplitter(line).Split(' ').SkipEmpty().Collect(&parsed);

        TString mode;
        TString name;
        TString fml;
        int nFactors;

        if (parsed[0] == "!" || parsed[0] == "*") {
            mode = parsed[0];
            name = parsed[1];
            fml = parsed[2];
            nFactors = FromString<int>(parsed[3]);
        } else {
            name = parsed[0];
            fml = parsed[1];
            nFactors = FromString<int>(parsed[2]);
        }

        if (nFactors > (int)maxFactors)
            ythrow yexception() << "too large factor count " <<  nFactors << " " <<  (int)maxFactors;

        SRelevanceFormula formula;
        Decode(&formula, fml, nFactors);

        if (mode == "!")
            simpleSSECodeGen.OnLine(formula, name, nFactors, false, checkRemovedFactors);
        else if (mode == "*")
            simpleSSECodeGen.OnLine(formula, name, nFactors, true, checkRemovedFactors);
        else
            simpleCodeGen.OnLine(formula, name, nFactors, false, checkRemovedFactors);
    }
}

static void Usage() {
    fprintf(stderr,
        "Read input from stdin.\n"
        "Usage: relev_fml_codegen -o <out> [-b] [-2] [-3] [-O <out2>] [-f <polynom>]\n"
        "\n"
        "    -o    output file name\n"
        "    -b    optimize binary, false by default\n"
        "    -2    separate definition and declaration\n"
        "    -3    multiple definitions and one declaration\n"
        "    -O    definition output file name\n"
        "    -c    don't check that there are no removed factors in the formula\n"
        "    -f    read one polynom from .pln file\n"
        "    -T    read polynoms from text file instead of stdin\n"
        );
}

int codegen_main(int argc, char* argv[], const IFactorsInfo* factorsInfo) {
    TString output;
    TString outputImpl;
    TString list;
    TString prefix;
    TString formulaFromFile;
    TString formulasFromTextFile;
    bool optimizeBinary = false;
    bool twoOutputs = false;
    bool multiOutput = false;
    bool checkRemovedFactors = true;
    OPTION_HANDLING_PROLOG_ANON("o:b?2O:3L:P:cf:T:");
    OPTION_HANDLE('o', (output = opt.Arg));
    OPTION_HANDLE('b', (optimizeBinary = true));
    OPTION_HANDLE('O', (outputImpl = opt.Arg));
    OPTION_HANDLE('2', (twoOutputs = true));
    OPTION_HANDLE('3', (multiOutput = true));
    OPTION_HANDLE('L', (list = opt.Arg));
    OPTION_HANDLE('P', (prefix = opt.Arg));
    OPTION_HANDLE('c', (checkRemovedFactors = false));
    OPTION_HANDLE('f', (formulaFromFile = opt.Arg));
    OPTION_HANDLE('T', (formulasFromTextFile = opt.Arg));
    case '?':
        Usage();
        return 1;
    OPTION_HANDLING_EPILOG;

    if (output.empty()) {
        Usage();
        return 1;
    }

    if ((twoOutputs || multiOutput) && outputImpl.empty()) {
        Usage();
        return 1;
    }

    try {
        TFormulaOutput fOut(twoOutputs, multiOutput, output, outputImpl, list, prefix);
        fOut.StreamImplCommon() << "#include <library/cpp/sse/sse.h>\n";
        fOut.StreamImplCommon() << "#include <kernel/relevfml/relev_fml.h>\n";
        fOut.StreamImplCommon() << "\n";

        TSimpleCodeGen simpleCodeGen(fOut, factorsInfo, optimizeBinary);
        TSimpleSSECodeGen simpleSSECodeGen(fOut, factorsInfo);

        if (!formulaFromFile.empty()) {
            TFileInput in(formulaFromFile);
            TPolynomDescr pol;
            ::Load(&in,pol);

            const TString name = PolynomNameFromFileName(formulaFromFile);
            simpleSSECodeGen.OnLine(pol.Descr, name, factorsInfo->GetFactorCount(), false, checkRemovedFactors);

        } else {
            if (!formulasFromTextFile.empty()) {
                TFileInput in(formulasFromTextFile);
                GenerateFromText(in, simpleCodeGen, simpleSSECodeGen, factorsInfo->GetFactorCount(), checkRemovedFactors);
            } else {
                GenerateFromText(Cin, simpleCodeGen, simpleSSECodeGen, factorsInfo->GetFactorCount(), checkRemovedFactors);
            }
        }

        simpleSSECodeGen.OutStat();
    }
    catch (...) {
        std::remove(output.data());
        if (!!outputImpl)
            std::remove(outputImpl.data());
        Cerr << "relev_fml_codegen failed: " << CurrentExceptionMessage() << Endl;
        return 1;
    }

    return 0;
}
