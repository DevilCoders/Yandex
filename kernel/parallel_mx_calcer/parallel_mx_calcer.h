#pragma once

#include <util/generic/fwd.h>
#include <util/generic/ptr.h>

class TFactorStorage;
class IThreadPool;
class TAsyncTaskBatch;
struct TAsyncTaskStats;

namespace NMatrixnet {
    template <class T>
    struct TBundleElement;

    class IRelevCalcer;
    class TMnSseInfo;

    struct TParallelMtpOptions {
        IThreadPool* Queue = nullptr;
        size_t MaxDocCount = 0;
        size_t NumTreesPerParallelMnSseTask = 0;
    };

    class TSharedBinarizationContext {
    public:
        TSharedBinarizationContext(
            const NMatrixnet::TMnSseInfo& model,
            const TVector<const TFactorStorage*>& factors,
            size_t maxDocsPerBatch,
            TVector<double>& predictsDst,
            TAsyncTaskBatch& binarizationTaskBatch,
            TAsyncTaskBatch& mainTaskBatch,
            size_t treesPerTask
        );

        TSharedBinarizationContext(TSharedBinarizationContext&&) = default;
        TSharedBinarizationContext& operator=(TSharedBinarizationContext&&) = default;

        void StartBinarization();
        void WaitBinarizationAndStartCalc(TAsyncTaskStats* asyncTaskStats = nullptr);
        void WriteToPredictsDst();

        ~TSharedBinarizationContext();
    private:
        class TImpl;
        struct TImplDeleter {
            static void Destroy(TImpl* obj);
        };

        THolder<TImpl, TImplDeleter> Impl;
    };

    void CalcRelevs(const IRelevCalcer* mxNet, const TVector<const TFactorStorage*>& factors, TVector<double>& values, const TParallelMtpOptions* mtpOptions, TAsyncTaskStats* asyncTaskStats = nullptr);
    void CalcRelevs(const TVector<TBundleElement<TAtomicSharedPtr<const TMnSseInfo>>>& weightedModels, const TVector<const TFactorStorage*>& factors, TVector<TVector<double>>& relevs, const TParallelMtpOptions* mtpOptions, TAsyncTaskStats* asyncTaskStats = nullptr);
    void CalcRelevs(const IRelevCalcer* mxNet, const TVector<const TFactorStorage*>& factors, TVector<double>& values, TAsyncTaskBatch& q, size_t docsInBatch);
}

