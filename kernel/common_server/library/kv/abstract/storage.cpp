#include "storage.h"
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/util/object_counter/object_counter.h>

namespace NCS {
    namespace NKVStorage {
        bool IStorage::PutData(const TString& path, const TBlob& data, const TWriteOptions& options /*= TWriteOptions()*/) const {
            TFsPath fsPath(path);
            auto gLogging = TFLRecords::StartContext()("&storage_name", StorageName).SignalId("storage_put_data")("visibility", options.GetVisibilityScope())("path", path);
            if (!PrepareEnvironment(options)) {
                TFLEventLog::Error().Signal()("&code", "bad_env");
                return false;
            }
            if (DoPutData(fsPath.GetPath(), data, options)) {
                TFLEventLog::JustSignal()("&code", "success");
                return true;
            } else {
                TFLEventLog::Error().Signal()("&code", "error");
                return false;
            }
        }

        class TPutDataTask: public IObjectInQueue, public NCSUtil::TObjectCounter<TPutDataTask> {
        private:
            const IStorage* Owner = nullptr;
            const TString Path;
            const TBlob Data;
            const TWriteOptions Options;
        public:
            TPutDataTask(const IStorage* owner, const TString& path, const TBlob& data, const TWriteOptions& options = TWriteOptions())
                : Owner(owner)
                , Path(path)
                , Data(data)
                , Options(options)
            {
            }

            virtual void Process(void* /*threadSpecificResource*/) override {
                if (!Owner->PutData(Path, Data, Options)) {
                    TFLEventLog::Alert("Fail put async data")("path", Path)("data", Data.AsStringBuf());
                }
            }
        };

        bool IStorage::PutDataAsync(const TString& path, const TBlob& data, const TWriteOptions& options /*= TWriteOptions()*/) const {
            auto gLogging = TFLRecords::StartContext()("&storage_name", StorageName).SignalId("storage_put_data_async")("visibility", options.GetVisibilityScope())("path", path);
            if (!PrepareEnvironment(options)) {
                TFLEventLog::Error().Signal()("&code", "bad_env");
                return false;
            }
            if (DeferredTasksThreads.AddAndOwn(MakeHolder<TPutDataTask>(this, path, data, options))) {
                TFLEventLog::JustSignal()("&code", "success");
                return true;
            } else {
                TFLEventLog::Error().Signal()("&code", "error");
                return false;
            }
        }

        bool IStorage::GetData(const TString& path, TMaybe<TBlob>& data, const TReadOptions& options) const {
            auto gLogging = TFLRecords::StartContext()("&storage_name", StorageName).SignalId("storage_get_data")("visibility", options.GetVisibilityScope())("path", path);
            if (!PrepareEnvironment(options)) {
                TFLEventLog::Error().Signal()("&code", "bad_env");
                return false;
            }
            TFsPath fsPath(path);
            if (DoGetData(fsPath.GetPath(), data, options)) {
                TFLEventLog::JustSignal()("&code", "success");
                return true;
            } else {
                TFLEventLog::Error().Signal()("&code", "error");
                return false;
            }
        }

        TString IStorage::GetPathByHash(const TBlob& data, const TReadOptions& options) const {
            auto gLogging = TFLRecords::StartContext()("&storage_name", StorageName).SignalId("storage_get_path_by_hash")("visibility", options.GetVisibilityScope());
            if (data.Empty()) {
                TFLEventLog::JustSignal()("&code", "empty_data");
                return "";
            }
            if (!PrepareEnvironment(options)) {
                TFLEventLog::Error().Signal()("&code", "bad_env");
                return "";
            }
            return DoGetPathByHash(data, options);
        }

        bool IStorage::PrepareEnvironment(const TBaseOptions& options) const {
            return DoPrepareEnvironment(options);
        }

    }
}
