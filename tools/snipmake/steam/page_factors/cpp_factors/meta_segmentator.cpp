#include "meta_segmentator.h"
#include "factor_node.h"

namespace NSegmentator {
    TMetaSegmentator::TMetaSegmentator(
            const TVector<TPageSegment>& pageSegments,
            const TMetaSegmentatorCfg& cfg)
        : PageSegments(pageSegments)
        , Cfg(cfg)
    {
    }

    void TMetaSegmentator::FillMetaSegments() {
        ResetAllMetaSegments();
        FillMainContent();
    }

    // for now it's simple heuristics from https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/Annotation/DataCooking/SemanticData/VisualSegment/SegTypes#metasegmenty
    void TMetaSegmentator::FillMainContent() {
        bool wasDHA = false;
        for (const auto& pageSeg : PageSegments) {
            if (pageSeg.Empty())
                continue;
            const TString& segType = GetSegType(pageSeg.First->TypeId);
            if (segType == "DHC"sv) {
                MainContent.push_back(pageSeg);
            } else if (segType == "DCT"sv || segType == "DCM"sv ||
                segType == "LCN"sv)
            {
                if (wasDHA)
                    continue;
                MainContent.push_back(pageSeg);
            } else if (Cfg.NeedDMD4MainContent && segType == "DMD"sv) {
                MainContent.push_back(pageSeg);
            } else if (Cfg.NeedDHA4MainContent && segType == "DHA"sv) {
                MainContent.push_back(pageSeg);
                wasDHA = true;
            }
        }
    }

    const TVector<TPageSegment>& TMetaSegmentator::GetMainContent() const {
        return MainContent;
    }

    void TMetaSegmentator::ResetAllMetaSegments() {
        MainContent.clear();
    }
}

