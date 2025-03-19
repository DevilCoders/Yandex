#include "parallel_mx_calcer.h"

#include <kernel/bundle/bundle.h>
#include <kernel/matrixnet/mn_sse.h>
#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/containers/2d_array/2d_array.h>
#include <library/cpp/threading/async_task_batch/async_task_batch.h>
#include <util/generic/xrange.h>

using namespace NMatrixnet;

namespace {

    struct TMnSseTreeRange {
        size_t TreeRangeStart = 0;
        size_t TreeRangeFinish = Max();
        TMnSseTreeRange() = default;
        TMnSseTreeRange(size_t start, size_t finish)
            : TreeRangeStart(start)
            , TreeRangeFinish(finish)
        {
            Y_ENSURE(TreeRangeStart <= TreeRangeFinish);
        }

        operator bool() const {
            return !(TreeRangeStart == 0 && TreeRangeFinish == Max<size_t>());
        }
    };

    struct TCalcRelevItem {
        const NMatrixnet::IRelevCalcer* MxNet = nullptr;
        TMnSseTreeRange MnSseTreeRange;
        const TFactorStorage* const* Factors = nullptr;
        double* Relevs = nullptr;
        size_t NumDocs = 0;
    public:
        TCalcRelevItem() = default;

        TCalcRelevItem(const NMatrixnet::IRelevCalcer* mxNet, const TFactorStorage* const* factors, double* relevs, size_t numDocs)
            : MxNet(mxNet)
            , Factors(factors)
            , Relevs(relevs)
            , NumDocs(numDocs)
        {
        }

        TCalcRelevItem(const NMatrixnet::IRelevCalcer* mxNet, const TVector<const TFactorStorage*>& factors, TVector<double>& relevs)
            : TCalcRelevItem(mxNet, factors.data(), relevs.data(), factors.size())
        {
        }

        void Calculate() {
            if (MnSseTreeRange) {
                MxNet->DoSlicedCalcRelevs(Factors, Relevs, NumDocs, MnSseTreeRange.TreeRangeStart, MnSseTreeRange.TreeRangeFinish);
            } else if (MxNet) {
                MxNet->DoSlicedCalcRelevs(Factors, Relevs, NumDocs);
            }
        }
    };

    class TBaseCalcRelevTask
        : public TAsyncTaskBatch::ITask
    {
    public:
        explicit TBaseCalcRelevTask(const TCalcRelevItem& item)
            : Item(item)
        {
        }

        void Process() override {
            Item.Calculate();
        }
    public:
        TCalcRelevItem Item;
    };

    class TRenormedCalcRelevTask
        : public TBaseCalcRelevTask
    {
    public:
        explicit TRenormedCalcRelevTask(const TCalcRelevItem& item, NMatrixnet::TBundleRenorm renorm)
            : TBaseCalcRelevTask(item)
            , Renorm_(renorm)
        {
        }
        void Process() override {
            TBaseCalcRelevTask::Process();
            for (size_t i = 0; i < Item.NumDocs; ++i) {
                Item.Relevs[i] = Renorm_.Apply(Item.Relevs[i]);
            }
        }
    private:
        const NMatrixnet::TBundleRenorm Renorm_;
    };

    class TCalcRelevTaskBatch
        : public TAsyncTaskBatch
    {
    public:
        TCalcRelevTaskBatch(const TParallelMtpOptions& opts)
            : TAsyncTaskBatch(opts.Queue)
            , Opts_(opts)
        {
        }

        TVector<TCalcRelevItem> MakeCalcItems(const NMatrixnet::IRelevCalcer* mxNet,
            const TFactorStorage* const* factors,
            double* relevs,
            size_t numDocs,
            TMnSseTreeRange range = {}
        ) {
            TVector<TCalcRelevItem> res;

            if (Opts_.MaxDocCount == 0) {
                res.push_back(TCalcRelevItem(mxNet, factors, relevs, numDocs));
                if (range) {
                    res.back().MnSseTreeRange = range;
                }
                return res;
            }

            size_t i = 0;
            while (i < numDocs) {
                const size_t taskBegin = i;
                TCalcRelevItem item;
                item.MxNet = mxNet;
                item.Factors = &factors[taskBegin];
                item.Relevs = &relevs[taskBegin];

                i += Opts_.MaxDocCount;
                item.NumDocs = Min(numDocs, i) - taskBegin;

                if (range) {
                    item.MnSseTreeRange = range;
                }
                res.push_back(item);
            }

            return res;
        }

        TVector<TCalcRelevItem> MakeCalcItems(const NMatrixnet::IRelevCalcer* mxNet,
            const TVector<const TFactorStorage*>& factors,
            TVector<double>& relevs,
            TMnSseTreeRange range = {}
        ) {
            Y_VERIFY(factors.size() == relevs.size());
            return MakeCalcItems(mxNet, factors.data(), relevs.data(), factors.size(), range);
        }
    private:
        TParallelMtpOptions Opts_;
    };

