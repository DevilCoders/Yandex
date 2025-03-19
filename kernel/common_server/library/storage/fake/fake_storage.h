#pragma once

#include <kernel/common_server/library/storage/abstract.h>

namespace NRTProc {
    class TFakeStorage : public IVersionedStorage {
    public:
        TFakeStorage(const TOptions& options)
            : IVersionedStorage(options)
        {

        }
        ~TFakeStorage() {

        }
        virtual bool RemoveNode(const TString& /*key*/, bool withHistory = false) const override {
            Y_UNUSED(withHistory);
            return true;
        };

        virtual bool ExistsNode(const TString& /*key*/) const override {
            return false;
        }
        virtual bool GetVersion(const TString& /*key*/, i64& version) const override {
            version = 0;
            return true;
        }
        virtual bool GetNodes(const TString& /*key*/, TVector<TString>& result, bool withDirs = false) const override {
            result.clear();
            return true;
            Y_UNUSED(withDirs);
        }

        virtual bool GetValue(const TString& /*key*/, TString& result, i64 version = -1, bool lock = true) const override {
            result = "";
            Y_UNUSED(version);
            Y_UNUSED(lock);
            return true;
        }

        virtual bool SetValue(const TString& /*key*/, const TString& /*value*/, bool storeHistory = true, bool lock = true, i64* version = nullptr) const override {
            return true;
            Y_UNUSED(storeHistory);
            Y_UNUSED(lock);
            Y_UNUSED(version);
        }

    protected:
        virtual TAbstractLock::TPtr NativeWriteLockNode(const TString& /*path*/, TDuration timeout = TDuration::Seconds(100000)) const {
            Y_UNUSED(timeout);
            S_FAIL_LOG << "Incorrect using" << Endl;
            return nullptr;
        }

        virtual TAbstractLock::TPtr NativeReadLockNode(const TString& /*path*/, TDuration timeout = TDuration::Seconds(100000)) const {
            Y_UNUSED(timeout);
            S_FAIL_LOG << "Incorrect using" << Endl;
            return nullptr;

        }
    private:
    };
}
