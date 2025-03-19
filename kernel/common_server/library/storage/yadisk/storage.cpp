#include "storage.h"

namespace NRTProc {
    TYaDiskStorage::TYaDiskStorage(const TOptions& options, const TYaDiskStorageOptions& selfOptions)
        : IReadValueOnlyStorage(options)
        , Client(selfOptions)
    {}

    bool TYaDiskStorage::GetValue(const TString& key, TString& result, i64 /*version*/, bool /*lock*/) const {
        TVector<IDocumentStorage::IDocumentStorageFile::TPtr> files;
        if (!Client.GetFiles(key, files)) {
            return false;
        }
        if (files.size() != 1) {
            ERROR_LOG << "unique file not found at given key" << Endl;
            return false;
        }
        if (!files.front()->GetData(result)) {
            return false;
        }
        return true;
    }
}