    void ParallelCalc(const NMatrixnet::IRelevCalcer* mxNet,
        const TVector<const TFactorStorage*>& factors,
        TVector<double>& relevs,
        const TParallelMtpOptions& opts,
        TAsyncTaskStats* asyncTaskStats
    ) {
        TCalcRelevTaskBatch batch(opts);
        TAsyncTaskBatch::TWaitInfo waitInfo;
        if (!TDynamicBundle::IsDynamicBundle(*mxNet)) {
            for (const auto& item : batch.MakeCalcItems(mxNet, factors, relevs)) {
                batch.Add(new TBaseCalcRelevTask(item));
            }
            waitInfo = batch.WaitAllAndProcessNotStarted();
        } else {
            const NMatrixnet::TMnSseInfo* info = dynamic_cast<const NMatrixnet::TMnSseInfo*>(mxNet);
            const auto& staticInfo = info->GetSseDataPtrs().Meta;
            TArray2D<double> relevParts(relevs.size(), staticInfo.DynamicBundle.size());
            double* relevsRow = &relevParts[0][0];
            for (size_t componentInd = 0; componentInd < staticInfo.DynamicBundle.size(); ++componentInd) {
                const auto& component = staticInfo.DynamicBundle[componentInd];
                for (const auto& item : batch.MakeCalcItems(
                    mxNet,
                    factors.data(),
                    relevsRow,
                    factors.size(),
                    { component.TreeIndexFrom, component.TreeIndexTo })
                ) {
                    batch.Add(new TBaseCalcRelevTask(item));
                }
                relevsRow += factors.size();
            }
            waitInfo = batch.WaitAllAndProcessNotStarted();
            std::fill(relevs.begin(), relevs.end(), 0);
            for (size_t i = 0; i < relevParts.GetYSize(); ++i) {
                const auto& component = staticInfo.DynamicBundle[i];
                for (size_t j = 0; j < relevParts.GetXSize(); ++j) {
                    bool existsWeight = factors[j]->Ptr(component.FeatureIndex);
                    Y_ASSERT(existsWeight);
                    relevs[j] += (
                        (relevParts[i][j] * component.Scale + component.Bias) *
                        (existsWeight ? (*factors[j])[component.FeatureIndex] : 1.0)
                    );
                }
            }
        }
        if (asyncTaskStats) {
            *asyncTaskStats = waitInfo.GetStats();
        }
    }

    void ParallelCalc(const NMatrixnet::TRankModelVector& weightedModels,
        const TVector<const TFactorStorage*>& factors,
        TVector<TVector<double>>& relevs,
        const TParallelMtpOptions& opts,
        TAsyncTaskStats* asyncTaskStats
    ) {
        TCalcRelevTaskBatch batch(opts);

        TVector<TVector<TVector<double>>> relevParts(relevs.size());

        for (size_t i = 0; i < weightedModels.size(); ++i) {
            const size_t numTrees = weightedModels[i].Matrixnet->NumTrees();
            const size_t numTreesPerParallelMnSseTask = opts.NumTreesPerParallelMnSseTask == 0 ? numTrees : opts.NumTreesPerParallelMnSseTask;
            const size_t numRanges = numTrees / numTreesPerParallelMnSseTask + !!(numTrees % numTreesPerParallelMnSseTask);
            relevParts[i].assign(numRanges, TVector<double>(relevs[i].size()));

            size_t rangeBegin = 0;
            size_t j = 0;
            while (rangeBegin < numTrees) {
                for (const auto& item : batch.MakeCalcItems(
                    weightedModels[i].Matrixnet.Get(),
                    factors,
                    relevParts[i][j],
                    { rangeBegin, std::min<size_t>(rangeBegin + numTreesPerParallelMnSseTask, numTrees) })
                ) {
                    batch.Add(new TBaseCalcRelevTask(item));
                }
                ++j;
                rangeBegin += numTreesPerParallelMnSseTask;
            }
        }
        auto waitInfo = batch.WaitAllAndProcessNotStarted();
        if (asyncTaskStats) {
            *asyncTaskStats = waitInfo.GetStats();
        }

        for (size_t i = 0; i < relevParts.size(); ++i) {
            for (size_t j = 0; j < relevParts[i].size(); ++j) {
                for (size_t k = 0; k < relevParts[i][j].size(); ++k) {
                    relevs[i][k] += relevParts[i][j][k];
                }
            }
        }

        for (size_t i = 0; i < relevs.size(); ++i) {
            for (size_t j = 0; j < relevs[i].size(); ++j) {
                relevs[i][j] = weightedModels[i].Renorm.Apply(relevs[i][j]);
            }
        }
    }
}

