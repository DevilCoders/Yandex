#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

struct TSegment {
    double Goal;
    double Prediction;

    TString Url;
    size_t DocId;
    // stats of the content
    size_t Lcs;
    size_t MainSize; // main content size
    size_t Size;

    static bool PredictionLess(const TSegment& l, const TSegment& r) {
        if (l.Prediction != r.Prediction) {
            return l.Prediction < r.Prediction;
        }
        if (l.Goal != r.Goal) {
            return l.Goal > r.Goal;
        }
        return l.DocId < r.DocId;
    }

    static bool PredictionGreater(const TSegment& l, const TSegment& r) {
        if (l.Prediction != r.Prediction) {
            return l.Prediction > r.Prediction;
        }
        if (l.Goal != r.Goal) {
            return l.Goal < r.Goal;
        }
        return l.DocId > r.DocId;
    }
};

struct TPool {
    size_t NumDocs;
    TVector<TSegment> Segments;

    TPool()
        : NumDocs(0)
    {
    }
};

TAutoPtr<TPool> LoadPool(const TString& fname) {
    TAutoPtr<TPool> pool = new TPool();

    THashMap<TString, size_t> docIds;

    TFileInput in(fname);
    TString line;
    while (in.ReadLine(line)) {
        TSegment s;
        TStringBuf tok(line);
        tok.NextTok('\t');

        s.Goal = FromString(tok.NextTok('\t'));

        TStringBuf tok2 = tok.NextTok('\t');
        tok2.NextTok(' ');
        s.Lcs = FromString(tok2.NextTok(' '));
        s.MainSize = FromString(tok2.NextTok(' '));
        s.Size = FromString(tok2.NextTok(' '));
        s.Url = tok2.NextTok(' ');

        size_t& docId = docIds[s.Url];
        if (docId == 0) {
            // just inserted so assign a new one
            docId = docIds.size();
        }
        s.DocId = docId;

        tok.NextTok('\t');
        s.Prediction = FromString(tok.NextTok('\t'));

        pool->Segments.push_back(s);
    }

    Sort(pool->Segments.begin(), pool->Segments.end(), TSegment::PredictionGreater);
    pool->NumDocs = docIds.size();

    return pool;
}

struct TMetrics {
    static double Beta;

    double Precision;
    double Recall;

    TMetrics(double precision=0, double recall=0)
        : Precision(precision)
        , Recall(recall)
    {
    }

    double FMeasure() const {
        if (Precision > 0 || Recall > 0) {
            return (1 + Beta) * Precision * Recall / (Beta * Precision + Recall);
        }
        return 0;
    }

    TMetrics Divide(size_t precisionCount, size_t recallCount) const {
        return TMetrics(Precision / precisionCount, Recall / recallCount);
    }

    TMetrics& operator += (const TMetrics& rhs) {
        Precision += rhs.Precision;
        Recall += rhs.Recall;
        return *this;
    }

    TMetrics operator + (const TMetrics& rhs) const {
        TMetrics res(*this);
        res += rhs;
        return res;
    }

    TMetrics& operator -= (const TMetrics& rhs) {
        Precision -= rhs.Precision;
        Recall -= rhs.Recall;
        return *this;
    }

    void Print(IOutputStream& s) const {
        s << "Precision: " << Precision << "\n"
          << "Recall:    " << Recall << "\n"
          << "F-measure: " << FMeasure() << "\n";
    }
};

double TMetrics::Beta = 1;

struct TDocStat {
    TMetrics Metrics;
    size_t NumTakenSegments;
    size_t TakenSize; // measured in words
    size_t TakenLcs;  // measured in words

    TDocStat()
        : NumTakenSegments(0)
        , TakenSize(0)
        , TakenLcs(0)
    {
    }

    void AddSegment(const TSegment& s) {
        NumTakenSegments += 1;
        TakenLcs = Min(s.MainSize, TakenLcs + s.Lcs);
        TakenSize += s.Size;
        Metrics.Precision = (double)TakenLcs / TakenSize;
        Metrics.Recall = (double)TakenLcs / s.MainSize;
    }
};

