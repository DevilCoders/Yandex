package ru.yandex.ci.storage.tms.spring.bazinga;

import java.util.Optional;

import javax.annotation.Nullable;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.joda.time.Duration;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.DependsOn;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.bolts.collection.ListF;
import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.S3LogStorage;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.common.bazinga.spring.S3BazingaLoggerConfig;
import ru.yandex.ci.storage.core.archive.ArchiveTask;
import ru.yandex.ci.storage.core.clickhouse.ExportToClickhouse;
import ru.yandex.ci.storage.core.yt.impl.YtExportTask;
import ru.yandex.ci.util.HostnameUtils;
import ru.yandex.commune.actor.typed.dynamic.DynamicConfigurationTypedServer;
import ru.yandex.commune.bazinga.BazingaConfiguration;
import ru.yandex.commune.bazinga.BazingaConfigurator;
import ru.yandex.commune.bazinga.BazingaControllerAndWorkerApps;
import ru.yandex.commune.bazinga.BazingaControllerApp;
import ru.yandex.commune.bazinga.BazingaWorkerApp;
import ru.yandex.commune.bazinga.admin.BazingaAdminAddressResolver;
import ru.yandex.commune.bazinga.context.BazingaWorkerTaskInitializer;
import ru.yandex.commune.bazinga.impl.controller.BazingaControllerConfiguration;
import ru.yandex.commune.bazinga.impl.controller.ControllerTaskOptions;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;
import ru.yandex.commune.bazinga.impl.worker.BazingaHostPort;
import ru.yandex.commune.bazinga.impl.worker.BazingaWorkerAvailableMemoryProvider;
import ru.yandex.commune.bazinga.impl.worker.BazingaWorkerConfiguration;
import ru.yandex.commune.bazinga.impl.worker.WorkerLogConfiguration;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskLoggerSource;
import ru.yandex.commune.bazinga.scheduler.TaskCategory;
import ru.yandex.commune.bazinga.scheduler.TaskQueue;
import ru.yandex.commune.zk2.ZkPath;
import ru.yandex.commune.zk2.client.ZkManager;
import ru.yandex.commune.zk2.primitives.observer.ZkPathObserver;
import ru.yandex.misc.dataSize.DataSize;
import ru.yandex.misc.io.exec.ProcessUtils;
import ru.yandex.misc.io.file.File2;
import ru.yandex.misc.ip.InternetDomainName;
import ru.yandex.misc.ip.IpPort;

