package ru.yandex.ci.tms.spring.bazinga;

import java.util.Optional;

import javax.annotation.Nullable;

import io.micrometer.core.instrument.MeterRegistry;
import lombok.extern.slf4j.Slf4j;
import org.joda.time.Duration;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.DependsOn;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.bolts.collection.Cf;
import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.S3LogStorage;
import ru.yandex.ci.common.bazinga.monitoring.BazingaExecutionMetrics;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.common.bazinga.spring.S3BazingaLoggerConfig;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.flow.spring.ZkConfig;
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
import ru.yandex.commune.bazinga.impl.controller.ControllerTaskManager;
import ru.yandex.commune.bazinga.impl.controller.ControllerTaskOptions;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;
import ru.yandex.commune.bazinga.impl.worker.BazingaHostPort;
import ru.yandex.commune.bazinga.impl.worker.BazingaWorkerAvailableMemoryProvider;
import ru.yandex.commune.bazinga.impl.worker.BazingaWorkerConfiguration;
import ru.yandex.commune.bazinga.impl.worker.TaskExecutorMetrics;
import ru.yandex.commune.bazinga.impl.worker.WorkerLogConfiguration;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskLoggerSource;
import ru.yandex.commune.bazinga.scheduler.TaskCategory;
import ru.yandex.commune.zk2.ZkPath;
import ru.yandex.commune.zk2.client.ZkManager;
import ru.yandex.commune.zk2.primitives.observer.ZkPathObserver;
import ru.yandex.misc.dataSize.DataSize;
import ru.yandex.misc.io.exec.ProcessUtils;
import ru.yandex.misc.io.file.File2;
import ru.yandex.misc.ip.InternetDomainName;
import ru.yandex.misc.ip.IpPort;

@Slf4j
@Configuration
@Import({
        CommonConfig.class,
        BazingaCoreConfig.class,
        ZkConfig.class,
        S3BazingaLoggerConfig.class
})
public class BazingaServiceConfig {
    static {
        ControllerTaskManager.useReadyStatusToSelectOnetimeJobs = true;
    }

    // SHARED OPTIONS

    @Bean
    public WorkerLogConfiguration workerLogConfiguration(
            @Value("${ci.workerLogConfiguration.logDir}") String logDir,
            @Value("${ci.workerLogConfiguration.onetimeLogRotation}") java.time.Duration onetimeLogRotation
    ) {
        return WorkerLogConfiguration.onetime(
                new File2(logDir),
                DataSize.fromGigaBytes(1),
                1,
                Duration.standardSeconds(onetimeLogRotation.toSeconds())
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
            @Value("${ci.bazingaWorkerConfiguration.threadCount}") int threadCount,
            @Value("${ci.bazingaWorkerConfiguration.queueSize}") int queueSize
    ) {
        log.info("Using task logger source: {}", taskLoggerSource);
        return new BazingaWorkerConfiguration(
                Cf.list(TaskCategory.DEFAULT),
                workerLogConfiguration,
                Option.empty(),
                Cf.toList(BazingaWorkerConfiguration.getQueue(threadCount, 0, queueSize, 0)),
                BazingaWorkerAvailableMemoryProvider.DEFAULT,
                ProcessUtils::destroySafe,
                api -> api,
                Cf.map(),
                taskLoggerSource
        );
    }

    @Bean
    public TaskExecutorMetrics taskExecutorMetrics(MeterRegistry meterRegistry) {
        return new BazingaExecutionMetrics(meterRegistry);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public DynamicConfigurationTypedServer typedServer(@Value("${ci.typedServer.bazingaPort}") int bazingaPort) {
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
    public BazingaControllerAndWorkerApps bazingaControllerAndWorkerApps(
            @Nullable S3LogStorage s3LogStorage,
            BazingaStorage bazingaStorage,
            BazingaConfiguration bazingaConfiguration,
            BazingaControllerConfiguration bazingaControllerConfiguration,
            BazingaWorkerConfiguration bazingaWorkerConfiguration,
            DynamicConfigurationTypedServer typedServer,
            ZkManager zkManager,
            ZkPathObserver zkPathObserver,
            TaskExecutorMetrics taskExecutorMetrics,
            @Value("${ci.bazingaControllerAndWorkerApps.controllerForceFetchCount}") int controllerForceFetchCount,
            @Value("${ci.bazingaControllerAndWorkerApps.minTaskFetchesBeforeDeadline}") int minTaskFetchesBeforeDeadline
    ) {
        //TODO add TaskExecutorMetrics to sensors
        var controllerOptions = new ControllerTaskOptions(
                true, true, controllerForceFetchCount, minTaskFetchesBeforeDeadline
        );
        var controllerAndWorker = BazingaConfigurator.controllerAndWorker(
                bazingaStorage,
                bazingaConfiguration,
                bazingaControllerConfiguration,
                bazingaWorkerConfiguration,
                typedServer,
                controllerOptions,
                zkManager,
                zkPathObserver,
                Optional.of(taskExecutorMetrics)
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
            @Value("${ci.bazingaAdminAddressResolver.httpPort}") int httpPort
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
