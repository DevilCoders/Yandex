#include "option_enums.h"

#include <library/cpp/hnsw/index/dense_vector_distance.h>
#include <library/cpp/hnsw/index/dense_vector_index.h>

#include <library/cpp/containers/limited_heap/limited_heap.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/threading/local_executor/local_executor.h>

#include <util/folder/path.h>
#include <util/random/shuffle.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/string/vector.h>
#include <util/system/hp_timer.h>


const TVector<size_t> QUANTILES = {50, 95, 99, 100};

struct TOptions {
    TFsPath VectorFilename;
    TFsPath HnswFilename;
    TString QueriesFilename;
    EQueriesFormat QueriesFormat;
    TVector<size_t> SearchNeighborhoodSizes;
    TVector<size_t> DistanceCalcLimits;
    TVector<size_t> TopSizes;
    EVectorComponentType VectorComponentType;
    EDistance Distance;
    size_t Dimension;
    size_t NumThreads;
    bool UseCache = false;
    bool Multithreading = false;
    bool ShowDistanceCounts = false;

    static TVector<size_t> ParseCsv(const TString& csv) {
        TVector<size_t> result;
        for (auto split : StringSplitter(csv).Split(',')) {
            result.push_back(FromString<size_t>(split.Token()));
        }
        return result;
    }

    TOptions(int argc, char** argv) {
        using namespace NLastGetopt;
        TOpts opts = NLastGetopt::TOpts::Default();
        TString searchNeighborhoodSizeCsv;
        TString distanceCalcLimitCsv;
        TString topSizeCsv;
        opts
            .AddHelpOption();
        opts
            .AddLongOption('v', "vectors")
            .StoreResult(&VectorFilename)
            .Required()
            .Help("File with vectors from index.");
        opts
            .AddLongOption('d', "dimension")
            .Required()
            .StoreResult(&Dimension)
            .Help("Dimension of vectors.");
        opts
            .AddLongOption('t', "type")
            .RequiredArgument("STRING")
            .StoreResult(&VectorComponentType)
            .Required()
            .Help("One of { i8, i32, float }. Type of vectors' components.");
        opts
            .AddLongOption('D', "distance")
            .RequiredArgument("STRING")
            .StoreResult(&Distance)
            .Required()
            .Help("One of { l1_distance, l2_sqr_distance, dot_product }.");
        opts
            .AddLongOption('q', "queries")
            .StoreResult(&QueriesFilename)
            .Required()
            .Help("File with query features");
        opts
            .AddLongOption("queries-format")
            .DefaultValue(EQueriesFormat::Binary)
            .StoreResult(&QueriesFormat)
            .Help("Format of queries file: binary or tsv");
        opts
            .AddLongOption('i', "index")
            .StoreResult(&HnswFilename)
            .Required()
            .Help("File with HNSW index");
        opts
            .AddLongOption('r', "search-neighborhood-sizes")
            .DefaultValue("10,30,50,70,100,200,500,1000,2000")
            .StoreResult(&searchNeighborhoodSizeCsv)
            .Help("Comma separated list of search neighborhood sizes.");
        opts
            .AddLongOption("distance-calc-limits")
            .StoreResult(&distanceCalcLimitCsv)
            .Help("Comma separated list of distance calc limits for each search neighborhood size.");
        opts
            .AddLongOption('k', "recall-at-k")
            .DefaultValue("1,5,10,50")
            .StoreResult(&topSizeCsv)
            .Help("Comma separated list of top sizes to measure recall.");
        opts
            .AddLongOption('n', "num-threads")
            .DefaultValue("32")
            .StoreResult(&NumThreads);
        opts
            .AddLongOption("use-cache")
            .NoArgument()
            .StoreValue(&UseCache, true)
            .Help("Cache ground truth nearest vectors. Caching is based on vectors and queries filenames.");
        opts
            .AddLongOption("use-multithreading")
            .NoArgument()
            .StoreValue(&Multithreading, true)
            .Help("Use multithreading when searching nearest in index");
        opts
            .AddLongOption("show-distance-counts")
            .NoArgument()
            .StoreValue(&ShowDistanceCounts, true)
            .Help("Show counts of distance calculations per query.");
        TOptsParseResult res(&opts, argc, argv);
        SearchNeighborhoodSizes = ParseCsv(searchNeighborhoodSizeCsv);
        DistanceCalcLimits = distanceCalcLimitCsv.empty()
                            ? TVector<size_t>(SearchNeighborhoodSizes.size(), Max<size_t>())
                            : ParseCsv(distanceCalcLimitCsv);
        Y_ENSURE(SearchNeighborhoodSizes.size() == DistanceCalcLimits.size());
        TopSizes = ParseCsv(topSizeCsv);
    }
};

