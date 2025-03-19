#pragma once
#include <kernel/common_server/library/executor/abstract/storage.h>
#include <kernel/common_server/library/storage/abstract.h>
#include <library/cpp/threading/future/async.h>

class TVSStorageConfig: public IDTasksStorageConfig {
private:
    static TFactory::TRegistrator<TVSStorageConfig> Registrator;
    NRTProc::TStorageOptions Options;
protected:

    virtual bool Init(const TYandexConfig::Section* section) override {
        auto child = section->GetAllChildren();
        auto it = child.find("Storage");
        AssertCorrectConfig(it != child.end(), "Incorrect configuration: no 'VStorage' section");
        AssertCorrectConfig(Options.Init(it->second), "Incorrect configuration: cannot initialize options for storage");
        return true;
    }

    virtual void DoToString(IOutputStream& os) const override {
        os << "<Storage>" << Endl;
        Options.ToString(os);
        os << "</Storage>" << Endl;
    }

public:

    virtual ~TVSStorageConfig() = default;

    const NRTProc::TStorageOptions& GetStorageOptions() const {
        return Options;
    }

    virtual TString GetName() const override {
        return "VStorage";
    }

    virtual IDTasksStorage::TPtr Construct() const override;
};

class TVSTasksStorage: public IDTasksStorage {
private:
    const TVSStorageConfig Config;
    NRTProc::IVersionedStorage::TPtr Storage;
    mutable TThreadPool AsyncQueue;

protected:
    virtual bool ReadDataImpl(const TVector<TString>& identifiers, TVector<NRTProc::IVersionedStorage::TKValue>& result) const override {
        return Storage->GetValues(MakeSet(identifiers), result);
    }

    virtual EReadStatus ReadDataImpl(const TString& path, TBlob& result, TInstant deadline = TInstant::Max()) const override {
        EReadStatus status = EReadStatus::OK;
        if (deadline != TInstant::Max()) {
            auto func = [this, path]() -> TString {
                TString value;
                if (this->Storage->GetValue(path, value, -1, false)) {
                    return value;
                }
                return Default<TString>();
            };
            NThreading::TFuture<TString> future = NThreading::Async(func, AsyncQueue);
            future.Wait(deadline - TDuration::MilliSeconds(200));
            if (future.HasValue()) {
                if (future.GetValue()) {
                    result = TBlob::FromString(future.GetValue());
                    status = EReadStatus::OK;
                } else {
                    status = EReadStatus::NotFound;
                }
            } else {
                status = EReadStatus::Timeout;
            }
        } else {
            TString value;
            if (Storage->GetValue(path, value, -1, false)) {
                result = TBlob::FromString(value);
                status = EReadStatus::OK;
            } else {
                status = EReadStatus::NotFound;
            }
        }
        return status;
    }

    virtual bool StoreDataImpl2(const TString& path, const TBlob& data) const override {
        TString dataStr(data.AsCharPtr(), data.Size());
        return Storage->SetValue(path, dataStr, false, false);
    }

    virtual bool RemoveDataImpl(const TSet<TString>& pathes) const override {
        return Storage->RemoveNodes(pathes);
    }

    virtual bool RemoveDataImpl(const TString& path) const override {
        return Storage->RemoveNode(path);
    }

    virtual NRTProc::TAbstractLock::TPtr LockData(const TString& identifier, TDuration timeout = TDuration::Zero()) const override {
        return Storage->WriteLockNode(identifier, timeout);
    }

    virtual TVector<TString> GetNodes() const override {
        TVector<TString> nodes;
        if (Storage->GetNodes("/", nodes, true)) {
            return nodes;
        }
        return TVector<TString>();
    }

public:

    virtual ~TVSTasksStorage() {
        AsyncQueue.Stop();
    }

    TVSTasksStorage(const TVSStorageConfig& config)
        : Config(config) {
        Storage = Config.GetStorageOptions().ConstructStorage();
        AsyncQueue.Start(128);
    }
};

