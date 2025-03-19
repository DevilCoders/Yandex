#include "client.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/system/env.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

Y_UNIT_TEST_SUITE(GeocoderClientSuite) {

    TString CONFIG = R"(
            ApiHost: addrs-testing.search.yandex.net
            ApiPort: 80
            Https: 0
            RequestTimeout: 1s
            <RequestConfig>
                MaxAttempts: 1
                TimeoutSendingms: 8000
                TimeoutConnectms: 5000
                GlobalTimeout: 5000
                TasksCheckInterval: 5000
            </RequestConfig>
            <Customization>
                Type: geocoder
            </Customization>
            DestinationClientId: 2008261
            SelfClientId: 10)";

    Y_UNIT_TEST(Simple) {
        NExternalAPI::TSenderConfig config;
        TAnyYandexConfig yConfig;
        UNIT_ASSERT(yConfig.ParseMemory(CONFIG.data()));
        config.Init(yConfig.GetRootSection(), nullptr);
        NExternalAPI::TSender client(config, "test-client", nullptr);

        TGeoCoord coordinate = { 37.420038, 55.849238 };
        auto reply = client.SendRequest<TGeocoderRequest>(coordinate, LANG_RUS);
        UNIT_ASSERT(reply.IsSuccess());

        UNIT_ASSERT_VALUES_EQUAL(reply.GetDetails().GeoId, 213);
        UNIT_ASSERT(reply.GetDetails().Title);
        UNIT_ASSERT_VALUES_EQUAL(reply.GetDetails().Kind, "house");
        UNIT_ASSERT(!reply.GetDetails().Address.Street.empty());
    }

    /*
    Y_UNIT_TEST(WithTvm) {
        ui32 source = FromStringWithDefault(GetEnv("TVM_SOURCE"), 2000184);
        TString token = GetEnv("TVM_TOKEN", "2MOzJtXfbLvZ3ySzj3zmRw");
        UNIT_ASSERT(token);

        NLogistic::TGeocoder::TOptions options;
        options.Host = "addrs.yandex.ru";
        options.Port = 17140;
        options.Path = "yandsearch";
        options.TvmDestination = 2001886;
        options.TvmSource = source;
        options.TvmToken = token;
        NLogistic::TGeocoder client(options);

        TGeoCoord coordinate = { 37.589961, 55.733858 };
        auto decodedF = client.Decode(coordinate);
        UNIT_ASSERT(decodedF.Initialized());
        NLogistic::TGeocoder::TResponse decoded = decodedF.GetValueSync();
        UNIT_ASSERT_VALUES_EQUAL(decoded.GeoId, 213);
        UNIT_ASSERT(decoded.Title);
        UNIT_ASSERT_VALUES_EQUAL(decoded.Kind, "house");
    }
     */
}