template <class T, class TDistance>
struct TDistanceWithCounter : TDistance {
public:
    TDistanceWithCounter() = default;

    TDistanceWithCounter(const TDistance& base)
        : TDistance(base)
    {
    }

    auto operator()(const T* a, const T* b, size_t dimension) const {
        Counter++;
        return TDistance::operator()(a, b, dimension);
    }

    size_t GetDistanceCount() {
        return Counter;
    }
private:
    mutable size_t Counter = 0;
};

template<class TValue, class TDistance>
class TRecallMeasurer {
    using TIndex = NHnsw::THnswDenseVectorIndex<TValue>;
public:
    struct TSearchResult {
        typename TDistance::TResult Dist = 0.0;
        ui32 Id = 0;

        TSearchResult(typename TDistance::TResult dist, ui32 id)
            : Dist(dist)
            , Id(id)
        {
        }

        bool operator<(const TSearchResult& other) const {
            constexpr typename TDistance::TLess less;
            return less(Dist,  other.Dist) || Dist == other.Dist && Id < other.Id;
        }
    };


    static TVector<TVector<TValue>> LoadQueriesBinary(const TString& filename, size_t dimension) {
        TVector<TVector<TValue>> result;
        TBlob blob = TBlob::PrechargedFromFile(filename);
        for (TBlob::const_iterator pos = blob.Begin(); pos != blob.End(); pos += dimension * sizeof(TValue)) {
            const TValue* data = reinterpret_cast<const TValue*>(pos);
            result.emplace_back(data, data + dimension);
        }
        return result;
    }

    static TVector<TVector<TValue>> LoadQueriesTsv(const TString& filename, size_t dimension) {
        TVector<TVector<TValue>> result;
        TFileInput fin(filename);
        for (TString line; fin.ReadLine(line);) {
            auto featureStrs = StringSplitter(line).SplitBySet(" \t").ToList<TString>();
            if (featureStrs.back().empty()) {
                featureStrs.pop_back();
            }
            Y_VERIFY(featureStrs.size() == dimension);
            result.emplace_back(Scan<TValue>(featureStrs));
        }
        return result;
    }

    static TFsPath GetGroundTruthFileName(const TString& vectorFilename,
                                          const TString& queriesFilename) {
        return TFsPath(".") / TFsPath(vectorFilename).Basename() + TString(".") + TFsPath(queriesFilename).Basename() + TString(".gt");
    }

    static void SaveGroundTruth(const TString& vectorFilename,
                                const TString& queriesFilename,
                                const TVector<TVector<TSearchResult>>& groundTruth) {
        TFixedBufferFileOutput out(GetGroundTruthFileName(vectorFilename, queriesFilename));
        for (const auto &result : groundTruth) {
            out.Write(result.data(), result.size() * sizeof(TSearchResult));
        }
    }

