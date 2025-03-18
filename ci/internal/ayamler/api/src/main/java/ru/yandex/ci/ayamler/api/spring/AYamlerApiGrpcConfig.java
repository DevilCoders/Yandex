package ru.yandex.ci.ayamler.api.spring;

import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import io.grpc.Server;
import io.grpc.internal.GrpcUtil;
import io.grpc.netty.NettyServerBuilder;
import io.grpc.protobuf.services.ProtoReflectionService;
import io.prometheus.client.CollectorRegistry;
import me.dinowernli.grpc.prometheus.MonitoringServerInterceptor;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.ayamler.api.controllers.AYamlerController;
import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.blackbox.BlackboxClient;
import ru.yandex.ci.client.blackbox.BlackboxClientImpl;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.client.tvm.grpc.AuthSettings;
import ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor;
import ru.yandex.ci.common.grpc.ExceptionInterceptor;
import ru.yandex.ci.common.grpc.ServerInfoInterceptor;
import ru.yandex.ci.common.grpc.ServerLoggingInterceptor;
import ru.yandex.ci.core.exceptions.CiExceptionsConverter;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        TvmClientConfig.class,
        AYamlerApiConfig.class,
})
public class AYamlerApiGrpcConfig {

    @Bean(initMethod = "start", destroyMethod = "shutdown")
    public Server aYamlerServer(
            @Value("${ayamler.aYamlerServer.threads}") int threads,
            @Value("${ayamler.aYamlerServer.grpcPort}") int grpcPort,
            @Value("${ayamler.aYamlerServer.maxInboundMessageSizeMb}") int maxInboundMessageSizeMb,
            YandexAuthInterceptor yandexAuthInterceptor,
            AYamlerController aYamlerController,
            MonitoringServerInterceptor monitoringServerInterceptor,
            ServerLoggingInterceptor loggingInterceptor
    ) {
        return NettyServerBuilder.forPort(grpcPort)
                .maxInboundMessageSize(maxInboundMessageSizeMb * 1024 * 1024)
                .executor(
                        Executors.newFixedThreadPool(
                                threads,
                                new ThreadFactoryBuilder()
                                        .setDaemon(true)
                                        .setNameFormat("ayamler-api-netty-%d")
                                        .build()
                        )
                )
                .addService(ProtoReflectionService.newInstance())
                .addService(aYamlerController)
                .keepAliveTimeout(GrpcUtil.DEFAULT_SERVER_KEEPALIVE_TIMEOUT_NANOS / 2, TimeUnit.SECONDS)
                .permitKeepAliveWithoutCalls(true)
                .intercept(ExceptionInterceptor.instance(CiExceptionsConverter.instance()))
                .intercept(yandexAuthInterceptor)
                .intercept(monitoringServerInterceptor)
                .intercept(loggingInterceptor)
                .intercept(ServerInfoInterceptor.instance())
                .build();
    }

    @Bean
    public TvmTargetClientId blackboxTvmClientId(@Value("${ayamler.blackboxTvmClientId.tvmId}") int tvmId) {
        // Need this for tvm authentication by service tickets
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    public BlackboxClient blackbox(
            @Value("${ayamler.blackbox.host}") String host,
            TvmClient tvmClient,
            TvmTargetClientId blackboxTvmClientId,
            CallsMonitorSource callsMonitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(host)
                .authProvider(new TvmAuthProvider(tvmClient, blackboxTvmClientId.getId()))
                .callsMonitorSource(callsMonitorSource)
                .build();
        return BlackboxClientImpl.create(properties);
    }

    @Bean
    public YandexAuthInterceptor tvmAuthInterceptor(
            @Value("${ayamler.tvmAuthInterceptor.debugAuth}") boolean debugAuth,
            TvmClient tvmClient,
            BlackboxClient blackboxClient
    ) {
        return new YandexAuthInterceptor(
                AuthSettings.builder()
                        .tvmClient(tvmClient)
                        .blackbox(blackboxClient)
                        .mandatoryUserTicket(AuthSettings.NOT_REQUIRED)
                        .mandatoryServiceTicket(AuthSettings.REQUIRED)
                        .debug(debugAuth)
                        .build());
    }

    @Bean
    public MonitoringServerInterceptor monitoringServerInterceptor(
            @Value("${ayamler.monitoringServerInterceptor.histogramBuckets}") double[] histogramBuckets,
            CollectorRegistry collectorRegistry
    ) {
        return MonitoringServerInterceptor.create(me.dinowernli.grpc.prometheus.Configuration.allMetrics()
                .withCollectorRegistry(collectorRegistry)
                .withLatencyBuckets(histogramBuckets));
    }

    @Bean
    public ServerLoggingInterceptor loggingInterceptor() {
        return new ServerLoggingInterceptor();
    }

}
