#pragma once

#include "request_params.h"

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <antirobot/lib/spravka.h>


namespace NAntiRobot {

class ISpravkaSessionsStorage {
public:
    virtual NThreading::TFuture<bool> CheckSession(const TRequest& req, const TSpravka& spravka) = 0;

    virtual ~ISpravkaSessionsStorage() {
    }
};

class TDummySpravkaSessionsStorage : public ISpravkaSessionsStorage {
public:
    NThreading::TFuture<bool> CheckSession(const TRequest&, const TSpravka&) override {
        return NThreading::MakeFuture<bool>(true);
    }
};

class TYdbSessionStorage : public ISpravkaSessionsStorage {
    NYdb::TDriver Driver;
    NYdb::NTable::TTableClient Client;
    const TString TablePath;
    const TString LoadSessionDataQuery;
    const TString StoreSessionDataQuery;

public:
    TYdbSessionStorage();
    NThreading::TFuture<bool> CheckSession(const TRequest& req, const TSpravka& spravka) override;
};

inline ISpravkaSessionsStorage* CreateSpravkaSessionsStorage() {
    if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
        return new TYdbSessionStorage;
    }
    return new TDummySpravkaSessionsStorage;
}

} // namespace NAntiRobot
