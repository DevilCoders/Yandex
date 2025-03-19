#include "config.h"

#include <kernel/daemon/config/daemon_config.h>

#include <library/cpp/testing/unittest/registar.h>

void ExecutorConfigToString(IOutputStream& os, const TString& storageType, const ui32 threadsCount, bool syncMode, const TString& queueTableName, const TString& storageTableName) {
    Y_UNUSED(syncMode);
    os << "ThreadsCountDequeue: " << threadsCount << Endl;
    os << "ThreadsCountExecute: " << threadsCount << Endl;
    os << "ClearingRegularity: 10s" << Endl;
    os << "WaitingDurationClearTasks: 10s" << Endl;
    os << "WaitingDurationClearData: 10s" << Endl;
    os << "UnlockDataWaitingDuration: 100ms" << Endl;
    os << "EventLogUsage: true" << Endl;
    os << "<Queue>" << Endl;
    os << "QueueIdentifier: QueueForUT" << Endl;
    os << "Type: VStorage" << Endl;
    os << "<Storage>" << Endl;
    if (storageType == "Postgres" || storageType == "Postgres-ZOO") {
        os << "Type: Postgres" << Endl;
        os << "<Postgres>" << Endl;
        os << "ConnectionString: host=pgaas-test.mail.yandex.net port=12000 dbname=test_drivesmall_db user=robot_carsharing password=PWM1Mzk4ZTQyZmEwMDY1MDI2Zj" << Endl;
        os << "TableName: " << queueTableName << Endl;
        if (storageType == "Postgres-ZOO") {
            os << "<Locker>" << Endl;
            os << "Type: ZOO" << Endl;
            os << "<Zoo>" << Endl;
            os << "Address: localhost:2181" << Endl;
            os << "Root: ya_drive_supertest_queue" << Endl;
            os << "</Zoo>" << Endl;
            os << "</Locker>" << Endl;
        }
        os << "</Postgres>" << Endl;
    } else {
        os << "Type: LOCAL" << Endl;
        os << "<LOCAL>" << Endl;
        os << "Root: ./000" << Endl;
        os << "</LOCAL>" << Endl;
    }
    os << "</Storage>" << Endl;
    os << "</Queue>" << Endl;
    os << "<Storage>" << Endl;
    os << "Type: VStorage" << Endl;
    os << "<Storage>" << Endl;
    if (storageType == "Postgres" || storageType == "Postgres-ZOO") {
        os << "Type: Postgres" << Endl;
        os << "<Postgres>" << Endl;
        os << "ConnectionString: host=pgaas-test.mail.yandex.net port=12000 dbname=test_drivesmall_db user=robot_carsharing password=PWM1Mzk4ZTQyZmEwMDY1MDI2Zj" << Endl;
        os << "TableName: " << storageTableName << Endl;
        if (storageType == "Postgres-ZOO") {
            os << "<Locker>" << Endl;
            os << "Type: ZOO" << Endl;
            os << "<Zoo>" << Endl;
            os << "Address: localhost:2181" << Endl;
            os << "Root: ya_drive_supertest_storage" << Endl;
            os << "</Zoo>" << Endl;
            os << "</Locker>" << Endl;
        }
        os << "</Postgres>" << Endl;
    } else if (storageType == "LOCAL") {
        os << "Type: LOCAL" << Endl;
        os << "<LOCAL>" << Endl;
        os << "Root: ./111" << Endl;
        os << "</LOCAL>" << Endl;
    } else if (storageType == "RTY") {
        os << "Type: RTY" << Endl
            << "<RTY>" << Endl
            << "IndexHost: saas-indexerproxy-maps-prestable.yandex.net" << Endl
            << "IndexPort: 80" << Endl
            << "IndexKey: 5b94a487879afad899e536bf774bc245" << Endl // json ref
            << "SearchHost: saas-searchproxy-maps-prestable.yandex.net" << Endl
            << "SearchPort: 17000" << Endl
            << "SearchService: drive_offers" << Endl
            << "Prefix: 0" << Endl;

        os << "<Locker>" << Endl;
        os << "Type: ZOO" << Endl;
        os << "<Zoo>" << Endl;
        os << "Address: localhost:2181" << Endl;
        os << "Root: ya_drive_supertest" << Endl;
        os << "</Zoo>" << Endl;
        os << "</Locker>" << Endl;
        os << "</RTY>" << Endl;
    } else {
        UNIT_ASSERT_C(false, "Unknown storage type: " + storageType);
    }

    os << "</Storage>" << Endl;
    os << "</Storage>" << Endl;
}

TTaskExecutorConfig BuildExecutorConfig(const TString& storageType, const ui32 threadsCount, bool syncMode, const TString& queueTableName, const TString& storageTableName) {
    TTaskExecutorConfig config;
    TFsPath("./000").ForceDelete();
    TFsPath("./111").ForceDelete();
    TStringStream ssConfig;
    ExecutorConfigToString(ssConfig, storageType, threadsCount, syncMode, queueTableName, storageTableName);

    TAnyYandexConfig yConfig;
    CHECK_WITH_LOG(yConfig.ParseMemory(ssConfig.Str()));
    config.Init(yConfig.GetRootSection());

    TStringStream ss;
    yConfig.PrintConfig(ss);
    Cerr << "Config: " << ss.Str() << Endl;
    return config;
}
