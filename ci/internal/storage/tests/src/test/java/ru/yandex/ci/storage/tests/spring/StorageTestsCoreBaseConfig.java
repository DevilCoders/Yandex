package ru.yandex.ci.storage.tests.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.storage.api.controllers.StorageApiController;
import ru.yandex.ci.storage.api.controllers.StorageFrontApiController;
import ru.yandex.ci.storage.api.controllers.StorageFrontHistoryApiController;
import ru.yandex.ci.storage.api.controllers.StorageFrontTestsApiController;
import ru.yandex.ci.storage.api.controllers.public_api.CommonPublicApiController;
import ru.yandex.ci.storage.api.controllers.public_api.RawDataPublicApiController;
import ru.yandex.ci.storage.api.controllers.public_api.SearchPublicApiController;
import ru.yandex.ci.storage.core.bazinga.SingleThreadBazinga;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.test_metrics.TestMetricsService;
import ru.yandex.ci.storage.core.yt.YtClientFactory;
import ru.yandex.ci.storage.tests.BadgeEventsSenderTestImpl;
import ru.yandex.ci.storage.tests.TestAYamlerClientImpl;
import ru.yandex.ci.storage.tests.TestArcanumClient;
import ru.yandex.ci.storage.tests.TestCiClient;
import ru.yandex.ci.storage.tests.TestMetricsServiceImpl;
import ru.yandex.ci.storage.tests.TestsArcService;
import ru.yandex.ci.storage.tests.api.tester.CommonPublicApiTester;
import ru.yandex.ci.storage.tests.api.tester.RawDataPublicApiTester;
import ru.yandex.ci.storage.tests.api.tester.SearchPublicApiTester;
import ru.yandex.ci.storage.tests.api.tester.StorageApiTester;
import ru.yandex.ci.storage.tests.api.tester.StorageFrontApiTester;
import ru.yandex.ci.storage.tests.api.tester.StorageFrontHistoryApiTester;
import ru.yandex.ci.storage.tests.api.tester.StorageFrontTestsApiTester;
import ru.yandex.ci.storage.tests.logbroker.TestLogbrokerService;
import ru.yandex.ci.storage.tests.logbroker.TestsMainStreamWriter;
import ru.yandex.ci.storage.tests.tester.StorageTester;
import ru.yandex.ci.storage.tests.yt.YtClientFactoryEmptyImpl;

@Configuration
@Import({
        StorageTestsLogbrokerWritersConfig.class,
})
public class StorageTestsCoreBaseConfig {

    @Bean
    public TestAYamlerClientImpl aYamlerClient() {
        return new TestAYamlerClientImpl();
    }

    @Bean
    public TestsArcService testsArcService() {
        return new TestsArcService();
    }

    @Bean
    public TestArcanumClient arcanumClient() {
        return new TestArcanumClient(); // WTF?
    }

    @Bean
    public HttpClientProperties arcanumHttpClientProperties() {
        return HttpClientProperties.ofEndpoint("http://localhost"); // WTF?
    }

    @Bean
    public TestCiClient ciClient() {
        return new TestCiClient();
    }

    @Bean
    public TestMetricsService emptyTestMetricsService() {
        return new TestMetricsServiceImpl();
    }

    @Bean
    public YtClientFactory ytClientFactory() {
        return new YtClientFactoryEmptyImpl();
    }

    @Bean
    public StorageApiTester storageApiTester(StorageApiController apiController) {
        return new StorageApiTester(apiController);
    }

    @Bean
    public StorageFrontApiTester storageFrontApiTester(StorageFrontApiController apiController) {
        return new StorageFrontApiTester(apiController);
    }

    @Bean
    public StorageFrontHistoryApiTester storageFrontHistoryApiTester(StorageFrontHistoryApiController apiController) {
        return new StorageFrontHistoryApiTester(apiController);
    }

    @Bean
    public StorageFrontTestsApiTester storageFrontTestsApiTester(StorageFrontTestsApiController apiController) {
        return new StorageFrontTestsApiTester(apiController);
    }

    @Bean
    public CommonPublicApiTester commonPublicApiTester(CommonPublicApiController commonPublicApiController) {
        return new CommonPublicApiTester(commonPublicApiController);
    }

    @Bean
    public SearchPublicApiTester searchPublicApiTester(SearchPublicApiController searchPublicApiController) {
        return new SearchPublicApiTester(searchPublicApiController);
    }

    @Bean
    public RawDataPublicApiTester rawDataPublicApiTester(RawDataPublicApiController rawDataPublicApiController) {
        return new RawDataPublicApiTester(rawDataPublicApiController);
    }

    @Bean
    public BadgeEventsSenderTestImpl badgeEventsSender() {
        return new BadgeEventsSenderTestImpl();
    }

    @Bean
    public StorageTester storageTester(
            CiStorageDb db,
            TestsMainStreamWriter mainStreamWriter,
            TestLogbrokerService logbrokerService,
            StorageApiTester storageApiTester,
            StorageFrontApiTester storageFrontApiTester,
            StorageFrontHistoryApiTester storageFrontHistoryApiTester,
            StorageFrontTestsApiTester storageFrontTestsApiTester,
            CommonPublicApiTester commonPublicApiTester,
            SearchPublicApiTester searchPublicApiTester,
            RawDataPublicApiTester rawDataPublicApiTester,
            SingleThreadBazinga singleThreadBazinga
    ) {
        return new StorageTester(
                db, mainStreamWriter, logbrokerService,
                storageApiTester, storageFrontApiTester, storageFrontHistoryApiTester,
                storageFrontTestsApiTester,
                commonPublicApiTester,
                searchPublicApiTester,
                rawDataPublicApiTester,
                singleThreadBazinga
        );
    }
}
