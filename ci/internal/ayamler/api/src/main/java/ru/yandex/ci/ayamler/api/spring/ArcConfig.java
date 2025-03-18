package ru.yandex.ci.ayamler.api.spring;

import java.time.Duration;
import java.util.concurrent.TimeUnit;

import com.netflix.concurrency.limits.Limit;
import com.netflix.concurrency.limits.grpc.client.ConcurrencyLimitClientInterceptor;
import com.netflix.concurrency.limits.grpc.client.GrpcClientLimiterBuilder;
import com.netflix.concurrency.limits.limit.AIMDLimit;
import com.netflix.concurrency.limits.limit.WindowedLimit;
import com.netflix.concurrency.limits.spectator.SpectatorMetricRegistry;
import com.netflix.spectator.api.Id;
import com.netflix.spectator.micrometer.MicrometerRegistry;
import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.core.io.Resource;

import ru.yandex.ci.client.tvm.grpc.OAuthCallCredentials;
import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientImpl;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceImpl;
import ru.yandex.ci.core.spring.CommonConfig;

@Configuration
@Import(CommonConfig.class)
public class ArcConfig {

    @Bean
    public ArcService arcService(
            MeterRegistry meterRegistry,
            GrpcClient arcServiceChannel,
            @Value("${ayamler.arcService.processChangesAndSkipNotFoundCommits}")
            boolean processChangesAndSkipNotFoundCommits
    ) {
        return new ArcServiceImpl(arcServiceChannel, meterRegistry, processChangesAndSkipNotFoundCommits);
    }

    @Bean
    public GrpcClient arcServiceChannel(
            MeterRegistry meterRegistry,
            @Value("${ayamler.arcServiceChannel.endpoint}") String endpoint,
            @Value("${ayamler.arcServiceChannel.token}") String token,
            @Value("${ayamler.arcServiceChannel.connectTimeout}") Duration connectTimeout,
            @Value("${ayamler.arcServiceChannel.deadlineAfter}") Duration deadlineAfter,
            @Value("classpath:arcServiceChannel.grpcServiceConfig.json") Resource grpcServiceConfig,
            Limit concurrencyArcRequestsLimit
    ) {
        var metricRegistry = new SpectatorMetricRegistry(
                new MicrometerRegistry(meterRegistry),
                Id.create("ayamler_arc_grpc_client")
        );

        var interceptor = new ConcurrencyLimitClientInterceptor(
                new GrpcClientLimiterBuilder()
                        .blockOnLimit(true)
                        .limit(concurrencyArcRequestsLimit)
                        .metricRegistry(metricRegistry)
                        .build()
        );

        var properties = GrpcClientProperties.builder()
                .endpoint(endpoint)
                .connectTimeout(connectTimeout)
                .deadlineAfter(deadlineAfter)
                .maxRetryAttempts(3)
                .grpcServiceConfig(grpcServiceConfig)
                .plainText(false)
                .callCredentials(new OAuthCallCredentials(token))
                .interceptor(interceptor)
                .build();

        return arcServiceChannelBuilder(properties);
    }

    @Bean
    public Limit concurrencyArcRequestsLimit() {
        var limit = AIMDLimit.newBuilder()
                .initialLimit(200)
                .minLimit(200)
                .maxLimit(500)
                .backoffRatio(0.8)
                .timeout(4, TimeUnit.SECONDS)
                .build();
        return WindowedLimit.newBuilder()
                .minWindowTime(100, TimeUnit.MILLISECONDS)
                .maxWindowTime(100, TimeUnit.MILLISECONDS)
                .build(limit);
    }

    public static GrpcClient arcServiceChannelBuilder(GrpcClientProperties clientProperties) {
        return GrpcClientImpl.builder(clientProperties, ArcServiceImpl.class)
                .withoutClientLogging()
                .build();
    }
}
