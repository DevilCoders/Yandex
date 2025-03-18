package ru.yandex.ci.storage.tms.spring;

import java.time.Clock;
import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.arcanum.ArcanumClient;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.client.oldci.OldCiClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.storage.core.archive.ArchiveTask;
import ru.yandex.ci.storage.core.archive.CheckArchiveService;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.check.PostCommitNotificationService;
import ru.yandex.ci.storage.core.check.TestRestartService;
import ru.yandex.ci.storage.core.check.tasks.ArcanumCheckStatusReporterTask;
import ru.yandex.ci.storage.core.check.tasks.CancelIterationFlowTask;
import ru.yandex.ci.storage.core.check.tasks.ProcessPostCommitTask;
import ru.yandex.ci.storage.core.check.tasks.RestartTestsTask;
import ru.yandex.ci.storage.core.clickhouse.AutocheckClickhouse;
import ru.yandex.ci.storage.core.clickhouse.ClickHouseExportService;
import ru.yandex.ci.storage.core.clickhouse.ClickHouseExportServiceEmptyImpl;
import ru.yandex.ci.storage.core.clickhouse.ClickHouseExportServiceImpl;
import ru.yandex.ci.storage.core.clickhouse.ExportToClickhouse;
import ru.yandex.ci.storage.core.clickhouse.SpClickhouseConfig;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsSender;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.model.StorageEnvironment;
import ru.yandex.ci.storage.core.spring.BadgeEventsConfig;
import ru.yandex.ci.storage.core.spring.ClientsConfig;
import ru.yandex.ci.storage.core.spring.LogbrokerConfig;
import ru.yandex.ci.storage.core.spring.StorageEnvironmentConfig;
import ru.yandex.ci.storage.core.spring.StorageEventProducerConfig;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;
import ru.yandex.ci.storage.core.ydb.SequenceService;
import ru.yandex.ci.storage.core.yt.IterationToYtExporter;
import ru.yandex.ci.storage.core.yt.YtClientFactory;
import ru.yandex.ci.storage.core.yt.impl.IterationToYtExporterImpl;
import ru.yandex.ci.storage.core.yt.impl.YtExportTask;
import ru.yandex.ci.storage.tms.cron.ArchiveCron;
import ru.yandex.ci.storage.tms.cron.CheckIdGenerationCron;
import ru.yandex.ci.storage.tms.cron.GroupsSyncCronTask;
import ru.yandex.ci.storage.tms.cron.IterationTimeoutCronTask;
import ru.yandex.ci.storage.tms.cron.MissingRevisionsCronTask;
import ru.yandex.ci.storage.tms.cron.MuteDigestCron;
import ru.yandex.ci.storage.tms.cron.OldCiMuteCron;
import ru.yandex.ci.storage.tms.cron.StatisticsCronTask;
import ru.yandex.ci.storage.tms.monitoring.BazingaMonitoringSchedule;
import ru.yandex.ci.storage.tms.services.MuteDigestService;
import ru.yandex.ci.storage.tms.spring.bazinga.StorageBazingaServiceConfig;
import ru.yandex.ci.storage.tms.spring.clients.YtClientConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        ClientsConfig.class,
        StorageYdbConfig.class,
        StorageBazingaServiceConfig.class,
        ArcanumClientConfig.class,
        LogbrokerConfig.class,
        LargeTasksConfig.class,
        StorageTmsCacheConfig.class,
        StorageEventProducerConfig.class,
        BadgeEventsConfig.class,
        BazingaMonitoringSchedule.class,
        SpClickhouseConfig.class,
        StorageEnvironmentConfig.class,
        YtClientConfig.class
})
public class StorageTmsConfig {

    @Bean
    public IterationTimeoutCronTask iterationTimeoutCronTask(
            CiStorageDb db,
            StorageEventsProducer storageEventsProducer,
            @Value("${storage.iterationTimeoutCronTask.cancelOlderThan}") Duration cancelOlderThan,
            @Value("${storage.iterationTimeoutCronTask.period}") Duration period
    ) {
        return new IterationTimeoutCronTask(db, storageEventsProducer, Clock.systemUTC(), cancelOlderThan, period);
    }

    @Bean
    public StatisticsCronTask statisticsCronTask(
            CiStorageDb db,
            @Value("${storage.statisticsCronTask.period}") Duration period
    ) {
        return new StatisticsCronTask(db, period);
    }

    @Bean
    public CheckIdGenerationCron checkIdGenerationCron(CiStorageDb db) {
        return new CheckIdGenerationCron(db, 32688, 60);
    }

    @Bean
    @Profile(value = CiProfile.STABLE_PROFILE)
    public OldCiMuteCron oldCiMuteCron(CiStorageDb db, OldCiClient oldCiClient) {
        return new OldCiMuteCron(db, oldCiClient, 60);
    }

