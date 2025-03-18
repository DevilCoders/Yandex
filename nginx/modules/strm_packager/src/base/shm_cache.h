#pragma once

#include <nginx/modules/strm_packager/src/base/shm_zone.h>
#include <nginx/modules/strm_packager/src/base/timer.h>
#include <nginx/modules/strm_packager/src/base/workers.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/buffer.h>
#include <util/generic/yexception.h>

namespace NStrm::NPackager {
    class TRequestWorker;

    class TShmCache {
    public:
        template <typename TData>
        using TDataGetter = std::function<NThreading::TFuture<TData>()>;

        template <typename TData>
        using TDataLoader = std::function<TData(void const* buffer, size_t bufferSize)>;

        template <typename TData, typename TBytes>
        using TDataSaver = std::function<TBytes(const TData& data)>;

        struct TSettings {
            ngx_msec_t ReadyTimeout;      // how long ready element live
            ngx_msec_t InProgressTimeout; // how long in-progress element live
            ngx_msec_t MaxWaitTimeout;    // max duration for timer wait
        };

        TShmCache() = default;

        NThreading::TFuture<TBuffer> Get(
            TRequestWorker& request,
            const TString& key,
            TDataGetter<TBuffer> runGetter);

        // TBytes must have .Data() and .Size() methods
        template <typename TData, typename TMidData, typename TBytes>
        NThreading::TFuture<TData> Get(
            TRequestWorker& request,
            const TString& key,
            TDataGetter<TMidData> runGetter,
            TDataLoader<TData> loader,
            TDataSaver<TMidData, TBytes> saver);

        // must be called before init
        void SetSettings(const TSettings&);

    private:
        friend class TShmZone<TShmCache>;
        using TZone = TShmZone<TShmCache>;

        void Init(TZone& zone);
        void InitWithExistingShmState(TZone& zone);

        struct TElement {
            ngx_rbtree_node_t RBTreeNode; // inside TState::RBTree
            ngx_queue_t QMainNode;        // inside TState::QReady if DataSize > 0 otherwise inside TState::QInProgress

            ngx_msec_t CreationTime;

            ui32 KeyLength;
            ui32 DataSize;

            // Pid of the process which has created the cache element
            ngx_pid_t Pid;
            // Pointer to the request in case it is in-progress, else nullptr
            TRequestWorker* RequestPtr;

            // next KeyLength bytes is key
            // next DataSize bytes is data (if DataSize > 0)

            ui8* KeyPtr() {
                return ((ui8*)this) + sizeof(TElement);
            }

            ui8* DataPtr() {
                return ((ui8*)this) + sizeof(TElement) + KeyLength;
            }
        };

        static inline ui32 Crc32(const TString& str) {
            return ngx_crc32_short((ui8*)str.data(), str.length());
        }

        static int RBTCompare(ui32 aCrc32, ui32 aKeyLength, ui8* aKeyPtr, ui32 bCrc32, ui32 bKeyLength, ui8* bKeyPtr);

        static int RBTCompare(TElement& a, TElement& b) {
            if (a.RBTreeNode.key < b.RBTreeNode.key) {
                return -1;
            } else if (a.RBTreeNode.key > b.RBTreeNode.key) {
                return 1;
            } else {
                return ngx_memn2cmp(a.KeyPtr(), b.KeyPtr(), a.KeyLength, b.KeyLength);
            }
        }

        template <typename T, T TElement::*Field>
        static TElement* Field2Elt(T* fieldPtr) {
            return (TElement*)(((ui8*)fieldPtr) - (size_t) & (((TElement*)nullptr)->*Field));
        }

        struct TState {
            ngx_rbtree_t RBTree;
            ngx_rbtree_node_t RBTreeSentinel;
            ngx_queue_t QReady;       // elements with ready data, last - the most resently added elements
            ngx_queue_t QInProgress;  // elements without data, last - the most resently added elements
            ngx_queue_t QAccessOrder; // last - the most recently accessed element

            TSettings Settings;
        };

        void RemoveElement(TElement* element);
        void RemoveExpiredElements();
        bool RemoveOneOldestElement();
        TElement* FindElement(const ui32 crc32, const TString& key);

