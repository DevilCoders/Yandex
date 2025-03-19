#include "manual.h"

#include <kernel/snippets/factors/factors.h>

#include <util/generic/vector.h>

namespace NSnippets
{
    static TVector<double> InitManualCoefficients(double PLMLikeWeight = 0, double LinesCountWeight = 0)
    {
        TVector<double> manualCoefficients(FC_ALGO2, 0.0);

        manualCoefficients[A2_MF_FOOTER] = -1000;
        manualCoefficients[A2_MF_TEXT_AREA] = -500;
        manualCoefficients[A2_MF_TELORG] = 100;
        manualCoefficients[A2_MF_VIDEOATTR] = 1; // used only if BUILD_FOR_VIDEO is defined
        manualCoefficients[A2_MF_READABLE] = -50;
        manualCoefficients[A2_LUPORN] = 100; //blast from the porn-free past
        manualCoefficients[A2_MF_TITLELIKE] = -50;
        manualCoefficients[A2_MCONTENT_BOOST] = 0.5;
        manualCoefficients[A2_USELESS_PESS] = -50;
        manualCoefficients[A2_REPEAT_PESS] = -0.1;
        manualCoefficients[A2_MF_KANDZI] = -0.5;
        manualCoefficients[A2_PLM_LIKE] = PLMLikeWeight;
        manualCoefficients[A2_LINES_COUNT] = LinesCountWeight;
        manualCoefficients[A2_REGION_WORDS] = 0.25;
        manualCoefficients[A2_PESSIMIZE_CYR] = -100;
        return manualCoefficients;
    }

    static TVector<double> InitFactSnippetManualCoefficients()
    {
        TVector<double> manualCoefficients(FC_ALGO2, 0.0);
        return manualCoefficients;
    }

    const TVector<double> MK(InitManualCoefficients());
    const TLinearFormula ManualFormula("manual", algo2Domain, &MK[0]);
    const TVector<double> MKWithPLMLike(InitManualCoefficients(0.05));
    const TLinearFormula ManualFormulaWithPLMLike("manual_with_plm_like", algo2Domain, &MKWithPLMLike[0]);
    const TVector<double> MKWithLinesCount(InitManualCoefficients(0.01, -0.065));
    const TLinearFormula ManualFormulaWithLinesCount("manual_with_lines_count", algo2Domain, &MKWithLinesCount[0]);
    const TVector<double> MKInfRead(InitManualCoefficients(0.1, -0.512));
    const TLinearFormula ManualFormulaInfRead("manual_inf_read", algo2Domain, &MKInfRead[0]);
    const TVector<double> MK1940(InitManualCoefficients(0.05, -0.25));
    const TLinearFormula ManualFormula1940("manual_1940", algo2Domain, &MK1940[0]);
    const TVector<double> MK29410(InitManualCoefficients(0.05, -0.2));
    const TLinearFormula ManualFormula29410("manual_29410", algo2Domain, &MK29410[0]);
    const TVector<double> FactSnippetMK(InitFactSnippetManualCoefficients());
    const TLinearFormula FactSnippetManualFormula("fact_snippet_manual", algo2Domain, &FactSnippetMK[0]);
}