    @Bean
    public TestRestartService testRestartService(
            CiStorageDb db,
            CiClient ciClient,
            StorageEventsProducer storageEventsProducer
    ) {
        return new TestRestartService(db, storageEventsProducer, ciClient, 2500);
    }

    @Bean
    public RestartTestsTask restartTestsTask(TestRestartService testRestartService) {
        return new RestartTestsTask(testRestartService);
    }

    @Bean
    public PostCommitNotificationService postCommitNotificationService(
            CiStorageDb db, BadgeEventsSender badgeEventsSender
    ) {
        return new PostCommitNotificationService(db, badgeEventsSender);
    }

    @Bean
    public ProcessPostCommitTask processPostCommitTask(PostCommitNotificationService service) {
        return new ProcessPostCommitTask(service);
    }

    @Bean
    public ArcanumCheckStatusReporterTask arcanumCheckStatusReporterTask(
            StorageCoreCache<?> storageTmsCache,
            ArcanumClient arcanumClient
    ) {
        return new ArcanumCheckStatusReporterTask(storageTmsCache, arcanumClient);
    }

    @Bean
    public IterationToYtExporter iterationToYtExporter(
            CiStorageDb db,
            YtClientFactory ytClientFactory,
            @Value("${storage.iterationToYtExporter.rootPath}") String rootPath,
            @Value("${storage.iterationToYtExporter.batchSize}") int batchSize
    ) {
        return new IterationToYtExporterImpl(db, rootPath, ytClientFactory, batchSize);
    }

    @Bean
    public YtExportTask ytExport(IterationToYtExporter iterationToYtExporter) {
        return new YtExportTask(iterationToYtExporter);
    }

    @Bean
    public CancelIterationFlowTask cancelIterationFlowTask(
            StorageCoreCache<?> storageTmsCache,
            CiClient ciClient,
            StorageEnvironment storageEnvironment
    ) {
        return new CancelIterationFlowTask(storageTmsCache, ciClient, storageEnvironment.getValue());
    }

    @Bean
    public GroupsSyncCronTask groupsSyncCronTask(CiStorageDb db, ArcanumClient arcanumClient) {
        return new GroupsSyncCronTask(db, arcanumClient, 60);
    }

    @Bean
    public MissingRevisionsCronTask missingRevisionsCronTask(CiStorageDb db, ArcService arcService) {
        return new MissingRevisionsCronTask(db, arcService);
    }

    @Bean
    @Profile(value = CiProfile.NOT_STABLE_OR_TESTING_PROFILE)
    public ClickHouseExportService clickHouseExportServiceEmpty() {
        return new ClickHouseExportServiceEmptyImpl();
    }

    @Bean
    public SequenceService sequenceService(CiStorageDb db) {
        return new SequenceService(db);
    }

    @Bean
    @Profile(value = CiProfile.STABLE_OR_TESTING_PROFILE)
    public ClickHouseExportService clickHouseExportService(
            CiStorageDb db, SequenceService sequenceService,
            AutocheckClickhouse clickhouse,
            @Value("${storage.clickHouseExportService.batchSize}") int batchSize
    ) {
        return new ClickHouseExportServiceImpl(db, clickhouse, sequenceService, batchSize);
    }

    @Bean
    public ExportToClickhouse exportToClickhouse(ClickHouseExportService exportService) {
        return new ExportToClickhouse(exportService);
    }

    @Bean
    public CheckArchiveService checkArchiveService(
            Clock clock,
            CiStorageDb db,
            BazingaTaskManager bazingaTaskManager,
            @Value("${storage.checkArchiveService.archiveOlderThan}") Duration archiveOlderThan,
            @Value("${storage.checkArchiveService.maxArchiveInProgress}") int maxArchiveInProgress
    ) {
        return new CheckArchiveService(clock, db, bazingaTaskManager, archiveOlderThan, maxArchiveInProgress);
    }

    @Bean
    public ArchiveCron archiveCron(CheckArchiveService archiveService) {
        return new ArchiveCron(archiveService, 10);
    }

    @Bean
    public ArchiveTask archiveTask(CheckArchiveService archiveService) {
        return new ArchiveTask(archiveService);
    }

    @Bean
    public MuteDigestService muteDigestService(
            CiStorageDb db, BadgeEventsSender badgeEventsSender
    ) {
        return new MuteDigestService(db, badgeEventsSender);
    }

    @Bean
    public MuteDigestCron muteDigestCron(
            MuteDigestService muteDigestService,
            @Value("${storage.muteDigestCron.period:24h}") Duration period,
            @Value("${storage.muteDigestCron.batchSize:100000}") int batchSize
    ) {
        return new MuteDigestCron(muteDigestService, period, batchSize);
    }
}
