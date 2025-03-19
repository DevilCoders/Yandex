#pragma once
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/instant_model.h>
#include <util/datetime/base.h>
#include <util/memory/blob.h>
#include <util/thread/pool.h>

namespace NCS {
    namespace NKVStorage {
        class TBaseOptions {
        private:
            CSA_DEFAULT(TBaseOptions, TString, VisibilityScope);
            CS_ACCESS(TBaseOptions, TInstant, Timestamp, ModelingNow());
            CS_ACCESS(TBaseOptions, TString, UserId, "Undefined");
            CSA_DEFAULT(TBaseOptions, TString, DataHash);
        public:
            virtual TString GetMetaData() const {
                return "";
            }
        };

        class TReadOptions: public TBaseOptions {
        public:
        };

        class TWriteOptions: public TBaseOptions {
        public:
        };

        class IStorage {
        private:
            CSA_READONLY_DEF(TString, StorageName);
            CSA_READONLY(ui32, ThreadsCount, 4);
            mutable TThreadPool DeferredTasksThreads;

        protected:
            virtual bool DoPutData(const TString& path, const TBlob& data, const TWriteOptions& options) const = 0;
            virtual bool DoGetData(const TString& path, TMaybe<TBlob>& data, const TReadOptions& options) const = 0;
            virtual TString DoGetPathByHash(const TBlob& data, const TReadOptions& options) const = 0;
            virtual bool DoPrepareEnvironment(const TBaseOptions& options) const = 0;
        public:
            using TPtr = TAtomicSharedPtr<IStorage>;
            IStorage(const TString& storageName, const ui32 threadsCount = 4)
                : StorageName(storageName)
                , ThreadsCount(threadsCount)
            {
                DeferredTasksThreads.Start(ThreadsCount);
            }
            virtual ~IStorage() {
                DeferredTasksThreads.Stop();
            }
            bool PutData(const TString& path, const TBlob& data, const TWriteOptions& options = TWriteOptions()) const;
            bool PutDataAsync(const TString& path, const TBlob& data, const TWriteOptions& options = TWriteOptions()) const;
            bool GetData(const TString& path, TMaybe<TBlob>& data, const TReadOptions& options = TReadOptions()) const;
            TString GetPathByHash(const TBlob& data, const TReadOptions& options = TReadOptions()) const;
            bool PrepareEnvironment(const TBaseOptions& options) const;
        };
    }
}
