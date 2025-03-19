#include <library/cpp/testing/unittest/registar.h>

#include "relev_fml.h"

const char* encodedFmls1[] = {
    "00",
    "14000000V1",
    "1K05G02G000000S7D6JP4VKPC6JRJ6JPCMF0"
};

const char* encodedFmls2[] = {
    "0000",
    "10010000000U3",
    "100500A00800800G00000000V9JPC6P7D6JPSUKPC6JT3",
    "D901008JPC6T3"
};

const char** encodedFmls[] = {encodedFmls1, encodedFmls2};

template<class TCmd>
class SRelevanceFormulaTest : public TTestBase {
    UNIT_TEST_SUITE(SRelevanceFormulaTest);
        UNIT_TEST(TestCalculate);
    UNIT_TEST_SUITE_END();

    TString GetFormulaString(const SRelevanceFormulaBase<TCmd>& fml);
    void TestEncodeDecode(const SRelevanceFormulaBase<TCmd>& fml, const char* encodedFormula);

public:
    void TestCalculate();
};

template<class TCmd>
TString SRelevanceFormulaTest<TCmd>::GetFormulaString(const SRelevanceFormulaBase<TCmd>& fml) {
    TStringStream result;

    TVector< TVector<size_t> > params;
    TVector<float> weights;

    fml.GetFormula(&params, &weights);

    for (size_t i = 0; i < weights.size(); ++i) {
        if (i) {
            result << " + ";
        }

        result << weights[i];

        const TVector<size_t>& curParams = params[i];

        for (TVector<size_t>::const_iterator it = curParams.begin();
             it != curParams.end(); ++it) {

            result << " * " << "f[" << *it << "]";
        }
    }

    return result.Str();
}

template<class TCmd>
void SRelevanceFormulaTest<TCmd>::TestEncodeDecode(const SRelevanceFormulaBase<TCmd>& fml, const char* encodedFormula) {
    UNIT_ASSERT_STRINGS_EQUAL(encodedFormula, Encode(fml).data());
    SRelevanceFormulaBase<TCmd> f;
    Decode(&f, encodedFormula, 1024);
    UNIT_ASSERT_STRINGS_EQUAL(encodedFormula, Encode(f).data());
}

template<class TCmd>
void SRelevanceFormulaTest<TCmd>::TestCalculate() {
    size_t fmlWidth = sizeof(TCmd);

    float factors[] = { 0.1f, 0.2f };

    //0 summands

    SRelevanceFormulaBase<TCmd> f;

    UNIT_ASSERT(f.Calc(factors) == 0);
    UNIT_ASSERT(GetFormulaString(f) == "");
    TestEncodeDecode(f, encodedFmls[fmlWidth - 1][0]);

    //1 summand

    TVector<size_t> facs;
    facs.push_back(0);
    f.AddSlag(facs, 0.5f);

    UNIT_ASSERT_DOUBLES_EQUAL(0.05f, f.Calc(factors), 0.001f);
    UNIT_ASSERT_STRINGS_EQUAL("0.5 * f[0]", GetFormulaString(f).data());
    TestEncodeDecode(f, encodedFmls[fmlWidth - 1][1]);

    //many summands

    //1 multiplier
    facs[0] = 1;
    f.AddSlag(facs, 0.2f);

    //many multipliers
    facs.push_back(0);
    f.AddSlag(facs, 0.1f);

    //0 multipliers
    facs.clear();
    f.AddSlag(facs, 0.4f);

    UNIT_ASSERT_DOUBLES_EQUAL(0.492f, f.Calc(factors), 0.0001f);
    UNIT_ASSERT_STRINGS_EQUAL("0.5 * f[0] + 0.2 * f[1] + 0.1 * f[1] * f[0] + 0.4", GetFormulaString(f).data());
    TestEncodeDecode(f, encodedFmls[fmlWidth - 1][2]);

    //wide formula
    if (fmlWidth >= 2) {
        TVector<float> wideFactors(301);
        wideFactors[300] = 0.1;
        facs.clear();
        facs.push_back(300);
        SRelevanceFormulaBase<TCmd> f2;
        f2.AddSlag(facs, 0.3f);

        UNIT_ASSERT_DOUBLES_EQUAL(0.03f, f2.Calc(&wideFactors[0]), 0.001f);
        UNIT_ASSERT_STRINGS_EQUAL("0.3 * f[300]", GetFormulaString(f2).data());
        TestEncodeDecode(f2, encodedFmls[fmlWidth - 1][3]);
    }
}

UNIT_TEST_SUITE_REGISTRATION(SRelevanceFormulaTest<ui8>);

class SRelevanceFormulaTestUi16 : public SRelevanceFormulaTest<ui16> {
public:
    TString Name() const noexcept override {
        return "SRelevanceFormulaTest<ui16>";
    }

    static TString StaticName() noexcept {
        return "SRelevanceFormulaTest<ui16>";
    }
};

UNIT_TEST_SUITE_REGISTRATION(SRelevanceFormulaTestUi16);
