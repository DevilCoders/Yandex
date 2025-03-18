package ru.yandex.ci.storage.reader.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.model.StorageEnvironment;
import ru.yandex.ci.storage.core.spring.StorageEnvironmentConfig;
import ru.yandex.ci.storage.core.spring.StorageTemporalServiceConfig;
import ru.yandex.ci.storage.reader.AggregatedStatisticsService;
import ru.yandex.ci.storage.reader.check.CheckEventsListener;
import ru.yandex.ci.storage.reader.check.TestRestartPlanner;
import ru.yandex.ci.storage.reader.check.listeners.LargeAutostartCheckEventsListener;
import ru.yandex.ci.storage.reader.check.listeners.PostCommitChecksListener;
import ru.yandex.ci.storage.reader.check.listeners.PrestableYtExportListener;
import ru.yandex.ci.storage.reader.check.listeners.TemporalYtExportListener;
import ru.yandex.ci.storage.reader.check.listeners.YtExportListener;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        MainStreamConfig.class,
        ShardOutStreamConfig.class,
        EventsStreamConfig.class,
        StorageReaderCoreConfig.class,
        StorageTemporalServiceConfig.class,
        StorageEnvironmentConfig.class
})
public class StorageReaderConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public AggregatedStatisticsService aggregatedStatisticsService(CiStorageDb db) {
        var service = new AggregatedStatisticsService(db, meterRegistry);
        var thread = new Thread(service::start, "aggregates-statistics");
        thread.start();

        return service;
    }

    @Bean
    public LargeAutostartCheckEventsListener largeAutostartProcessor(
            BazingaTaskManager bazingaTaskManager,
            RequirementsService requirementsService
    ) {
        return new LargeAutostartCheckEventsListener(bazingaTaskManager, requirementsService);
    }

    @Bean
    @Profile(value = CiProfile.TESTING_PROFILE)
    public CheckEventsListener ytExportListener(TemporalService temporalService) {
        return new TemporalYtExportListener(temporalService);
    }

    @Bean
    @Profile(value = CiProfile.PRESTABLE_PROFILE)
    public CheckEventsListener prestableYtExportListener(BazingaTaskManager bazingaTaskManager) {
        return new PrestableYtExportListener(bazingaTaskManager);
    }

    @Bean
    @Profile(value = CiProfile.STABLE_PROFILE)
    public CheckEventsListener stableYtExportListener(BazingaTaskManager bazingaTaskManager) {
        return new YtExportListener(bazingaTaskManager);
    }


    @Bean
    public TestRestartPlanner testRestartPlanner(
            BazingaTaskManager bazingaTaskManager,
            StorageEnvironment storageEnvironment
    ) {
        return new TestRestartPlanner(bazingaTaskManager, 500, 2500, 5, storageEnvironment.getValue());
    }

    @Bean
    public PostCommitChecksListener postCommitChecksListener(
            BazingaTaskManager bazingaTaskManager
    ) {
        return new PostCommitChecksListener(bazingaTaskManager);
    }
}
