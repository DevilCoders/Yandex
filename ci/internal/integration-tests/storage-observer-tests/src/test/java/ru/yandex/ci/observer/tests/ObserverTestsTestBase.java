package ru.yandex.ci.observer.tests;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;
import org.springframework.test.context.ActiveProfiles;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.ContextHierarchy;
import org.springframework.test.context.TestExecutionListeners;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.observer.api.spring.ObserverApiPropertiesTestConfig;
import ru.yandex.ci.observer.api.statistics.DetailedStatisticsService;
import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.spring.ObserverCorePropertiesTestConfig;
import ru.yandex.ci.observer.core.spring.ObserverYdbTestConfig;
import ru.yandex.ci.observer.reader.spring.ObserverReaderPropertiesTestConfig;
import ru.yandex.ci.observer.tests.logbroker.TestCommonLogbrokerService;
import ru.yandex.ci.observer.tests.spring.TestLogbrokerConfig;
import ru.yandex.ci.observer.tests.spring.TestObserverConfig;
import ru.yandex.ci.observer.tests.spring.TestStorageConfig;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.sharding.ChunkDistributor;
import ru.yandex.ci.storage.core.spring.StorageCoreTestConfig;
import ru.yandex.ci.storage.core.spring.StorageYdbTestConfig;
import ru.yandex.ci.storage.tests.StorageTestsYdbTestBase;
import ru.yandex.ci.storage.tests.TestArcanumClient;
import ru.yandex.ci.storage.tests.spring.StoragePropertiesTestsConfig;
import ru.yandex.ci.storage.tests.tester.StorageTester;
import ru.yandex.ci.test.LoggingTestListener;
import ru.yandex.ci.test.YdbCleanupTestListener;
import ru.yandex.ci.util.Retryable;

@ExtendWith(SpringExtension.class)
@ActiveProfiles(CiProfile.UNIT_TEST_PROFILE)
@ContextHierarchy({
        @ContextConfiguration(classes = {
                TestLogbrokerConfig.class
        }),
        @ContextConfiguration(
                name = "storage",
                classes = {
                        ObserverTestsTestBase.StoragePropertiesConfig.class,
                        StorageYdbTestConfig.class,
                        StorageCoreTestConfig.class,
                        TestStorageConfig.class
                }),
        @ContextConfiguration(
                name = "observer",
                classes = {
                        ObserverTestsTestBase.ObserverPropertiesConfig.class,
                        ObserverYdbTestConfig.class,
                        StorageCoreTestConfig.class,
                        TestObserverConfig.class
                })
})
@TestExecutionListeners(
        value = {LoggingTestListener.class, YdbCleanupTestListener.class},
        mergeMode = TestExecutionListeners.MergeMode.MERGE_WITH_DEFAULTS
)
public class ObserverTestsTestBase {

    @Configuration
    @Import(StoragePropertiesTestsConfig.class)
    @PropertySource("classpath:ci-observer-tests-storage.properties")
    public static class StoragePropertiesConfig {
    }

    @Configuration
    @Import({
            CommonTestConfig.class,
            ObserverCorePropertiesTestConfig.class,
            ObserverApiPropertiesTestConfig.class,
            ObserverReaderPropertiesTestConfig.class
    })
    @PropertySource("classpath:ci-observer-tests-observer.properties")
    public static class ObserverPropertiesConfig {
    }

    @Autowired
    protected TestCommonLogbrokerService logbrokerService;

    @Autowired
    protected StorageTester storageTester;

    @Autowired
    protected StorageCoreCache<?> shardStorageCache;

    @Autowired
    protected StorageCoreCache<?> storageReaderCache;

    @Autowired
    protected StorageCoreCache<?> storageApiCache;

    @Autowired
    protected StorageCoreCache<?> postProcessorCache;

    @Autowired
    protected TestArcanumClient testArcanumClient;

    @Autowired
    protected CiObserverDb ciObserverDb;

    @Autowired
    protected ObserverCache observerCache;

    @Autowired
    protected DetailedStatisticsService detailedStatisticsService;

    @Autowired
    protected ChunkDistributor chunkDistributor;

    protected StorageTestEntitiesBase storageTestEntitiesBase = new StorageTestEntitiesBase();

    public ObserverTestsTestBase() {
        Retryable.disable();
    }

    @BeforeEach
    public void setUp() {
        storageTester.initialize();
        logbrokerService.reset();
        testArcanumClient.reset();
        chunkDistributor.initializeOrCheckState();
    }

    @AfterEach
    public void tearDown() {
        this.shardStorageCache.modify(StorageCoreCache.Modifiable::invalidateAll);
        this.storageReaderCache.modify(StorageCoreCache.Modifiable::invalidateAll);
        this.storageApiCache.modify(StorageCoreCache.Modifiable::invalidateAll);
        this.postProcessorCache.modify(StorageCoreCache.Modifiable::invalidateAll);
        this.observerCache.modify(ObserverCache.Modifiable::invalidateAll);
    }

    public static class StorageTestEntitiesBase extends StorageTestsYdbTestBase {
    }
}
