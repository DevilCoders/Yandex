#include "manual_video.h"

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

    const TVector<double> MKVideo(InitManualCoefficients(0.05));
    const TLinearFormula ManualVideoFormula("manual_with_plm_like", algo2Domain, &MKVideo[0]);
}

