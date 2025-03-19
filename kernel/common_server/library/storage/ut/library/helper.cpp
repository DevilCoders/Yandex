#include "helper.h"

#include <kernel/daemon/config/daemon_config.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/logger/global/global.h>

NRTProc::TStorageOptions BuildStorageOptions(const TString& storageType, ui64 cacheLevel, bool consistency) {
        TStringStream ssConfig;
        ssConfig << "CacheLevel: " << cacheLevel << Endl;
        if (storageType == "ZOO") {
            ssConfig << "Type: ZOO" << Endl;
            ssConfig << "<Zoo>" << Endl;
            ssConfig << "Address: saas2-zookeeper1.search.yandex.net:19940,saas2-zookeeper2.search.yandex.net:19940,saas2-zookeeper3.search.yandex.net:19940,saas2-zookeeper4.search.yandex.net:19940,saas2-zookeeper5.search.yandex.net:19940" << Endl;
            ssConfig << "Root: rtline_test_1" << Endl;
            ssConfig << "</Zoo>" << Endl;
        } else if (storageType.StartsWith("Postgres")) {
            ssConfig << "Type: Postgres" << Endl;
            ssConfig << "<Postgres>" << Endl;
            ssConfig << "NotCheckConsistency: " << consistency << Endl;
            ssConfig << "ConnectionString: host=pgaas-test.mail.yandex.net port=12000 dbname=test_drivesmall_db user=robot_carsharing" << Endl;
            if (storageType != "Postgres") {
                ssConfig << "<Versioning>" << Endl;
                ssConfig << "Enable: true" << Endl;
                ssConfig << "</Versioning>" << Endl;
            }
            ssConfig << "TableName: t000" << Endl;
            if (storageType == "Postgres-ZOO") {
                ssConfig << "<Locker>" << Endl;
                ssConfig << "Type: ZOO" << Endl;
                ssConfig << "<Zoo>" << Endl;
                ssConfig << "Address: saas2-zookeeper1.search.yandex.net:19940,saas2-zookeeper2.search.yandex.net:19940,saas2-zookeeper3.search.yandex.net:19940,saas2-zookeeper4.search.yandex.net:19940,saas2-zookeeper5.search.yandex.net:19940" << Endl;
                ssConfig << "Root: ya_drive_supertest_ut" << Endl;
                ssConfig << "</Zoo>" << Endl;
                ssConfig << "</Locker>" << Endl;
            }
            ssConfig << "</Postgres>" << Endl;
        } else if (storageType == "LOCAL") {
            ssConfig << "Type: LOCAL" << Endl;
            ssConfig << "<LOCAL>" << Endl;
            ssConfig << "Root: ./111" << Endl;
            ssConfig << "</LOCAL>" << Endl;
        } else if (storageType == "RTY" || storageType == "RTY-ZOO") {
            ssConfig << "Type: RTY" << Endl
                << "<RTY>" << Endl
                << "IndexHost: saas-indexerproxy-maps-prestable.yandex.net" << Endl
                << "IndexPort: 80" << Endl
                << "IndexKey: 5b94a487879afad899e536bf774bc245" << Endl // json ref
                << "SearchHost: saas-searchproxy-maps-prestable.yandex.net" << Endl
                << "SearchPort: 17000" << Endl
                << "SearchService: drive_offers" << Endl
                << "Prefix: 0" << Endl;
            if (storageType == "RTY-ZOO") {
                ssConfig << "<Locker>" << Endl;
                ssConfig << "Type: ZOO" << Endl;
                ssConfig << "<Zoo>" << Endl;
                ssConfig << "Address: localhost:2181" << Endl;
                ssConfig << "Root: ya_drive_supertest" << Endl;
                ssConfig << "</Zoo>" << Endl;
                ssConfig << "</Locker>" << Endl;
            }
            ssConfig << "</RTY>" << Endl;
        } else {
            UNIT_ASSERT_C(false, "Unknown storage type: " + storageType);
        }

        TAnyYandexConfig yConfig;
        CHECK_WITH_LOG(yConfig.ParseMemory(ssConfig.Str()));

        NRTProc::TStorageOptions options;
        options.Init(yConfig.GetRootSection());

        TStringStream osConfig;
        options.ToString(osConfig);
        Cerr << "Config: " << osConfig.Str() << Endl;

        return options;
    }
