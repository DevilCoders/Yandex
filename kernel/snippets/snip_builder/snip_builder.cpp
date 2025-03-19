#include "snip_builder.h"
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/single_snip.h>

#include <kernel/snippets/config/config.h>

namespace NSnippets {

    void TSnipBuilder::ConvertParts(TVector<TSingleSnip>& p, size_t i, TPos l, TPos r) {
        p.clear();
        p.reserve(Parts.size());
        for (size_t j = 0; j < Parts.size(); ++j) {
            if (j == i) {
                p.push_back(TSingleSnip(l, r, SentsMatchInfo));
            } else {
                p.push_back(TSingleSnip(Parts[j].L, Parts[j].R, SentsMatchInfo));
            }
        }
    }

    void TSnipBuilder::ConvertParts(TVector<TSingleSnip>& p) const {
        p.clear();
        p.reserve(Parts.size());
        for (size_t j = 0; j < Parts.size(); ++j) {
            p.push_back(TSingleSnip(Parts[j].L, Parts[j].R, SentsMatchInfo));
        }
    }

    bool TSnipBuilder::JustGrow(size_t i, TPos l, TPos r) {
        // shift boundaries due to other parts
        if (i > 0 && Parts[i - 1].R >= l)
            l = Parts[i - 1].R.Next();
        if (i + 1 < Parts.size() && Parts[i + 1].L <= r)
            r = Parts[i + 1].L.Prev();
        // part coord are the same
        if (r == Parts[i].R && l == Parts[i].L) {
            return false;
        }

        // reached end
        if (l == SentsMatchInfo.SentsInfo.End<TPos>() || r == SentsMatchInfo.SentsInfo.End<TPos>()) {
            return false;
        }
        // something nasty happened
        if (l > r) {
            return false;
        }

        // check part size
        float len = SpanLen.CalcLength(TSingleSnip(l, r, SentsMatchInfo));
        if (len > MaxPartSize) {
            return false;
        }

        // check overall size
        TVector<TSingleSnip> converted;
        ConvertParts(converted, i, l, r);
        len = SpanLen.CalcLength(converted);
        if (len > MaxSize) {
            return false;
        }

        // assign new boundaries
        Parts[i].L = l;
        Parts[i].R = r;
        return true;
    }

    TSnipBuilder::TSnipBuilder(const TSentsMatchInfo& match, const TWordSpanLen& wordSpanLen, float maxSize, float maxPartSize)
      : SpanLen(wordSpanLen)
      , SentsMatchInfo(match)
      , MaxPartSize(maxPartSize)
      , MaxSize(maxSize)
    {
    }

    void TSnipBuilder::Reset() {
        Parts.clear();
    }

    TSnip TSnipBuilder::Get(double weight, TFactorStorage&& factors) const {
        TSnip::TSnips res;
        for (TParts::const_iterator i = Parts.begin(); i != Parts.end(); ++i)
            res.push_back(TSingleSnip(i->L, i->R, SentsMatchInfo));
        return TSnip(res, weight, std::move(factors));
    }

    TSnip TSnipBuilder::Get(const TInvalidWeight& invalidWeight) const {
        TSnip::TSnips res;
        for (TParts::const_iterator i = Parts.begin(); i != Parts.end(); ++i)
            res.push_back(TSingleSnip(i->L, i->R, SentsMatchInfo));
        return TSnip(res, invalidWeight);
    }

    bool TSnipBuilder::Add(const TPos& a, const TPos& b) {
        // check new part length
        if (SpanLen.CalcLength(TSingleSnip(a, b, SentsMatchInfo)) > MaxPartSize)
            return false;

        // check overall length
        TVector<TSingleSnip> p;
        ConvertParts(p);
        p.push_back(TSingleSnip(a, b, SentsMatchInfo));
        if (SpanLen.CalcLength(p) > MaxSize)
            return false;

        // add new part
        Parts.push_back(TPart(a, b));
        return true;
    }

    float TSnipBuilder::GetSize() const {
        TVector<TSingleSnip> p;
        ConvertParts(p);
        return SpanLen.CalcLength(p);
    }

    int TSnipBuilder::GetPartsSize() const {
        return Parts.size();
    }

    TSnipBuilder::TPos TSnipBuilder::GetPartL(int part) const {
        return Parts[part].L;
    }

    TSnipBuilder::TPos TSnipBuilder::GetPartR(int part) const {
        return Parts[part].R;
    }

