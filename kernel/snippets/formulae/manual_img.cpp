#include "manual_img.h"

#include <kernel/snippets/factors/factors.h>

#include <util/generic/vector.h>

namespace NSnippets
{
    static TVector<double> InitManualImgCoefficients()
    {
        TVector<double> manualImgCoefficients(FC_ALGO2, 0.0);

        manualImgCoefficients[A2_MF_FOOTER] = -1000;
        manualImgCoefficients[A2_MF_TEXT_AREA] = -500;
        manualImgCoefficients[A2_MF_TELORG] = 100;
        manualImgCoefficients[A2_MF_VIDEOATTR] = 1; // used only if BUILD_FOR_VIDEO is defined
        manualImgCoefficients[A2_MF_READABLE] = -50;
        /*
         * stuff below is now used only as factors for MF_IMGBOOST factor
         *
        manualImgCoefficients[A2_MF_IMG_IsPornoAdv] = -100;
        manualImgCoefficients[A2_MF_IMG_IsYellowAdv] = -25;
        manualImgCoefficients[A2_MF_IMG_IsPopunder] = -50;
        manualImgCoefficients[A2_MF_IMG_IsClickunder] = -50;
        manualImgCoefficients[A2_MF_IMG_TrashAdvGE8] = -50;
        manualImgCoefficients[A2_MF_IMG_NoSize] = -1000;
        manualImgCoefficients[A2_MF_IMG_Size] = 10;
        manualImgCoefficients[A2_MF_IMG_NewsIfPolitician] = 1000;
        manualImgCoefficients[A2_MF_IMG_DefHide] = -100;
        manualImgCoefficients[A2_MF_IMG_DefTic] = 0.01;
        manualImgCoefficients[A2_MF_IMG_PornoPess] = -2500;
        */
        manualImgCoefficients[A2_MF_IMG_NotDefForeign] = 100;
        manualImgCoefficients[A2_MF_IMG_ComEng] = 200;
        manualImgCoefficients[A2_MF_IMG_DefRUk] = 0.04;
        manualImgCoefficients[A2_MF_IMG_UaMatch] = 400;
        manualImgCoefficients[A2_MF_IMG_TrDomTr] = 200;
        manualImgCoefficients[A2_MF_IMG_MainLangMatch] = 200;
        manualImgCoefficients[A2_MF_IMG_SecondLangMatch] = 100;

        manualImgCoefficients[A2_MF_IMG_SiteMatch] = 1000;
        manualImgCoefficients[A2_MF_IMG_SeriesMatch] = 5000;
        manualImgCoefficients[A2_MF_IMG_ConstrPass] = 1000;
        manualImgCoefficients[A2_MF_IMG_ExactWallpaper] = 1000;
        manualImgCoefficients[A2_MF_IMG_LargerWallpaper] = 500;

        manualImgCoefficients[A2_MF_IMG_RealSizeOnSeries] = 0.05;

        manualImgCoefficients[A2_MF_IMG_CyrPess] = -400;

        manualImgCoefficients[A2_MF_IMGBOOST] = 1;
        return manualImgCoefficients;
    }

    const TVector<double> MK_IMG(InitManualImgCoefficients());

    const TLinearFormula ManualImgFormula("manual_img", algo2Domain, &MK_IMG[0]);
}