    static bool TryLoadGroundTruth(const TString& vectorFilename,
                                   const TString& queriesFilename,
                                   size_t topSize,
                                   TVector<TVector<TSearchResult>>* groundTruth) {
        TFsPath file = GetGroundTruthFileName(vectorFilename, queriesFilename);
        if (!file.Exists()) {
            return false;
        }
        TBlob blob = TBlob::PrechargedFromFile(file);
        size_t numQueries = blob.Size() / sizeof(TSearchResult) / topSize;
        groundTruth->resize(numQueries);
        const TSearchResult* data = reinterpret_cast<const TSearchResult*>(blob.Begin());
        for (size_t i = 0; i < numQueries; ++i) {
            (*groundTruth)[i].assign(data, data + topSize);
            data += topSize;
        }
        Y_VERIFY(reinterpret_cast<TBlob::const_iterator>(data) == blob.End());
        return true;
    }

    static TVector<TVector<TSearchResult>> GetGroundTruth(const TValue* vectors,
                                                          size_t numVectors,
                                                          size_t dimension,
                                                          const TVector<TVector<TValue>>& queries,
                                                          size_t topSize) {
        TVector<TVector<TSearchResult>> groundTruth(queries.size());

        THPTimer watch;
        TAtomic done = 0;
        auto task = [&](int id) {
            const TDistance distanceFunc;
            const auto& query = queries[id];
            TLimitedHeap<TSearchResult, TLess<TSearchResult>> top(topSize, TLess<TSearchResult>());
            for (ui32 vectorId = 0; vectorId < numVectors; ++vectorId) {
                auto dist = distanceFunc(query.data(), vectors + vectorId * dimension, dimension);
                top.Insert({ dist, vectorId });
            }
            auto& result = groundTruth[id];
            result.reserve(top.GetSize());
            for (; !top.IsEmpty(); top.PopMin()) {
                result.push_back(top.GetMin());
            }
            std::reverse(result.begin(), result.end());
            size_t curDone = AtomicAdd(done, 1);
            if (curDone == queries.size() || curDone * 100 / queries.size() != (curDone + 1) * 100 / queries.size()) {
                Cerr << curDone * 100 / queries.size() << "% done\r" << Flush;
            }
        };
        NPar::LocalExecutor().ExecRange(task, 0, queries.size(), NPar::TLocalExecutor::WAIT_COMPLETE);
        Cerr << queries.size() / watch.Passed() << " rps brute force " << Endl;
        return groundTruth;
    }

    static TVector<TVector<TSearchResult>> GetGroundTruth(const TString& vectorFilename,
                                                          const TString& queriesFilename,
                                                          size_t dimension,
                                                          const TVector<TVector<TValue>>& queries,
                                                          size_t topSize,
                                                          bool useCache) {
        TVector<TVector<TSearchResult>> groundTruth;
        if (useCache && TryLoadGroundTruth(vectorFilename, queriesFilename, topSize, &groundTruth)) {
            Cerr << "Loaded groundTruth from file" << Endl;
            return groundTruth;
        }

        TBlob blob = TBlob::PrechargedFromFile(vectorFilename);
        const TValue* vectors = reinterpret_cast<const TValue*>(blob.Begin());
        size_t numVectors = blob.Size() / sizeof(TValue) / dimension;

        groundTruth = GetGroundTruth(vectors, numVectors, dimension, queries, topSize);

        if (useCache) {
            SaveGroundTruth(vectorFilename, queriesFilename, groundTruth);
        }
        return groundTruth;
    }

    static TVector<TSearchResult> GetNearestVectors(const TIndex& index,
                                                    const TValue* query,
                                                    size_t topSize,
                                                    size_t searchNeighborhoodSize,
                                                    size_t distanceCalcLimit,
                                                    const TDistance& distance) {
        auto matches = index.template GetNearestNeighbors<TDistance>(query, topSize, searchNeighborhoodSize, distanceCalcLimit, distance);
        TVector<TSearchResult> result;
        result.reserve(matches.size());
        for (const auto &match : matches) {
            result.emplace_back(match.Dist, match.Id);
        }
        Sort(result.begin(), result.end());
        return result;
    }