@Configuration
@Import({
        StorageBazingaZkConfig.class,
        StorageZAdminConfig.class,
        BazingaCoreConfig.class,
        S3BazingaLoggerConfig.class
})
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class StorageBazingaServiceConfig {

    private static final Logger log = LogManager.getLogger();

    @Bean
    public WorkerLogConfiguration workerLogConfiguration(
            @Value("${storage.workerLogConfiguration.logDir}") String logDir,
            @Value("${storage.workerLogConfiguration.onetimeLogRotation}") java.time.Duration onetimeLogRotation
    ) {

        File2 bazingaLogDir = new File2(logDir);
        log.info("Bazinga log dir is: " + bazingaLogDir.getAbsolutePath());

        return WorkerLogConfiguration.onetime(
                bazingaLogDir,
                DataSize.fromGigaBytes(1),
                1,
                Duration.standardSeconds(onetimeLogRotation.toSeconds())
        );
    }

    @Bean
    public DynamicConfigurationTypedServer typedServer(@Value("${storage.typedServer.bazingaPort}") int bazingaPort) {
        return new DynamicConfigurationTypedServer(new IpPort(bazingaPort));
    }

    @Bean
    public BazingaConfiguration bazingaConfiguration(DynamicConfigurationTypedServer typedServer, ZkPath zkPath) {
        IpPort port = new IpPort(typedServer.getServer().getActualPort());

        return new BazingaConfiguration(
                zkPath,
                new InternetDomainName(HostnameUtils.getHostname()),
                port
        );
    }

    @Bean
    public BazingaControllerConfiguration bazingaControllerConfiguration() {
        return new BazingaControllerConfiguration(Duration.standardSeconds(1), Duration.standardMinutes(1));
    }

    @Bean
    public BazingaWorkerConfiguration bazingaWorkerConfiguration(
            WorkerLogConfiguration workerLogConfiguration,
            WorkerTaskLoggerSource taskLoggerSource,
            @Value("${storage.bazingaWorkerConfiguration.bazingaThreadCount}") int bazingaThreadCount,
            @Value("${storage.bazingaWorkerConfiguration.bazingaQueueSize}") int bazingaQueueSize,
            @Value("${storage.bazingaWorkerConfiguration.bazingaExportThreadCount}") int bazingaExportThreadCount,
            @Value("${storage.bazingaWorkerConfiguration.bazingaExportQueueSize}") int bazingaExportQueueSize,
            @Value("${storage.bazingaWorkerConfiguration.chExportThreadCount}") int chExportThreadCount,
            @Value("${storage.bazingaWorkerConfiguration.chExportQueueSize}") int chExportQueueSize,
            @Value("${storage.bazingaWorkerConfiguration.archiveThreadCount}") int archiveThreadCount,
            @Value("${storage.bazingaWorkerConfiguration.archiveQueueSize}") int archiveQueueSize
    ) {
        log.info("Using task logger source: {}", taskLoggerSource);

        ListF<TaskQueue> queues = Cf.arrayList();
        queues.addAll(BazingaWorkerConfiguration.getQueue(bazingaThreadCount, 0, bazingaQueueSize, 0));
        queues.add(new TaskQueue(YtExportTask.EXPORT_QUEUE, bazingaExportThreadCount, bazingaExportQueueSize));
        queues.add(new TaskQueue(ExportToClickhouse.EXPORT_QUEUE, chExportThreadCount, chExportQueueSize));
        queues.add(new TaskQueue(ArchiveTask.ARCHIVE_QUEUE, archiveThreadCount, archiveQueueSize));

        return new BazingaWorkerConfiguration(
                Cf.list(TaskCategory.DEFAULT),
                workerLogConfiguration,
                Option.empty(),
                queues,
                BazingaWorkerAvailableMemoryProvider.DEFAULT,
                ProcessUtils::destroySafe,
                api -> api,
                Cf.map(),
                taskLoggerSource
        );
    }

    @Bean
    public BazingaControllerAndWorkerApps bazingaControllerAndWorkerApps(
            @Nullable S3LogStorage s3LogStorage,
            BazingaStorage bazingaStorage,
            BazingaConfiguration bazingaConfiguration,
            BazingaControllerConfiguration bazingaControllerConfiguration,
            BazingaWorkerConfiguration bazingaWorkerConfiguration,
            DynamicConfigurationTypedServer dynamicConfigurationTypedServer,
            ZkManager zkManager,
            ZkPathObserver zkPathObserver

    ) {
        var controllerAndWorker = BazingaConfigurator.controllerAndWorker(
                bazingaStorage,
                bazingaConfiguration,
                bazingaControllerConfiguration,
                bazingaWorkerConfiguration,
                dynamicConfigurationTypedServer,
                ControllerTaskOptions.DEFAULT,
                zkManager,
                zkPathObserver,
                Optional.empty()
        );

        if (s3LogStorage != null) {
            log.info("Configuring S3 log storage with current task registry");
            var registry = controllerAndWorker.getWorkerApp().getWorkerTaskRegistry();
            s3LogStorage.setOnetimeTaskChecker(taskId -> registry.getOnetimeTaskO(taskId).toOptional());
        }

        return controllerAndWorker;
    }

    @Bean(initMethod = "start", destroyMethod = "close")
    public BazingaControllerApp controllerApp(BazingaControllerAndWorkerApps bazingaControllerAndWorkerApps) {
        return bazingaControllerAndWorkerApps.getControllerApp();
    }

    @Bean
    public BazingaWorkerTaskInitializer bazingaWorkerTaskInitializer(
            BazingaControllerAndWorkerApps bazingaControllerAndWorkerApps,
            ApplicationContext applicationContext
    ) {
        return new BazingaWorkerTaskInitializer(
                applicationContext,
                bazingaControllerAndWorkerApps.getWorkerApp().getWorkerTaskRegistry()
        );
    }

    @DependsOn("bazingaWorkerTaskInitializer")
    @Bean(initMethod = "start", destroyMethod = "close")
    public BazingaWorkerApp workerApp(BazingaControllerAndWorkerApps bazingaControllerAndWorkerApps) {
        return bazingaControllerAndWorkerApps.getWorkerApp();
    }

    @Bean
    public BazingaAdminAddressResolver bazingaAdminAddressResolver(
            @Value("${storage.bazingaAdminAddressResolver.httpPort}") int httpPort
    ) {
        return new BazingaAdminAddressResolver() {
            @Override
            public String zUrlForController(BazingaHostPort controllerHostPort) {
                return "http://" + controllerHostPort.getHost() + ":" + httpPort + "/z/";
            }

            @Override
            public String urlForLogs(BazingaHostPort controllerHostPort) {
                return zUrlForController(controllerHostPort) + "bazinga/";
            }
        };
    }

}
