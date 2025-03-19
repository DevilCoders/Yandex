#include "restr.h"
#include "segments_rules.h"
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/similarity.h>

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/iface/archive/sent.h>
#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/tarc/iface/tarcface.h>

namespace NSnippets {
    static TVector<bool> GetSkipBans(bool byLink, const TSentsInfo& info) {
        TVector<bool> res(info.SentencesCount(), byLink);
        if (byLink) {
            return res;
        }
        const TVector<TArchiveZoneSpan>& z = info.GetTextArcMarkup().GetZone(AZ_TEXTAREA).Spans;
        int j = 0;
        for (int i = 1; i < info.SentencesCount(); ++i) {
            const TArchiveSent& arcSent = info.GetArchiveSent(i);
            if (arcSent.SourceArc != ARC_TEXT) {
                continue;
            }
            while (j < z.ysize() && z[j].SentEnd < arcSent.SentId) {
                ++j;
            }
            if (j < z.ysize() && z[j].SentBeg <= arcSent.SentId) {
                res[i - 1] = true;
            }
        }
        return res;
    }

    static TVector<bool> CalcSkipRestr(const TVector<bool>& bans, const TSentsInfo& info, bool imgAttrs) {
        TVector<bool> res = bans;
        for (int sentId = 0; sentId < res.ysize(); ++sentId) {
            if (sentId + 1 >= info.SentencesCount()) {
                res[sentId] = false;
                continue;
            }
            if (info.SentVal[sentId].SourceType != SST_TEXT) {
                res[sentId] = info.SentVal[sentId].SourceType != info.SentVal[sentId + 1].SourceType;
                continue;
            }
            if (!info.IsSentIdFirstInArchiveSent(sentId + 1)) {
                res[sentId] = false;
                continue;
            }
            const TArchiveSent& curSent = info.GetArchiveSent(sentId);
            const TArchiveSent& nextSent = info.GetArchiveSent(sentId + 1);
            Y_ASSERT(curSent.SourceArc == nextSent.SourceArc);
            if (curSent.SentId + 1 != nextSent.SentId) {
                res[sentId] = true;
                continue;
            }
            if (imgAttrs) {
                if (curSent.Attr != nextSent.Attr) {
                    res[sentId] = true;
                    continue;
                }
            }
        }
        return res;
    }

    TSkippedRestr::TSkippedRestr(bool byLink, const TSentsInfo& sentsInfo, const TConfig& cfg)
      : MRestr(CalcSkipRestr(GetSkipBans(byLink, sentsInfo), sentsInfo, cfg.ImgSearch()))
    {
    }

    static TVector<bool> CalcParaRestr(const TSentsInfo& info) {
        const int n = info.SentencesCount();
        TVector<bool> res(n, false);
        for (int i = 0; i < n; ++i) {
            if (info.SentVal[i].ParaLenInSents >= 4 && info.IsSentIdLastInPara(i)) {
                res[i] = true;
            }
        }
        return res;
    }

    TParaRestr::TParaRestr(const TSentsInfo& info)
      : MRestr(CalcParaRestr(info))
    {
    }

    static TVector<bool> CalcQualRestr(const TSentsMatchInfo& info) {
        const int n = info.SentsInfo.SentencesCount();
        TVector<bool> res(n, false);
        TVector<bool> lq(n, false);
        for (int i = 0; i < n; ++i)
            lq[i] = info.GetSentQuality(i) < 0;
        for (int i = 0; i < n; ++i) {
            if (lq[i]) {
                for (int j = i - 1; j >= 0 && info.GetSentQuality(j) == 0 && !lq[j]; --j)
                    lq[j] = true;
                for (int j = i + 1; j < n && info.GetSentQuality(j) == 0 && !lq[j]; ++j)
                    lq[j] = true;
            }
        }
        for (int i = 0; i + 1 < n; ++i) {
            res[i] = lq[i] != lq[i + 1];
        }
        return res;
    }

    TQualRestr::TQualRestr(const TSentsMatchInfo& info)
      : MRestr(CalcQualRestr(info))
    {
    }

    static TVector<bool> CalcSegmentRestr(const TSentsMatchInfo& info) {
        const int n = info.SentsInfo.SentencesCount();
        TVector<bool> res(n, false);
        const NSegments::TSegmentsInfo* segments = info.SentsInfo.GetSegments();
        using namespace NSegments;
        if (segments && segments->HasData()) {
            for (int i = 1; i < n; ++i) {
                if (!info.SentsInfo.IsSentIdFirstInArchiveSent(i)) {
                    continue;
                }
                TSegmentCIt prevSeg = segments->GetArchiveSegment(info.SentsInfo.GetArchiveSent(i - 1));
                TSegmentCIt curSeg = segments->GetArchiveSegment(info.SentsInfo.GetArchiveSent(i));

                if (!segments->IsValid(curSeg) || !segments->IsValid(prevSeg)) {
                    continue;
                }

                if (curSeg != prevSeg) {
                    NSegm::ESegmentType curType = segments->GetType(curSeg);
                    NSegm::ESegmentType prevType = segments->GetType(prevSeg);
                    NSegm::ESegmentType nextType = segments->GetType(curSeg + 1);
                    if (curType != prevType
                            && !NRules::FromHeader(*prevSeg, *curSeg)
                            && !NRules::FromBad2Good(prevType, curType, info.SentHasMatches(i - 1))
                            &&
                            ( false
                              || NRules::GoodIsNear(prevType, curType, nextType)
                              || NRules::Footer(curType)
                              || NRules::ToHeader(*prevSeg, *curSeg)
                              || NRules::Inputs(*prevSeg, *curSeg)
                              || NRules::Words(*prevSeg, *curSeg)
                              || NRules::Ads(*prevSeg, *curSeg) || NRules::Ads(*curSeg, *prevSeg)
                              || NRules::TextVsLinks(*prevSeg, *curSeg) || NRules::TextVsLinks(*curSeg, *prevSeg)
                              || NRules::LocVsExt(*prevSeg, *curSeg) || NRules::LocVsExt(*curSeg, *prevSeg)
                            )) {
                        res[i-1] = true;
                    }
                }
            }
        }
        return res;
    }

    TSegmentRestr::TSegmentRestr(const TSentsMatchInfo& info)
      : MRestr(CalcSegmentRestr(info))
    {
    }

    void CalcSimilarRestr(TVector<bool>& res, const TSentsMatchInfo& info) {
        const int n = info.SentsInfo.SentencesCount();
        for (int i = 0; i < n; ++i) {
            for (int j = i - 1; j >= 0 && i - j < 4 && !res[j]; --j) {
                const double sim = GetSimilarity(info.GetSentEQInfos(i), info.GetSentEQInfos(j));
                if (sim >= 0.5) {
                    res[i - 1] = true;
                    break;
                }
            }
        }
    }
    TSimilarRestr::TSimilarRestr(const TSentsMatchInfo& info)
        : MRestr(info.SentsInfo.SentencesCount(), false)
    {
        CalcSimilarRestr(MRestr, info);
    }

}
