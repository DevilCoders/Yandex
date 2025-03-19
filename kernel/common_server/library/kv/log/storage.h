#pragma once
#include <kernel/common_server/library/kv/abstract/storage.h>
#include <util/system/mutex.h>
#include <util/generic/map.h>

namespace NCS {
    namespace NKVStorage {
        class TLogStorage: public IStorage {
        private:
            using TBase = IStorage;
            bool MemoryUsage = false;

            mutable TMutex Mutex;
            mutable TMap<TString, TBlob> MemoryData;
        protected:
            virtual bool DoPutData(const TString& path, const TBlob& data, const TWriteOptions& options) const override;
            virtual bool DoGetData(const TString& path, TMaybe<TBlob>& data, const TReadOptions& options) const override;
            virtual TString DoGetPathByHash(const TBlob& /*data*/, const TReadOptions& options) const override {
                Y_UNUSED(options);
                return "";
            }
            virtual bool DoPrepareEnvironment(const TBaseOptions& /*options*/) const override {
                return true;
            }
        public:
            TLogStorage(const TString& storageName, const ui32 threadsCount, const bool memoryUsage)
                : TBase(storageName, threadsCount)
                , MemoryUsage(memoryUsage)
            {

            }
        };
    }
}
