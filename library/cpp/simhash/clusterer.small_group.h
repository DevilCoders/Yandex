#pragma once

/////////////////////////////////////////////////////////////////////
// Clusterer for groups of size 1, 2, 3
class TSmallGroupClusterer: private TNonCopyable {
public:
    TSmallGroupClusterer(TClustererConfig& config, THandler* handler)
        : Config(config)
        , Handler(handler)
    {
    }

public:
    void OnHypothesis(const TVector<TDoc>& hypothesis) {
        const size_t s = hypothesis.size();
        if (s == 0 || s > 3) {
            ythrow TWithBackTrace<yexception>() << "Wrong hypothesis size";
        }
        if (s == 1) {
            Cluster1(hypothesis);
        } else if (s == 2) {
            Cluster2(hypothesis);
        } else if (s == 3) {
            Cluster3(hypothesis);
        }
    }

private:
    inline void Cluster1(const TVector<TDoc>& hypothesis) {
        Handler->OnDocument(hypothesis[0], nullptr);
    }

    inline void Cluster2(const TVector<TDoc>& hypothesis) {
        if (CheckPair(hypothesis[0], hypothesis[1])) {
            if (hypothesis[0].Mainrank <= hypothesis[1].Mainrank) {
                Handler->OnDocument(hypothesis[0], &hypothesis[1]);
                Handler->OnDocument(hypothesis[1], nullptr);
            } else {
                Handler->OnDocument(hypothesis[1], &hypothesis[0]);
                Handler->OnDocument(hypothesis[0], nullptr);
            }
        } else {
            Handler->OnDocument(hypothesis[0], nullptr);
            Handler->OnDocument(hypothesis[1], nullptr);
        }
    }

    inline void Cluster3(const TVector<TDoc>& hypothesis) {
        bool cmp01 = CheckPair(hypothesis[0], hypothesis[1]);
        bool cmp12 = CheckPair(hypothesis[1], hypothesis[2]);
        bool cmp02 = CheckPair(hypothesis[0], hypothesis[2]);
        const ui8 total = (cmp01 ? 1 : 0) + (cmp02 ? 1 : 0) + (cmp12 ? 1 : 0);
        if (total == 3) {
            const TDoc* dm = nullptr;
            const TDoc* d1 = nullptr;
            const TDoc* d2 = nullptr;
            if (hypothesis[0].Mainrank >= hypothesis[1].Mainrank) {
                if (hypothesis[0].Mainrank >= hypothesis[2].Mainrank) {
                    dm = &hypothesis[0];
                    d1 = &hypothesis[1];
                    d2 = &hypothesis[2];
                } else {
                    dm = &hypothesis[2];
                    d1 = &hypothesis[0];
                    d2 = &hypothesis[1];
                }
            } else {
                if (hypothesis[1].Mainrank >= hypothesis[2].Mainrank) {
                    dm = &hypothesis[1];
                    d1 = &hypothesis[0];
                    d2 = &hypothesis[2];
                } else {
                    dm = &hypothesis[2];
                    d1 = &hypothesis[0];
                    d2 = &hypothesis[1];
                }
            }
            Handler->OnDocument(*d1, dm);
            Handler->OnDocument(*d2, dm);
            Handler->OnDocument(*dm, nullptr);
        } else if (total == 2) {
            const TDoc* d1 = nullptr;
            const TDoc* d2 = nullptr;
            const TDoc* d = nullptr;
            if (!cmp01) {
                d1 = &hypothesis[0];
                d2 = &hypothesis[1];
                d = &hypothesis[2];
            } else if (!cmp02) {
                d1 = &hypothesis[0];
                d2 = &hypothesis[2];
                d = &hypothesis[1];
            } else {
                d1 = &hypothesis[1];
                d2 = &hypothesis[2];
                d = &hypothesis[0];
            }
            if (d1->Mainrank < d2->Mainrank) {
                DoSwap(d1, d2);
            }
            if (d->Mainrank >= d1->Mainrank) {
                Handler->OnDocument(*d1, d);
                Handler->OnDocument(*d2, d);
                Handler->OnDocument(*d, nullptr);
            } else {
                Handler->OnDocument(*d, d1);
                Handler->OnDocument(*d1, nullptr);
                Handler->OnDocument(*d2, nullptr);
            }
        } else if (total == 1) {
            const TDoc* d1 = nullptr;
            const TDoc* d2 = nullptr;
            const TDoc* d = nullptr;
            if (cmp01) {
                d1 = &hypothesis[0];
                d2 = &hypothesis[1];
                d = &hypothesis[2];
            } else if (cmp02) {
                d1 = &hypothesis[0];
                d2 = &hypothesis[2];
                d = &hypothesis[1];
            } else {
                d1 = &hypothesis[1];
                d2 = &hypothesis[2];
                d = &hypothesis[0];
            }
            if (d1->Mainrank >= d2->Mainrank) {
                Handler->OnDocument(*d2, d1);
                Handler->OnDocument(*d1, nullptr);
            } else {
                Handler->OnDocument(*d1, d2);
                Handler->OnDocument(*d2, nullptr);
            }
            Handler->OnDocument(*d, nullptr);
        } else if (total == 0) {
            Handler->OnDocument(hypothesis[0], nullptr);
            Handler->OnDocument(hypothesis[1], nullptr);
            Handler->OnDocument(hypothesis[2], nullptr);
        }
    }

    inline bool CheckPair(const TDoc& left, const TDoc& right) {
        return (HammingDistance(left.Sketch.Simhash, right.Sketch.Simhash) <= Config.CompareConfig.SimhashTreshold) &&
               (DifferenceRatio(left.Sketch.DocLength, right.Sketch.DocLength) <= Config.CompareConfig.LengthTreshold);
    }

private:
    TClustererConfig& Config;
    THandler* Handler;
};
//
/////////////////////////////////////////////////////////////////////
