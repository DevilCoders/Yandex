#include "storage.h"

namespace NRTProc {
    TClientStorage::TClientStorage(const TOptions& options, const TClientStorageOptions& selfOptions)
        : IReadValueOnlyStorage(options)
        , Client(selfOptions)
        , RequestTimeout(selfOptions.GetRequestTimeout())
    {}

    bool TClientStorage::GetValue(const TString& key, TString& result, i64 /*version*/, bool /*lock*/) const {
        const TInstant deadline = Now() + RequestTimeout;
        NNeh::THttpRequest request;
        request.SetUri(key);
        DEBUG_LOG << request.GetDebugRequest() << Endl;
        auto reply = Client->SendMessageSync(request, deadline);
        if (!reply.IsSuccessReply()) {
            ERROR_LOG << reply.Code() << " " << reply.Content() << " " << reply.ErrorMessage() << Endl;
            return false;
        }
        result = reply.Content();
        return true;
    }

    IReadValueOnlyStorage::IReadValueOnlyStorage(const TOptions& options)
        : IVersionedStorage(options)
    {}

    bool IReadValueOnlyStorage::RemoveNode(const TString& /*key*/, bool /*withHistory*/) const {
        ERROR_LOG << "Incorrect operation" << Endl;
        return false;
    }

    bool IReadValueOnlyStorage::ExistsNode(const TString& /*key*/) const {
        ERROR_LOG << "Incorrect operation" << Endl;
        return false;
    }

    bool IReadValueOnlyStorage::GetVersion(const TString& /*key*/, i64& version) const {
        version = 0;
        ERROR_LOG << "Incorrect operation" << Endl;
        return false;
    }

    bool IReadValueOnlyStorage::GetNodes(const TString& /*key*/, TVector<TString>& /*result*/, bool /*withDirs*/) const {
        ERROR_LOG << "Incorrect operation" << Endl;
        return false;
    }

    bool IReadValueOnlyStorage::SetValue(const TString& /*key*/, const TString& /*value*/, bool /*storeHistory*/, bool /*lock*/, i64* /*version*/) const {
        ERROR_LOG << "Incorrect operation" << Endl;
        return false;
    }

    TAbstractLock::TPtr IReadValueOnlyStorage::NativeWriteLockNode(const TString& /*path*/, TDuration /*timeout*/) const {
        S_FAIL_LOG << "Incorrect using" << Endl;
        return nullptr;
    }

    TAbstractLock::TPtr IReadValueOnlyStorage::NativeReadLockNode(const TString& /*path*/, TDuration /*timeout*/) const {
        S_FAIL_LOG << "Incorrect using" << Endl;
        return nullptr;
    }
}