    template<class T>
    static void CalcQuantiles(TVector<T>& data, TVector<T>& resultQuantiles) {
        resultQuantiles.resize(QUANTILES.size());
        auto lastQuantile = data.begin();
        for (size_t quantileId = 0; quantileId < QUANTILES.size(); ++quantileId) {
            auto nextQuantile = data.begin() + data.size() * QUANTILES[quantileId] / 100;
            if (nextQuantile == data.end()) {
                nextQuantile = data.end() - 1;
            }
            NthElement(lastQuantile, nextQuantile, data.end());
            resultQuantiles[quantileId] = *nextQuantile;
            lastQuantile = nextQuantile;
        }
    }

    static TVector<double> MeasureRecall(const TIndex& index,
                                         const TVector<TVector<TValue>>& queries,
                                         const TVector<TVector<TSearchResult>>& groundTruth,
                                         size_t searchNeighborhoodSize,
                                         size_t distanceCalcLimit,
                                         const TVector<size_t>& topSizes,
                                         bool useMultithreading,
                                         TVector<double>& timeQuantiles,
                                         TVector<size_t>& distanceCountQuantiles) {
        Y_VERIFY(queries.size() == groundTruth.size());
        TVector<TVector<size_t>> perQueryHits(queries.size(), TVector<size_t>(topSizes.size()));
        TVector<double> perQueryTimes(queries.size());
        TVector<size_t> perQueryDistanceCounts(queries.size());
        auto task = [&](int id) {
            const auto& query = queries[id];
            THPTimer timer;
            TDistance distance;
            auto matches = GetNearestVectors(index, query.data(), topSizes.back(), searchNeighborhoodSize, distanceCalcLimit, distance);
            perQueryTimes[id] = timer.Passed();
            perQueryDistanceCounts[id] = distance.GetDistanceCount();
            for (size_t topSizeId = 0, i = 0, j = 0; topSizeId < topSizes.size() && i < groundTruth[id].size() && j < matches.size(); ++i) {
                if (i >= topSizes[topSizeId]) {
                    ++topSizeId;
                }
                if (groundTruth[id][i].Id == matches[j].Id) {
                    ++perQueryHits[id][topSizeId];
                    ++j;
                }
            }
            for (size_t i = 1; i < topSizes.size(); ++i) {
                perQueryHits[id][i] += perQueryHits[id][i - 1];
            }
        };
        if (useMultithreading) {
            NPar::LocalExecutor().ExecRange(task, 0, queries.size(), NPar::TLocalExecutor::WAIT_COMPLETE);
        } else {
            for (size_t id = 0; id < queries.size(); ++id) {
                task(id);
            }
        }
        CalcQuantiles(perQueryTimes, timeQuantiles);
        CalcQuantiles(perQueryDistanceCounts, distanceCountQuantiles);

        TVector<double> recalls;
        recalls.reserve(topSizes.size());
        for (size_t topSizeId = 0; topSizeId < topSizes.size(); ++topSizeId) {
            size_t totalHits = 0;
            for (size_t id = 0; id < queries.size(); ++id) {
                totalHits += perQueryHits[id][topSizeId];
            }
            recalls.push_back(totalHits / (queries.size() * topSizes[topSizeId] * 1.0));
        }
        return recalls;
    }