void NMatrixnet::CalcRelevs(const NMatrixnet::IRelevCalcer* mxNet, const TVector<const TFactorStorage*>& factors, TVector<double>& relevs, const TParallelMtpOptions* mtpOptions, TAsyncTaskStats* asyncTaskStats) {
    relevs.resize(factors.size());

    if (mtpOptions && mtpOptions->Queue && mtpOptions->MaxDocCount > 0 && factors.size() > mtpOptions->MaxDocCount) {
        ParallelCalc(mxNet, factors, relevs, *mtpOptions, asyncTaskStats);
    } else {
        TMaybe<TPrecisionTimer> timer;
        if (asyncTaskStats) {
            timer.ConstructInPlace();
        }
        TCalcRelevItem item(mxNet, factors, relevs);
        TBaseCalcRelevTask(item).Process();
        if (asyncTaskStats) {
            asyncTaskStats->TotalTime = CyclesToDuration(timer->GetCycleCount());
            asyncTaskStats->SelfTime = CyclesToDuration(timer->GetCycleCount());
        }
    }
}

void NMatrixnet::CalcRelevs(const NMatrixnet::TRankModelVector& weightedModels, const TVector<const TFactorStorage*>& factors, TVector<TVector<double>>& relevs, const TParallelMtpOptions* mtpOptions, TAsyncTaskStats* asyncTaskStats) {
    relevs.resize(weightedModels.size());

    for (TVector<double>& r : relevs) {
        r.resize(factors.size());
    }

    if (mtpOptions && mtpOptions->Queue) {
        ParallelCalc(weightedModels, factors, relevs, *mtpOptions, asyncTaskStats);
    } else {
        TMaybe<TPrecisionTimer> timer;
        if (asyncTaskStats) {
            timer.ConstructInPlace();
        }
        for (size_t i = 0; i < weightedModels.size(); ++i) {
            TCalcRelevItem item(weightedModels[i].Matrixnet.Get(), factors, relevs[i]);
            TRenormedCalcRelevTask(item, weightedModels[i].Renorm).Process();
        }
        if (asyncTaskStats) {
            asyncTaskStats->TotalTime = CyclesToDuration(timer->GetCycleCount());
            asyncTaskStats->SelfTime = CyclesToDuration(timer->GetCycleCount());
        }
    }
}

void NMatrixnet::CalcRelevs(const IRelevCalcer* mxNet, const TVector<const TFactorStorage*>& factors, TVector<double>& values, TAsyncTaskBatch& q, size_t docsInBatch) {
    if (docsInBatch == 0) {
        docsInBatch = factors.size();
    }
    values.resize(factors.size());
    for(size_t left : xrange<size_t>(0, factors.size(), docsInBatch)) {
        auto featuresStart = factors.begin() + left;
        double* dstStart = values.begin() + left;
        size_t batchSize = Min(docsInBatch, factors.size() - left);

        q.Add([featuresStart, dstStart, batchSize, mxNet] () {
            mxNet->DoSlicedCalcRelevs(featuresStart, dstStart, batchSize);
        });
    }
}

struct TTreeSplitItem {
    size_t RangeBegin = 0;
    size_t RangeFinish = 0;
    double Bias = 0;
    double Scale = 1;
    TMaybe<NFactorSlices::TFullFactorIndex> WeightIndex = Nothing();
};