        void AddElement(
            const ui32 crc32,
            const TString& key,
            ui8 const* const dataBegin,
            ui8 const* const dataEnd,
            TRequestWorker* requestPtr = nullptr);

        template <typename TData, typename TMidData, typename TBytes>
        void GetImpl(
            TRequestWorker& request,
            TNgxTimer* timer,
            NThreading::TPromise<TData> promise,
            const ui32 crc32,
            const TString& key,
            const ngx_msec_t getStartTime,
            TDataGetter<TMidData> runGetter,
            TDataLoader<TData> loader,
            TDataSaver<TMidData, TBytes> saver);

        // replacement of ngx_rbtree_insert_value, required for full key comparison
        static void RBTreeInsertValue(ngx_rbtree_node_t* temp, ngx_rbtree_node_t* node, ngx_rbtree_node_t* sentinel);

        TMaybe<TZone> Zone;
        TMaybe<TSettings> ToInitSettings;

        class TInProgressElementCleaner {
        public:
            TInProgressElementCleaner(TShmCache& cache, TRequestWorker* requestPtr, ui32 crc32, TString key);

            // One-shot
            // Triggers in-progress element cleanup immediately
            void Trigger();

            // Triggers cleanup if it was not performed before
            ~TInProgressElementCleaner();

        private:
            TShmCache& Cache;
            TRequestWorker* RequestPtr;
            ui32 Crc32;
            TString Key;
            bool Triggered;
        };

        friend TInProgressElementCleaner;
    };

    // ======= implementation

    template <typename TData, typename TMidData, typename TBytes>
    inline NThreading::TFuture<TData> TShmCache::Get(
        TRequestWorker& request,
        const TString& key,
        TDataGetter<TMidData> runGetter,
        TDataLoader<TData> loader,
        TDataSaver<TMidData, TBytes> saver) {
        Y_ENSURE(key.length() > 0);
        Y_ENSURE(runGetter && loader && saver);
        Y_ENSURE(Zone);

        const ui32 crc32 = Crc32(key);

        NThreading::TPromise<TData> promise = NThreading::NewPromise<TData>();

        GetImpl<TData, TMidData, TBytes>(request, /*timer = */ nullptr, promise, crc32, key, ngx_current_msec, runGetter, loader, saver);

        return promise;
    }