    static void MeasureRecallsAndGetReport(const TIndex& index,
                                           const TVector<TVector<TValue>>& queries,
                                           const TVector<TVector<TSearchResult>>& groundTruth,
                                           const TVector<size_t>& searchNeighborhoodSizes,
                                           const TVector<size_t>& distanceCalcLimits,
                                           const TVector<size_t>& topSizes,
                                           bool useMultithreading,
                                           size_t numThreads,
                                           bool showDistanceCounts) {
        if (!useMultithreading) {
            numThreads = 1;
        }
        Cout << "searchNeighborhoodSize\trps\tmsPerQuery";
        for (size_t quantile : QUANTILES) {
            Cout << "\tmsPerQuery@" << quantile;
        }
        if (showDistanceCounts) {
            for (size_t quantile : QUANTILES) {
                Cout << "\tcalcsPerQuery@" << quantile;
            }
        }
        for (size_t topSize : topSizes) {
            Cout << "\trecall@" << topSize;
        }
        Cout << Endl;

        for (size_t i = 0; i < searchNeighborhoodSizes.size(); ++i) {
            size_t searchNeighborhoodSize = searchNeighborhoodSizes[i];
            size_t distanceCalcLimit = distanceCalcLimits[i];
            THPTimer watch;
            TVector<double> timeQuantiles;
            TVector<size_t> distanceCountQuantiles;
            TVector<double> recalls = MeasureRecall(index, queries, groundTruth, searchNeighborhoodSize, distanceCalcLimit,
                                                    topSizes, useMultithreading, timeQuantiles, distanceCountQuantiles);
            double msPerQuery = watch.Passed() * 1000.0 / queries.size() * numThreads;
            double rps = queries.size() / watch.Passed() / numThreads;

            Cout << searchNeighborhoodSize << '\t' << rps << '\t' << msPerQuery;
            for (double quantile : timeQuantiles) {
                Cout << '\t' << quantile * 1000.0;
            }
            if (showDistanceCounts) {
                for (double quantile : distanceCountQuantiles) {
                    Cout << '\t' << quantile;
                }
            }
            for (double recall : recalls) {
                Cout << '\t' << recall;
            }
            Cout << Endl;
        }
    }

    static void Main(const TOptions& opts) {
        Cerr << "Loading queries..." << Endl;
        TVector<TVector<TValue>> queries;
        if (opts.QueriesFormat == EQueriesFormat::Binary) {
            queries = LoadQueriesBinary(opts.QueriesFilename, opts.Dimension);
        } else if (opts.QueriesFormat == EQueriesFormat::Tsv) {
            queries = LoadQueriesTsv(opts.QueriesFilename, opts.Dimension);
        }

        Cerr << "Bruteforcing for ground truth..." << Endl;
        auto groundTruth = GetGroundTruth(opts.VectorFilename,
                opts.QueriesFilename,
                opts.Dimension,
                queries,
                opts.TopSizes.back(),
                opts.UseCache);

        Cerr << "Loading index..." << Endl;
        TIndex index(opts.HnswFilename, opts.VectorFilename, opts.Dimension);

        Cerr << "Measuring recall..." << Endl;
        MeasureRecallsAndGetReport(index, queries, groundTruth, opts.SearchNeighborhoodSizes,
                                   opts.DistanceCalcLimits, opts.TopSizes, opts.Multithreading,
                                   opts.NumThreads, opts.ShowDistanceCounts);
        Cerr << "Done." << Endl;
    }
};

template<class TValue>
void DispatchDistance(const TOptions& opts) {
    switch (opts.Distance) {
        case EDistance::L1Distance: {
            TRecallMeasurer<TValue, TDistanceWithCounter<TValue, NHnsw::TL1Distance<TValue>>>::Main(opts);
            break;
        }
        case EDistance::L2SqrDistance: {
            TRecallMeasurer<TValue, TDistanceWithCounter<TValue, NHnsw::TL2SqrDistance<TValue>>>::Main(opts);
            break;
        }
        case EDistance::DotProduct: {
            TRecallMeasurer<TValue, TDistanceWithCounter<TValue, NHnsw::TDotProduct<TValue>>>::Main(opts);
            break;
        }
        default: {
            Y_VERIFY(false, "Incorrect distance type");
            break;
        }
    }
}

void DispatchType(const TOptions& opts) {
    switch (opts.VectorComponentType) {
        case EVectorComponentType::I8: {
            DispatchDistance<i8>(opts);
            break;
        }
        case EVectorComponentType::I32: {
            DispatchDistance<i32>(opts);
            break;
        }
        case EVectorComponentType::Float: {
            DispatchDistance<float>(opts);
            break;
        }
        default: {
            Y_VERIFY(false, "Incorrect vector component type");
            break;
        }
    }
}

int main(int argc, char** argv) {
    TOptions opts(argc, argv);

    NPar::LocalExecutor().RunAdditionalThreads(opts.NumThreads - 1);

    DispatchType(opts);

    return 0;
}
