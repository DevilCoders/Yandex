package ru.yandex.ci.common.bazinga.spring;

import java.time.Duration;
import java.util.concurrent.Executors;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.bazinga.BazingaCleanupService;
import ru.yandex.ci.common.bazinga.S3LogStorage;
import ru.yandex.ci.common.bazinga.S3WorkerTaskLoggerSource;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskLoggerSource;

@Configuration
@Import(S3LogStorageConfig.class)
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class S3BazingaLoggerConfig {

    @Bean
    public BazingaCleanupService bazingaCleanup(
            S3LogStorage s3LogStorage,
            @Value("${bazinga.bazingaCleanup.cleanupOlderThan}") Duration cleanupOlderThan
    ) {
        return new BazingaCleanupService(s3LogStorage, cleanupOlderThan);
    }


    @Bean(destroyMethod = "close")
    public WorkerTaskLoggerSource bazingaTaskLoggerS3(
            S3LogStorage s3LogStorage,
            @Value("${bazinga.bazingaTaskLoggerS3.uploadThreads}") int uploadThreads,
            @Value("${bazinga.bazingaTaskLoggerS3.immediateFlush}") boolean immediateFlush
    ) {
        return new S3WorkerTaskLoggerSource(s3LogStorage, Executors.newFixedThreadPool(uploadThreads), immediateFlush);
    }

}