    template <typename TData, typename TMidData, typename TBytes>
    inline void TShmCache::GetImpl(
        TRequestWorker& request,
        TNgxTimer* timer,
        NThreading::TPromise<TData> promise,
        const ui32 crc32,
        const TString& key,
        const ngx_msec_t getStartTime,
        TDataGetter<TMidData> runGetter,
        TDataLoader<TData> loader,
        TDataSaver<TMidData, TBytes> saver) try {
        bool run;
        ngx_msec_t timerTime;
        TInProgressElementCleaner* cleaner = nullptr;

        {
            const TShmMutex shmMutex = Zone->GetMutex();
            TGuard<TShmMutex> shmGuard(shmMutex);

            RemoveExpiredElements();

            TElement* element = FindElement(crc32, key);

            if (!element) {
                // put in-progress element, create cleaner in pool, then run getter and subscribe on getter future
                AddElement(crc32, key, /*dataBegin = */ nullptr, /*dataEnd = */ nullptr, &request);
                shmGuard.Release();
                cleaner = request.template GetPoolUtil<TInProgressElementCleaner>().template New(*this, &request, crc32, key);
                run = true;
            } else if (element->DataSize > 0) {
                // just return data
                TData data(loader(element->DataPtr(), element->DataSize));
                shmGuard.Release();
                promise.SetValue(std::move(data));
                return;
            } else {
                const TState& state = Zone->GetShmState();

                if (ngx_msec_int_t(ngx_current_msec - getStartTime) >= (ngx_msec_int_t)state.Settings.InProgressTimeout) {
                    // dont wait more than one InProgressTimeout
                    run = true;
                } else {
                    // other getter is working, setup timer to wait it's result
                    const ngx_msec_t expireTime = element->CreationTime + state.Settings.InProgressTimeout;
                    timerTime = Min<ngx_msec_t>(expireTime - ngx_current_msec, Zone->GetShmState().Settings.MaxWaitTimeout);
                    run = false;
                }
            }
        }

        if (run) {
            // run getter and subscribe on its future
            // possible exception is caught here to trigger cleaner immediately
            NThreading::TFuture<TMidData> future;
            try {
                future = runGetter();
            } catch (const TFatalExceptionContainer&) {
                cleaner ? cleaner->Trigger() : void();
                throw;
            } catch (...) {
                cleaner ? cleaner->Trigger() : void();
                promise.SetException(std::current_exception());
                return;
            }

            future.Subscribe(request.MakeFatalOnException([this, crc32, key, promise, loader, saver, cleaner](const NThreading::TFuture<TMidData>& future) mutable {
                try {
                    const TMidData& data = future.GetValue();
                    const TBytes bytes = saver(data);

                    ui8 const* bytesBegin = (ui8 const*)bytes.Data();
                    ui8 const* bytesEnd = (ui8 const*)(bytes.Data() + bytes.Size());

                    {
                        TShmMutex shmMutex = Zone->GetMutex();
                        const TGuard<TShmMutex> shmGuard(shmMutex);

                        RemoveExpiredElements();
                        TElement* element = FindElement(crc32, key);
                        if (element && element->DataSize > 0) {
                            // some getter finished before this one
                            // no need to copy our data in shm
                        } else {
                            if (element && element->DataSize == 0) {
                                // remove in-progress element
                                RemoveElement(element);
                            }
                            // add our data
                            AddElement(crc32, key, bytesBegin, bytesEnd);
                        }
                    }

                    promise.SetValue(loader(bytesBegin, bytesEnd - bytesBegin));
                } catch (const TFatalExceptionContainer&) {
                    cleaner ? cleaner->Trigger() : void();
                    throw;
                } catch (...) {
                    cleaner ? cleaner->Trigger() : void();
                    promise.SetException(std::current_exception());
                }
            }));
        } else {
            // setup timer
            if (!timer) {
                timer = request.MakeTimer();
                timer->ResetCallback(request.MakeIndependentCallback(
                    [this, crc32, key, promise, &request, timer, getStartTime, runGetter, loader, saver]() {
                        GetImpl(request, timer, promise, crc32, key, getStartTime, runGetter, loader, saver);
                    }));
            }

            timer->ResetTime(timerTime);
        }
    } catch (const TFatalExceptionContainer&) {
        throw;
    } catch (...) {
        promise.SetException(std::current_exception());
    }

    inline void TShmCache::AddElement(const ui32 crc32, const TString& key, ui8 const* const dataBegin, ui8 const* const dataEnd, TRequestWorker* requestPtr) {
        TState& state = Zone->GetShmState();
        TElement* element = nullptr;

        const size_t dataSize = dataEnd - dataBegin;

        for (int rmcnt = 1;; rmcnt = Min(rmcnt * 2, 1024)) {
            element = (TElement*)Zone->AllocNoexcept(sizeof(TElement) + key.length() + dataSize);
            if (element) {
                break;
            }

            Y_ENSURE(RemoveOneOldestElement());
            for (int k = 1; k < rmcnt; ++k) {
                if (!RemoveOneOldestElement()) {
                    break;
                }
            }
        }

        element->RBTreeNode.key = crc32;
        element->CreationTime = ngx_current_msec;
        element->KeyLength = key.length();
        element->DataSize = dataSize;
        element->Pid = ngx_getpid();
        element->RequestPtr = requestPtr;

        if (dataSize > 0) {
            std::memcpy(element->DataPtr(), dataBegin, dataSize);
        }

        std::memcpy(element->KeyPtr(), key.data(), key.length());

        ngx_rbtree_insert(&state.RBTree, &element->RBTreeNode);
        if (dataSize > 0) {
            ngx_queue_insert_tail(&state.QReady, &element->QMainNode);
        } else {
            ngx_queue_insert_tail(&state.QInProgress, &element->QMainNode);
        }
    }
}