// take 1 best segment from each url
// segments will be sorted by ascending predictions
TVector<TSegment> TakeBestSegments(const TPool& pool) {
    TVector<TSegment> result;
    THashSet<size_t> seenDocs;
    for (size_t i = 0; i < pool.Segments.size(); ++i) {
        const TSegment& s = pool.Segments[i];
        if (!seenDocs.contains(s.DocId)) {
            seenDocs.insert(s.DocId);
            result.push_back(s);
        }
    }
    Sort(result.begin(), result.end(), TSegment::PredictionLess);
    return result;
}

struct TThresholdStats {
    TMetrics SumMetrics;
    size_t NumTakenDocs;

    TThresholdStats()
        : NumTakenDocs(0)
    {
    }

    void AddDoc(const TMetrics& docMetrics) {
        SumMetrics += docMetrics;
        NumTakenDocs += 1;
    }

    void RemoveDoc(const TMetrics& docMetrics) {
        SumMetrics -= docMetrics;
        NumTakenDocs -= 1;
    }

    void AddSegment(const TSegment& s) {
        AddDoc(TMetrics((double)s.Lcs / s.Size, (double)s.Lcs / s.MainSize));
    }

    void RemoveSegment(const TSegment& s) {
        RemoveDoc(TMetrics((double)s.Lcs / s.Size, (double)s.Lcs / s.MainSize));
    }
};

double CalcUnitedFMeasure(const TThresholdStats& t1stats, const TThresholdStats& t2stats,
                          size_t numDocs)
{
    size_t numTakenDocs = t1stats.NumTakenDocs + t2stats.NumTakenDocs; // see comment in ChooseThresholds() why this is legal
    TMetrics sumMetrics = t1stats.SumMetrics + t2stats.SumMetrics;
    return sumMetrics.Divide(numTakenDocs, numDocs).FMeasure();
}

void ChooseThreshold2(const TVector<TSegment>& segments, const TThresholdStats& t1stats,
                      size_t numDocs,
                      double* threshold2, TThresholdStats* t2stats)
{
    *threshold2 = 100;
    *t2stats = TThresholdStats();

    double bestFMeasure = 0;

    TThresholdStats candidateStats;

    for (size_t i = 0; i < segments.size(); ++i) {
        const TSegment& s = segments[segments.size() - i - 1];
        candidateStats.AddSegment(s);
        double fMeasure = CalcUnitedFMeasure(t1stats, candidateStats, numDocs);
        if (fMeasure > bestFMeasure) {
            double nextPrediction = i + 1 < segments.size() ? segments[segments.size() - i - 1].Prediction : 0;
            *threshold2 = (s.Prediction + nextPrediction) / 2;
            *t2stats = candidateStats;
            bestFMeasure = fMeasure;
        }
    }
}

inline size_t CalcNextThreshold2Recalc(size_t numBestSegments) {
    return (numBestSegments < 100)
        ? numBestSegments - 1
        : numBestSegments * 0.95;
}


