#pragma once

#include <kernel/common_server/library/storage/simple_client/storage.h>
#include <kernel/common_server/library/storage/yadisk/config.h>
#include <kernel/common_server/library/disk/client.h>

namespace NRTProc {
    class TYaDiskStorage : public IReadValueOnlyStorage {
    public:
        TYaDiskStorage(const TOptions& options, const TYaDiskStorageOptions& selfOptions);

        virtual bool GetValue(const TString& key, TString& result, i64 version = -1, bool lock = true) const override;

    private:
        TYandexDiskClient Client;
    };
}
