#pragma once

#include <kernel/common_server/library/storage/abstract.h>

#include <mapreduce/yt/interface/client.h>
#include "config.h"

namespace NRTProc {

    class TYTStorage: public IVersionedStorage {
    private:
        const TYTOptions SelfOptions;
        NYT::IClientPtr Client;
        NYT::TYPath GetYTPath(const TString& path) const {
            return NYT::TYPath(SelfOptions.GetRootPath() + "/" + path);
        }
    public:
        TYTStorage(const IVersionedStorage::TOptions& options, const TYTOptions& selfOptions)
            : IVersionedStorage(options)
            , SelfOptions(selfOptions)
        {
            NYT::JoblessInitialize();
            Client = NYT::CreateClient(SelfOptions.GetServerName());
        }

        virtual ~TYTStorage() {

        }

        virtual bool GetVersion(const TString& key, i64& version) const override {
            version = 0;
            return Client->Exists(GetYTPath(key));
        }

        virtual bool RemoveNode(const TString& key, bool /*withHistory*/ = false) const override {
            NYT::TRemoveOptions options;
            options.Force(true).Recursive(true);
            Client->Remove(GetYTPath(key), options);
            return true;
        }

        virtual bool ExistsNode(const TString& key) const override {
            return Client->Exists(GetYTPath(key));
        }

        virtual bool GetValue(const TString& key, TString& result, i64 version = -1, bool lock = true) const override {
            Y_UNUSED(version);
            Y_UNUSED(lock);
            NYT::TNode node = Client->Get(GetYTPath(key));
            if (node.IsNull()) {
                return false;
            } else {
                result = node.AsString();
                return true;
            }
        }

        virtual bool SetValue(const TString& key, const TString& value, bool storeHistory = true, bool lock = true, i64* version = nullptr) const override {
            Y_UNUSED(version);
            Y_UNUSED(lock);
            Y_UNUSED(storeHistory);
            NYT::TCreateOptions options;
            options.Force(true).Recursive(true);
            Client->Create(GetYTPath(key), NYT::NT_STRING, options);
            Client->Set(GetYTPath(key), value);
            return true;
        }

        virtual bool GetNodes(const TString& key, TVector<TString>& result, bool withDirs = false) const override {
            NYT::TNode::TListType list = Client->List(GetYTPath(key));
            for (auto&& i : list) {
                if (i.IsMap() && !withDirs) {
                    continue;
                }
                result.push_back(i.AsString());
            }
            return !result.empty();
        }

    protected:
        virtual TAbstractLock::TPtr NativeWriteLockNode(const TString& /*path*/, TDuration /*timeout*/) const override {
            S_FAIL_LOG << "!!!" << Endl;
            return nullptr;
        }
        virtual TAbstractLock::TPtr NativeReadLockNode(const TString& /*path*/, TDuration /*timeout*/) const override {
            S_FAIL_LOG << "!!!" << Endl;
            return nullptr;
        }

    };
}
