#pragma once

#include <util/thread/pool.h>
#include <util/datetime/base.h>
#include <util/generic/singleton.h>

    namespace NUtil {

        /**
         * @brief Queue process by dynamic size thread pool
         *
         */
        class TSmartMtpQueue: public IThreadPool, public TThreadFactoryHolder {
            class TImpl;
            THolder<TImpl> Impl;
        public:
            struct TOptions {
            public:
                TDuration IdleInterval;
                size_t MinThreadCount;
                bool   DisableBalloc;

            public:
                inline TOptions(size_t minThreadCount = 1, TDuration idleInterval = TDuration::Seconds(1), bool disableBalloc = false)
                    : IdleInterval(idleInterval)
                    , MinThreadCount(minThreadCount)
                    , DisableBalloc(disableBalloc)
                {}

            public:
                static TOptions NoBalloc;
            };

        public:

            /**
             * @brief Create Smart MTP Queue with default thread Pool
             *
             * @param minThreadCount Minimum thread count
             */
            explicit TSmartMtpQueue(const TOptions& options = Default<TOptions>());

            /**
             * @brief Create Smart MTP Queue with specified thread pool
             *
             * @param minThreadCount Minimum thread count
             * @param pool pointer to thread pool
             */
            explicit TSmartMtpQueue(IThreadFactory* pool, const TOptions& options = Default<TOptions>());

            /**
             * @brief Destroy queue
             *
             * @details Destructor wait all processing objects synchronously
             */
            virtual ~TSmartMtpQueue() override;

            /**
             * @brief Add new object to queue
             *
             * @details
             * if queue is not started by Start() object rejected
             * else if there is vacant threads object will be processed immediately
             * else if queue is not full object will be put to queue and process later
             * else if queue is full object rejected (return false)
             * @note  Obj is not deleted after execution
             * @param obj object to be processed
             * @return bool @b true success @b false object rejected will not be processed
             */
            virtual bool Add(IObjectInQueue* obj) override;

            /**
             * @brief Pending objects queue size
             *
             * @return size_t
             */
            virtual size_t Size() const noexcept override;

            /**
             * @brief Start queue processing
             *
             * @param maxThreadCount Maximum thread count
             * @param queueSizeLimit Maximum queue limit, 0 - unlimited
             * @return void
             */
            virtual void Start(size_t maxThreadCount, size_t queueSizeLimit = 0) override;
            /**
             * @brief Stop queue processing
             *
             * @return void
             */
            virtual void Stop() noexcept override;

            /**
             * @brief Set interval for waiting new object
             * @param interval interval
             * @return void
             */
            void SetMaxIdleTime(TDuration interval);

            size_t ThreadCount();

        };

    }
