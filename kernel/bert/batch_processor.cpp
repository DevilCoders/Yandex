#include "batch_processor.h"

#include <library/cpp/float16/float16.h>
#include <library/cpp/threading/cancellation/operation_cancelled_exception.h>

#include <util/generic/xrange.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/system/guard.h>

#include <dict/mt/libs/nn/ynmt/extra/encoder_head.h>

#include <algorithm>
#include <exception>


using namespace NDict::NMT::NYNMT;
using namespace NThreading;

namespace NBertApplier {

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    TBatchProcessor<TFloatType, TEncoderHead>::TBatchProcessor(IBertModel<TModelResult>& model, ITimeProvider& timeProvider
                                                                , TDuration accumulationPeriod)
        : Model(model)
        , TimeProvider(timeProvider)
        , RunProcessing()
        , ProcessingThread([this] { this->ProcessingProc(); })
        , Shutdown(false)
        , Timeout(accumulationPeriod)
        , RequestsLock()
        , Batches()
        , MaxBatchSize(model.GetMaxBatchSize())
        , MaxInputLength(model.GetMaxInputLength())

    {
        CreateNewBatch();
        ProcessingThread.Start();
    }

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    TBatchProcessor<TFloatType, TEncoderHead>::~TBatchProcessor() {
        {
            bool expected = false;
            if (!Shutdown.compare_exchange_strong(expected, true)) {
                return;
            }
            RunProcessing.Signal();
        }
        ProcessingThread.Join();
    }

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    TFuture<typename TBatchProcessor<TFloatType, TEncoderHead>::TModelResult>
            TBatchProcessor<TFloatType, TEncoderHead>::Process(TArrayRef<const int> data, bool forceAsync)
    {
        TGuard g(RequestsLock);
        auto result = EnqueueRequest(data);
        TryProcess(std::move(g), forceAsync);
        return result;
    }

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    typename TBatchProcessor<TFloatType, TEncoderHead>::TModelResult TBatchProcessor<TFloatType, TEncoderHead>::ProcessImmediatly(TArrayRef<const int> data) {
        TGuard g(RequestsLock);
        auto future = EnqueueRequest(data);
        Process(std::move(g));
        return future.ExtractValueSync();
    }

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    void TBatchProcessor<TFloatType, TEncoderHead>::Cancel() {
        TGuard g(RequestsLock);
        if (Batches.front().Empty()) {
            return;
        }

        auto e = std::make_exception_ptr(TOperationCancelledException());
        for (auto& b : Batches) {
            for (auto& r : b.Requests) {
                r.Promise.SetException(e);
            }
        }
        Batches.clear();
        CreateNewBatch();
    }

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    void TBatchProcessor<TFloatType, TEncoderHead>::ProcessingProc() {
        TThread::SetCurrentThreadName("bert_batch");
        Model.PrepareBackend();
        TDuration timeout = Timeout;
        while (!Shutdown) {
            RunProcessing.WaitT(timeout);
            RunProcessing.Reset();
            while (!Shutdown) {
                TGuard g(RequestsLock);
                Y_VERIFY(!Batches.empty());
                if (Batches.front().Empty()) {
                    timeout = Timeout;
                    break;
                }

                auto const& front = Batches.front();
                auto const now = TimeProvider.Now();
                if (front.Size() < GetMaxBatchSize() && front.Requests.front().Deadline > now) {
                    timeout = front.Requests.front().Deadline - now;
                    break;
                }
                Process(std::move(g));
            }
        }
    }

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    void TBatchProcessor<TFloatType, TEncoderHead>::TryProcess(TGuard<decltype(RequestsLock)>&& guard, bool forceAsync) {
        // The guard should be acquired
        auto const& batch = Batches.front();
        if (batch.Size() < GetMaxBatchSize() && batch.Requests.front().Deadline > TimeProvider.Now()) {
            return;
        }
        if (!forceAsync) {
            Process(std::move(guard));
        } else {
            guard.Release();
            RunProcessing.Signal();
        }
    }

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    void TBatchProcessor<TFloatType, TEncoderHead>::Process(TGuard<decltype(RequestsLock)>&& guard) {
        // The guard should be acquired
        auto const now = TimeProvider.Now();
        std::list<TBatch> batches;
        auto const sliceEnd = std::find_if(std::begin(Batches), std::end(Batches), [now, this](TBatch const& b)
                                            {
                                                return b.Empty() || (b.Size() < GetMaxBatchSize() && b.Requests.front().Deadline > now);
                                            });
        batches.splice(std::end(batches), Batches, std::begin(Batches), sliceEnd);
        if (Batches.empty()) {
            CreateNewBatch();
        }

        guard.Release();

        for (auto& batch : batches) {
            // process data
            auto const states = Model.ProcessBatch(batch.Input);

            // write results
            if (states.size() % batch.Size() != 0) {
                auto const e = std::make_exception_ptr(yexception() << "Model has returned an invalid processing result " << __FILE__ << ":" << __LINE__);
                for (auto& r : batch.Requests) {
                    r.Promise.SetException(e);
                }
                continue;
            }

            size_t const embeddingSize = states.size() / batch.Size();
            auto begin = states.begin();
            for (auto& r : batch.Requests) {
                auto const end = begin + embeddingSize;
                TModelResult result;
                result.assign(begin, end);
                r.Promise.SetValue(std::move(result));
                begin = end;
            }
        }
    }

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    TFuture<typename TBatchProcessor<TFloatType, TEncoderHead>::TModelResult>
            TBatchProcessor<TFloatType, TEncoderHead>::EnqueueRequest(TArrayRef<int const> data)
    {
        // Should be called under the RequestsLock
        if (Batches.back().Size() == GetMaxBatchSize()) {
            CreateNewBatch();
        }
        auto const deadline = TimeProvider.Now() + Timeout;
        auto promise = NewPromise<TModelResult>();
        auto result = promise.GetFuture();
        auto& batch = Batches.back();
        batch.Requests.push_back({std::move(promise), deadline});
        batch.Input.Add(data);
        return result;
    }

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    void TBatchProcessor<TFloatType, TEncoderHead>::CreateNewBatch() {
        // Should be called under the RequestsLock (unless called from the .ctor)
        Batches.emplace_back(GetMaxBatchSize(), GetMaxInputLength());
    }

    template class TBatchProcessor<float, NDict::NMT::NYNMT::TCopyHead>;
    template class TBatchProcessor<TFloat16, NDict::NMT::NYNMT::TCopyHead>;
    template class TBatchProcessor<float, NDict::NMT::NYNMT::TClassificationHead>;
    template class TBatchProcessor<TFloat16, NDict::NMT::NYNMT::TClassificationHead>;
    template class TBatchProcessor<float, NDict::NMT::NYNMT::TRegressionHead>;
    template class TBatchProcessor<TFloat16, NDict::NMT::NYNMT::TRegressionHead>;
    template class TBatchProcessor<float, NDict::NMT::NYNMT::TMultitargetHead>;
    template class TBatchProcessor<TFloat16, NDict::NMT::NYNMT::TMultitargetHead>;
#ifdef USE_GPU
    template class TGPUProcessor<float>;
    template class TGPUProcessor<TFloat16>;
#endif
}
