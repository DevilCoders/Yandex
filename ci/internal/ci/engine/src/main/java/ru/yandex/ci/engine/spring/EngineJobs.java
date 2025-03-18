package ru.yandex.ci.engine.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.engine.autocheck.jobs.ci.RunCiActionJob;
import ru.yandex.ci.engine.autocheck.jobs.large.StartLargeTestsJob;
import ru.yandex.ci.engine.autocheck.jobs.misc.SleepJob;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.flow.spring.FlowEngineJobsConfig;

@Configuration
@Import({
        AutocheckConfig.class,
        AutocheckStressTestConfig.class,
        FlowEngineJobsConfig.class,
        PciExpressConfig.class
})
public class EngineJobs {

    @Bean
    public RunCiActionJob runCiActionJob(
            SecurityAccessService securityAccessService,
            @Value("${spring.profiles.active}") String ciEnvironment
    ) {
        return new RunCiActionJob(securityAccessService, ciEnvironment, null);
    }

    @Bean
    public StartLargeTestsJob startLargeTestsJob(StorageApiClient storageApiClient) {
        return new StartLargeTestsJob(storageApiClient);
    }

    @Bean
    public SleepJob sleepJob() {
        return new SleepJob();
    }

}