std::pair<double, double> ChooseThresholds(TPool& pool, bool chooseThreshold2) {
    double bestFMeasure = 0;
    std::pair<double, double> bestThresholds;

    // stats by docs/segments chosen by different thresholds
    // It is guaranteed that sets of docs chosen by different threshold do not intersect
    // so it is safe to sum NumTakenDocs from 2 statistics
    TThresholdStats t1stats;
    TThresholdStats t2stats;

    // Stuff related to threshold2 calculation
    double threshold2 = 100;
    TVector<TSegment> bestSegments;
    size_t nextThreshold2Recalc = 0;
    if (chooseThreshold2) {
        bestSegments = TakeBestSegments(pool);
        ChooseThreshold2(bestSegments, t1stats, pool.NumDocs, &threshold2, &t2stats);
        // we will recalc threshold2 only when bestSegmens.size() will become < than this variable:
        nextThreshold2Recalc = CalcNextThreshold2Recalc(bestSegments.size());
    }

    THashMap<size_t, TDocStat> docStats;

    for (size_t i = 0; i < pool.Segments.size(); ++i) {
        const TSegment& s = pool.Segments[i];
        TDocStat& docStat = docStats[s.DocId];

        if (chooseThreshold2) {
            // Check if we should remove segment from list of best segments and recalculate threshold2.
            // Because segments are sorted in the same order in bestSegments and in pool
            // it is enough to compare current segment only with last segment in bestSegments list.
            if (!bestSegments.empty() && bestSegments.back().DocId == s.DocId) {
                // Remove segment and recalculate metrics for segments chosen by threshold2
                bestSegments.pop_back();
                t2stats.RemoveSegment(s);
                // Check if bestSegments has changed enough so it makes sense rechoose threshold2
                if (bestSegments.size() <= nextThreshold2Recalc) {
                    ChooseThreshold2(bestSegments, t1stats, pool.NumDocs, &threshold2, &t2stats);
                    nextThreshold2Recalc = CalcNextThreshold2Recalc(bestSegments.size());
                }
            }
        }

        if (docStat.NumTakenSegments > 0) {
            t1stats.RemoveDoc(docStat.Metrics);
        }
        docStat.AddSegment(s);
        t1stats.AddDoc(docStat.Metrics);

        double fMeasure = CalcUnitedFMeasure(t1stats, t2stats, pool.NumDocs);
        if (fMeasure > bestFMeasure) {
            double nextPrediction = i + 1 < pool.Segments.size() ? pool.Segments[i+1].Prediction : 0;
            bestFMeasure = fMeasure;
            bestThresholds = std::make_pair((s.Prediction + nextPrediction) / 2, threshold2);
        }
    }

    return bestThresholds;
}

TMetrics CalcMetrics(TPool& pool, std::pair<double, double> thresholds) {
    THashMap<size_t, TDocStat> docStats;

    for (size_t i = 0; i < pool.Segments.size(); ++i) {
        const TSegment& s = pool.Segments[i];
        if (s.Prediction > thresholds.first) {
            TDocStat& docStat = docStats[s.DocId];
            docStat.AddSegment(s);
        }
    }

    for (size_t i = 0; i < pool.Segments.size(); ++i) {
        const TSegment& s = pool.Segments[i];
        if (s.Prediction > thresholds.second && !docStats.contains(s.DocId)) {
            TDocStat& docStat = docStats[s.DocId];
            docStat.AddSegment(s);
        }
    }

    TMetrics sumMetrics;

    THashMap<size_t, TDocStat>::const_iterator it = docStats.begin();
    for (; it != docStats.end(); ++it) {
        sumMetrics += it->second.Metrics;
    }

    return sumMetrics.Divide(docStats.size(), pool.NumDocs);
}

int main(int argc, const char** argv) {
    TString learnFile;
    TString testFile;

    using namespace NLastGetopt;
    TOpts opts(TOpts::Default());

    opts.AddLongOption("learn", "learn pool").Required()
        .StoreResult(&learnFile).RequiredArgument("FILE");
    opts.AddLongOption("test", "test pool")
        .StoreResult(&testFile).RequiredArgument("FILE");
    opts.AddLongOption("beta", "coefficient for F-measure")
        .StoreResult(&TMetrics::Beta).DefaultValue(ToString(TMetrics::Beta));

    TOptsParseResult res(&opts, argc, argv);

    TAutoPtr<TPool> learn = LoadPool(learnFile);
    TAutoPtr<TPool> test;
    if (!!testFile) {
        test = LoadPool(testFile);
    }

    for (size_t i = 0; i < 2; ++i) {
        bool chooseThreshold2 = i == 1;
        std::pair<double, double> thresholds = ChooseThresholds(*learn, chooseThreshold2);
        if (i != 0) {
            Cout << "\n+++++++\n\n";
        }
        Cout << "Threshold-1: " << thresholds.first << "\n"
             << "Threshold-2: " << thresholds.second << "\n"
             << "=== Learn:\n";
        CalcMetrics(*learn, thresholds).Print(Cout);
        if (!!test) {
            Cout << "=== Test:\n";
            CalcMetrics(*test, thresholds).Print(Cout);
        }
    }

    return 0;
}