    void TSnipBuilder::GlueTornSents() {
        if (Parts.size() <= 1) {
            return;
        }
        size_t cnt = 1;
        for (size_t i = 1; i != Parts.size(); ++i) {
            if (!Parts[i].L.IsFirstInSent() && Parts[cnt-1].R.Next() == Parts[i].L) {
                TVector<TSingleSnip> pparts;
                bool glued = false;
                for (size_t j = 0; j < Parts.size(); ++j) {
                    if (j != i && j != (cnt - 1)) {
                        pparts.push_back(TSingleSnip(Parts[j].L, Parts[j].R, SentsMatchInfo));
                    } else {
                        if (!glued) {
                            pparts.push_back(TSingleSnip(Parts[cnt - 1].L, Parts[i].R, SentsMatchInfo));
                            glued = true;
                        }
                    }
                }
                if (SpanLen.CalcLength(pparts) <= MaxSize) {
                    Parts[cnt - 1].R = Parts[i].R;
                } else {
                    Parts[cnt++] = Parts[i];
                }
            } else {
                Parts[cnt++] = Parts[i];
            }
        }
        Parts.erase(Parts.begin() + cnt, Parts.end());
    }

    bool TSnipBuilder::GrowLeftToSent() {
        bool res = false;
        for (size_t i = 0; i != Parts.size(); ++i) {
            if (JustGrow(i, Parts[i].L.FirstInSameSent(), Parts[i].R))
                res = true;
        }
        return res;
    }
    bool TSnipBuilder::GrowLeftWordInSent() {
        bool res = false;
        for (size_t i = 0; i != Parts.size(); ++i) {
            if (Parts[i].L.IsFirstInSent())
                continue;
            if (JustGrow(i, Parts[i].L.Prev(), Parts[i].R))
                res = true;
        }
        return res;
    }
    bool TSnipBuilder::GrowRightToSent() {
        bool res = false;
        for (size_t i = 0; i != Parts.size(); ++i) {
            if (JustGrow(i, Parts[i].L, Parts[i].R.LastInSameSent()))
                res = true;
        }
        return res;
    }
    bool TSnipBuilder::GrowRightWordInSent() {
        bool res = false;
        for (size_t i = 0; i != Parts.size(); ++i) {
            if (Parts[i].R.IsLastInSent()) {
                continue;
            }
            if (JustGrow(i, Parts[i].L, Parts[i].R.Next())) {
                res = true;
            }
        }
        return res;
    }
    void TSnipBuilder::GlueSents(const IRestr& restr) {
        if (Parts.size() <= 1) {
            return;
        }
        size_t cnt = 1;
        for (size_t i = 1; i != Parts.size(); ++i) {
            if (Parts[i].L.IsFirstInSent() && Parts[cnt - 1].R.Next() == Parts[i].L && !restr(Parts[cnt - 1].R.GetSent())) {
                TVector<TSingleSnip> pparts;
                bool glued = false;
                for (size_t j = 0; j < Parts.size(); ++j) {
                    if (j != i && j != (cnt - 1)) {
                        pparts.push_back(TSingleSnip(Parts[j].L, Parts[j].R, SentsMatchInfo));
                    } else {
                        if (!glued) {
                            pparts.push_back(TSingleSnip(Parts[cnt - 1].L, Parts[i].R, SentsMatchInfo));
                            glued = true;
                        }
                    }
                }

                if (SpanLen.CalcLength(pparts) <= MaxSize) {
                    Parts[cnt - 1].R = Parts[i].R;
                } else {
                    Parts[cnt++] = Parts[i];
                }
            } else {
                Parts[cnt++] = Parts[i];
            }
        }
        Parts.erase(Parts.begin() + cnt, Parts.end());
    }
    bool TSnipBuilder::GrowRightMoreSent(const IRestr& restr) {
        bool res = false;
        for (size_t i = 0; i != Parts.size(); ++i) {
            const TPart& p = Parts[i];
            if (p.R.GetSent() + 1 == p.R.GetInfo()->SentencesCount()) {
                continue;
            }
            if (restr(p.R.GetSent())) {
                continue;
            }
            TPos r = TPos(TSentWord(p.L.GetInfo(), p.R.GetSent() + 1, 0)).LastInSameSent();
            if (JustGrow(i, p.L, r)) {
                res = true;
            }
        }
        return res;
    }

}