static TVector<TTreeSplitItem> GetSplitItems(const NMatrixnet::TMnSseInfo& model, size_t treesPerTask) {
    if (treesPerTask == 0) {
        treesPerTask = model.NumTrees() + 1;
    }
    TVector<TTreeSplitItem> res;
    if (auto bundle = model.GetSseDataPtrs().Meta.DynamicBundle; bundle) {
        for (const auto& component : bundle) {
            for (size_t ind = component.TreeIndexFrom; ind < component.TreeIndexTo; ind += treesPerTask) {
                size_t to = Min(ind + treesPerTask, component.TreeIndexTo);
                res.push_back({ ind, to, component.Bias * (to - ind) / (component.TreeIndexTo - component.TreeIndexFrom), component.Scale, component.FeatureIndex });
            }
        }
    } else {
        size_t numTrees = model.NumTrees();
        for (size_t ind = 0; ind < numTrees; ind += treesPerTask) {
            res.push_back({ ind, Min(ind + treesPerTask, numTrees), 0, 1 });
        }
    }
    return res;
}

template<typename TFunc>
struct TCalcBinarizationTask : public TAsyncTaskBatch::ITask {
    TCalcBinarizationTask(
        NMatrixnet::TMnSseInfo::TSlicedPreparedBatch& preparedBatch,
        const TFactorStorage* const* factors,
        size_t numDocs,
        const NMatrixnet::TMnSseInfo& model,
        TFunc afterProcess
    )
        : PreparedBatch(preparedBatch)
        , Factors(factors)
        , NumDocs(numDocs)
        , Model(model)
        , AfterProcess(afterProcess)
    {}

    void Process() override {
        PreparedBatch = Model.DoSlicedCalcBinarization(Factors, NumDocs);
        AfterProcess();
    }

    NMatrixnet::TMnSseInfo::TSlicedPreparedBatch& PreparedBatch;
    const TFactorStorage* const* Factors = nullptr;
    size_t NumDocs = 0;
    const NMatrixnet::TMnSseInfo& Model;
    TFunc AfterProcess;
};

struct TCalcRelevsTask : public TAsyncTaskBatch::ITask {
    TCalcRelevsTask(
        double* resPtr,
        size_t docCount,
        const NMatrixnet::TMnSseInfo& model,
        const NMatrixnet::TMnSseInfo::TSlicedPreparedBatch& batch,
        const TTreeSplitItem& splitItem
    )
        : ResPtr(resPtr)
        , DocCount(docCount)
        , Model(model)
        , Batch(batch)
        , SplitItem(splitItem)
    {}

    void Process() override {
        Model.DoCalcRelevs(*Batch.PreparedBatch, ResPtr, SplitItem.RangeBegin, SplitItem.RangeFinish);
        for (size_t i : xrange(DocCount)) {
            ResPtr[i] = ResPtr[i] * SplitItem.Scale + SplitItem.Bias;
            if (SplitItem.WeightIndex) {
                const float* ptr = Batch.Features[i]->Ptr(*SplitItem.WeightIndex);
                ResPtr[i] *= (ptr ? *ptr : 1.0);
            }
        }
    }

    double* ResPtr = nullptr;
    size_t DocCount = 0;
    const NMatrixnet::TMnSseInfo& Model;
    const NMatrixnet::TMnSseInfo::TSlicedPreparedBatch& Batch;
    const TTreeSplitItem& SplitItem;
};

class TSharedBinarizationContext::TImpl {
public:
    TImpl(
        const NMatrixnet::TMnSseInfo& model,
        const TVector<const TFactorStorage*>& factors,
        size_t maxDocsPerBatch,
        TVector<double>& predictsDst,
        TAsyncTaskBatch& binarizationTaskBatch,
        TAsyncTaskBatch& mainTaskBatch,
        size_t treesPerTask
    )
        : Model(model)
        , Factors(factors)
        , MaxDocsPerBatch(maxDocsPerBatch)
        , PredictsDst(predictsDst)
        , BinarizationTaskBatch(binarizationTaskBatch)
        , MainTaskBatch(mainTaskBatch)
        , TreesPerTask(treesPerTask)
        , SplitItems(GetSplitItems(Model, TreesPerTask))
        , RelevParts(factors.size(), SplitItems.size())
    {}

