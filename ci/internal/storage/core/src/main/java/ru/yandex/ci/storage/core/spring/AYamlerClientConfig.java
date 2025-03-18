package ru.yandex.ci.storage.core.spring;

import java.time.Duration;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.ThreadFactory;

import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.jvm.ExecutorServiceMetrics;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;
import org.springframework.core.io.Resource;
import org.springframework.scheduling.concurrent.ThreadPoolTaskExecutor;

import ru.yandex.ci.client.ayamler.AYamlerClient;
import ru.yandex.ci.client.ayamler.AYamlerClientImpl;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.client.tvm.grpc.TvmCallCredentials;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        CommonConfig.class,
        TvmClientConfig.class
})
@Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
public class AYamlerClientConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Bean
    public GrpcClientProperties aYamlerClientProperties(
            @Value("${storage.aYamlerClientProperties.endpoint}") String endpoint,
            @Value("${storage.aYamlerClientProperties.userAgent}") String userAgent,
            @Value("${storage.aYamlerClientProperties.connectTimeout}") Duration connectTimeout,
            @Value("${storage.aYamlerClientProperties.deadlineAfter}") Duration deadlineAfter,
            @Value("${storage.aYamlerClientProperties.maxRetryAttempts}") int maxRetryAttempts,
            @Value("classpath:ayamler-retry-service-config.json") Resource serviceConfigJson,
            TvmClient tvmClient,
            TvmTargetClientId aYamlerTvmClientId
    ) {
        return GrpcClientProperties.builder()
                .endpoint(endpoint)
                .userAgent(userAgent)
                .connectTimeout(connectTimeout)
                .deadlineAfter(deadlineAfter)
                .maxRetryAttempts(maxRetryAttempts)
                .grpcServiceConfig(serviceConfigJson)
                .callCredentials(new TvmCallCredentials(tvmClient, aYamlerTvmClientId.getId()))
                .build();
    }

    @Bean
    public ThreadPoolTaskExecutor aYamlerExecutor(GrpcClientProperties aYamlerClientProperties) {
        var executor = new ThreadPoolTaskExecutor() {
            @SuppressWarnings("NullableProblems")
            @Override
            protected ExecutorService initializeExecutor(ThreadFactory threadFactory,
                                                         RejectedExecutionHandler rejectedExecutionHandler) {
                var executorService = super.initializeExecutor(threadFactory, rejectedExecutionHandler);
                return ExecutorServiceMetrics.monitor(meterRegistry, executorService, "ayamler_executor");
            }
        };
        executor.setCorePoolSize(1);
        executor.setMaxPoolSize(4);
        executor.setThreadNamePrefix("ayamler-");
        executor.setWaitForTasksToCompleteOnShutdown(true);
        executor.setAwaitTerminationMillis(aYamlerClientProperties.getAwaitTermination().toMillis());
        return executor;
    }

    @Bean
    public AYamlerClient aYamlerClient(
            GrpcClientProperties aYamlerClientProperties,
            ThreadPoolTaskExecutor aYamlerExecutor
    ) {
        return AYamlerClientImpl.create(aYamlerClientProperties.toBuilder()
                .executor(aYamlerExecutor)
                .build()
        );
    }

    @Bean
    public TvmTargetClientId aYamlerTvmClientId(@Value("${storage.aYamlerTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }

}
