#pragma once

#include "bert_model.h"

#ifdef USE_GPU
#include <dict/mt/libs/nn/ynmt_backend/gpu/backend.h>
#endif

#include <library/cpp/threading/future/future.h>
#include <library/cpp/time_provider/time_provider.h>

#include <util/generic/array_ref.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>

#include <atomic>
#include <list>


namespace NBertApplier {

    template <typename TFloatType, template <typename THeadFloatType> typename TEncoderHead>
    class TBatchProcessor {
    public:
        using TModelResult = typename TEncoderHead<TFloatType>::TResult;

    private:
        struct TRequestInfo {
            NThreading::TPromise<TModelResult> Promise;
            TInstant Deadline;
        };

        struct TBatch {
            TBertInput Input;
            TVector<TRequestInfo> Requests;

            TBatch(size_t maxBatchSize, size_t maxInputLength)
                : Input(maxBatchSize, maxInputLength)
                , Requests()
            {
                Requests.reserve(maxBatchSize);
            }

            bool Empty() const noexcept {
                return Requests.empty();
            }

            size_t Size() const noexcept {
                return Requests.size();
            }
        };

    private:
        IBertModel<TModelResult>& Model;
        ITimeProvider& TimeProvider;
        TManualEvent RunProcessing;
        TThread ProcessingThread;
        std::atomic_bool Shutdown = false;
        const TDuration Timeout = TDuration::MilliSeconds(5);
        TMutex RequestsLock;
        std::list<TBatch> Batches;
        size_t MaxBatchSize;
        size_t MaxInputLength;

    public:
        //! Constructs the processor
        /** @param model - The model
            @param timeProvider - The time provider
            @param accumulationTimeout - The batch accumulation timeout. Process method will try to accumulate enough data for the batch during the given time period
        **/
        TBatchProcessor(IBertModel<TModelResult>& model, ITimeProvider& timeProvider
                        , TDuration accumulationPeriod = TDuration::MilliSeconds(5));
        ~TBatchProcessor();

        //! Starts batch processing of the data.
        /** Would run synchronously if enough data for the complete batch available and forceAsync is false
            Otherwise will schedule the data for an asynchronous processing
            @param data - The data to process
            @
        */
        NThreading::TFuture<TModelResult> Process(TArrayRef<const int> data, bool forceAsync = false);

        //! Processes the data and all pending data immediately
        /** This could be inefficient since there could be not enough data for the complete batch
            @param data - The data to process
        */
        TModelResult ProcessImmediatly(TArrayRef<const int> data);

        //! Cancels all requests those are not being processed at the moment
        void Cancel();

        //! Returns the maximum batch size
        inline size_t GetMaxBatchSize() const noexcept {
            return MaxBatchSize;
        }

        //! Returns the maximum length of a single input including the mandatory "beginning of stream (BOS)" marker
        inline size_t GetMaxInputLength() const noexcept {
            return MaxInputLength;
        }

    private:
        TBatchProcessor(TBatchProcessor const&) = delete;
        TBatchProcessor& operator=(TBatchProcessor const&) = delete;

        void ProcessingProc();
        void TryProcess(TGuard<decltype(RequestsLock)>&& guard, bool forceAsync);
        void Process(TGuard<decltype(RequestsLock)>&& guard);
        NThreading::TFuture<TModelResult> EnqueueRequest(TArrayRef<const int> data);
        void CreateNewBatch();

    private:
    };

}