    // TImpl(TImpl&&) = default;
    // TImpl& operator=(TImpl&&) = default;

    void StartBinarization() {
        if (Factors.size() == 0) {
            return;
        }
        if (MaxDocsPerBatch == 0) {
            PreparedBatches.push_back({});
            BinarizationTaskBatch.Add(new TCalcBinarizationTask(
                PreparedBatches.back(),
                Factors.data(),
                Factors.size(),
                Model,
                [&]() {
                    StartCalc();
                }
            ));
        } else {
            AtomicSet(BinarizationTaskCnt, Factors.size() / MaxDocsPerBatch + !!(Factors.size() % MaxDocsPerBatch));
            PreparedBatches.reserve(Factors.size() / MaxDocsPerBatch + !!(Factors.size() % MaxDocsPerBatch));
            for (size_t i = 0; i < Factors.size(); i += MaxDocsPerBatch) {
                size_t last = Min(i + MaxDocsPerBatch, Factors.size());
                PreparedBatches.push_back({});
                BinarizationTaskBatch.Add(new TCalcBinarizationTask(
                    PreparedBatches.back(),
                    Factors.data() + i,
                    last - i,
                    Model,
                    [&]() {
                        if (AtomicDecrement(BinarizationTaskCnt) == 0) {
                            StartCalc();
                        }
                    }
                ));
            }
        }
    }

    void WaitBinarizationAndStartCalc(TAsyncTaskStats* asyncTaskStats) {
        auto info = BinarizationTaskBatch.WaitAllAndProcessNotStarted();
        if (asyncTaskStats) {
            *asyncTaskStats = info.GetStats();
        }
    }

    void WriteToPredictsDst() {
        std::fill(PredictsDst.begin(), PredictsDst.end(), 0);
        for (size_t i : xrange(RelevParts.GetYSize())) {
            for (size_t j : xrange(RelevParts.GetXSize())) {
                PredictsDst[j] += RelevParts[i][j];
            }
        }
    }

private:
    void StartCalc() {
        double* relevPtr = &RelevParts[0][0];
        for (const auto& splitItem : SplitItems) {
            for (const auto& batch : PreparedBatches) {
                size_t docCount = batch.GetDocCount();
                MainTaskBatch.Add(new TCalcRelevsTask(
                    relevPtr,
                    docCount,
                    Model,
                    batch,
                    splitItem
                ));
                relevPtr += docCount;
            }
        }
    }

    const NMatrixnet::TMnSseInfo& Model;
    const TVector<const TFactorStorage*>& Factors;
    size_t MaxDocsPerBatch = 0;
    TVector<double>& PredictsDst;
    TAsyncTaskBatch& BinarizationTaskBatch;
    TAsyncTaskBatch& MainTaskBatch;
    TVector<NMatrixnet::TMnSseInfo::TSlicedPreparedBatch> PreparedBatches;
    size_t TreesPerTask = 0;
    TVector<TTreeSplitItem> SplitItems;
    TArray2D<double> RelevParts;
    TAtomic BinarizationTaskCnt = 0;
};

void TSharedBinarizationContext::TImplDeleter::Destroy(TSharedBinarizationContext::TImpl* obj) {
    delete obj;
}

TSharedBinarizationContext::TSharedBinarizationContext(
    const NMatrixnet::TMnSseInfo& model,
    const TVector<const TFactorStorage*>& factors,
    size_t maxDocsPerBatch,
    TVector<double>& predictsDst,
    TAsyncTaskBatch& binarizationTaskBatch,
    TAsyncTaskBatch& mainTaskBatch,
    size_t treesPerBatch
)
    : Impl(new TImpl(
        model, factors, maxDocsPerBatch,
        predictsDst, binarizationTaskBatch, mainTaskBatch, treesPerBatch
    ))
{}

void TSharedBinarizationContext::StartBinarization() {
    Impl->StartBinarization();
}

void TSharedBinarizationContext::WaitBinarizationAndStartCalc(TAsyncTaskStats* asyncTaskStats) {
    Impl->WaitBinarizationAndStartCalc(asyncTaskStats);
}

void TSharedBinarizationContext::WriteToPredictsDst() {
    Impl->WriteToPredictsDst();
}

TSharedBinarizationContext::~TSharedBinarizationContext() {}
