#include "trash_classifier.h"

#include <kernel/snippets/read_helper/read_helper.h>

#include <util/generic/string.h>
#include <util/charset/unidata.h>
#include <utility>

namespace NSnippets {
    namespace {

        const std::pair<double, double> GOOD_SPACE_RATIO_INTERVAL = std::pair<double,double>(0.0846, 0.23);
        const double UPPER_NOT_ALPHA_BOUND = 0.65;

        struct TWtrokaStatistics
        {
            i32 SpaceCount;
            i32 NotAlphaCount;
            i32 SymbolLength;

            TWtrokaStatistics()
                : SpaceCount(0)
                , NotAlphaCount(0)
                , SymbolLength(0)
            {
            }
        };

        class TStaticAnnotationClassifier
        {
        public:
            static bool IsGoodEnough(const TWtringBuf& wtroka);

        private:
            static bool IsGoodSpaceRatio(const TWtrokaStatistics& wtrokaStatistics);
            static bool IsGoodNotAlphaRatio(const TWtrokaStatistics& wtrokaStatistics);
            static TWtrokaStatistics GetStatistics(const TWtringBuf& wtroka);
        };

        TWtrokaStatistics TStaticAnnotationClassifier::GetStatistics(const TWtringBuf& wtroka)
        {
            TWtrokaStatistics wtrokaStatistics;
            wtrokaStatistics.SymbolLength = wtroka.length();

            for (TWtringBuf::const_iterator character = wtroka.begin();
                character != wtroka.end();
                ++character)
            {

                if (!IsAlpha(*character)) {
                    ++wtrokaStatistics.NotAlphaCount;
                }

                if (IsSpace(*character)) {
                    ++wtrokaStatistics.SpaceCount;
                }
            }

            return wtrokaStatistics;
        }

        bool TStaticAnnotationClassifier::IsGoodEnough(const TWtringBuf& wtroka)
        {
            const TWtrokaStatistics statistics = GetStatistics(wtroka);

            const bool isGoodSpaceRatio = IsGoodSpaceRatio(statistics);
            const bool isGoodNotAlphaRatio = IsGoodNotAlphaRatio(statistics);
            return isGoodSpaceRatio && isGoodNotAlphaRatio;
        }

        bool TStaticAnnotationClassifier::IsGoodSpaceRatio(const TWtrokaStatistics& wtrokaStatistics)
        {
            if (wtrokaStatistics.SymbolLength == 0) {
                return false;
            }

            const double spaceRatio =
                static_cast<double>(wtrokaStatistics.SpaceCount + 1) / wtrokaStatistics.SymbolLength;

            const bool biggerThanLowerBound = (spaceRatio > GOOD_SPACE_RATIO_INTERVAL.first);
            const bool smallerThanUpperBound = (spaceRatio < GOOD_SPACE_RATIO_INTERVAL.second);
            return biggerThanLowerBound && smallerThanUpperBound;
        }

        bool TStaticAnnotationClassifier::IsGoodNotAlphaRatio(const TWtrokaStatistics& wtrokaStatistics)
        {
            if (wtrokaStatistics.SymbolLength == 0) {
                return false;
            }

            const double notAlphaRatio =
                static_cast<double>(wtrokaStatistics.NotAlphaCount) / wtrokaStatistics.SymbolLength;

            return notAlphaRatio < UPPER_NOT_ALPHA_BOUND;
        }
    }

    namespace NTrashClassifier {
        static const TUtf16String BAD_SUBSTR[] = {
            u"http:",
            u"https:",
            u"=\"",
            u"&#",
            u"&nbsp",
            u"&raquo",
            u"&laquo",
            u"&ndash",
            u"&mdash",
        };

        static bool ContainsBadSubstr(const TUtf16String& desc) {
            for (const TUtf16String& str : BAD_SUBSTR) {
                if (desc.find(str) != desc.npos) {
                    return true;
                }
            }
            return false;
        }

        bool IsTrash(const TReplaceContext& ctx, const TUtf16String& desc) {
            if (ContainsBadSubstr(desc)) {
                return true;
            }
            TReadabilityChecker checker(ctx.Cfg, ctx.QueryCtx);
            checker.CheckCapitalization = true;
            checker.CheckRepeatedWords = true;
            checker.CheckBadCharacters = true;
            return !checker.IsReadable(desc, false);
        }

        bool IsGoodEnough(const TWtringBuf& wtroka) {
            return TStaticAnnotationClassifier::IsGoodEnough(wtroka);
        }
   }
}
