package ru.yandex.ci.storage.api.spring;

import java.net.InetSocketAddress;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.function.Predicate;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import io.grpc.MethodDescriptor;
import io.grpc.Server;
import io.grpc.ServerBuilder;
import io.grpc.netty.NettyServerBuilder;
import io.grpc.protobuf.services.ProtoReflectionService;
import io.prometheus.client.CollectorRegistry;
import me.dinowernli.grpc.prometheus.MonitoringServerInterceptor;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.blackbox.BlackboxClient;
import ru.yandex.ci.client.blackbox.BlackboxClientImpl;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.tvm.TvmTargetClientId;
import ru.yandex.ci.client.tvm.grpc.AuthSettings;
import ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.grpc.ExceptionInterceptor;
import ru.yandex.ci.common.grpc.ServerInfoInterceptor;
import ru.yandex.ci.common.grpc.ServerLoggingInterceptor;
import ru.yandex.ci.core.exceptions.CiExceptionsConverter;
import ru.yandex.ci.core.spring.clients.TvmClientConfig;
import ru.yandex.ci.storage.api.CommonPublicApiGrpc;
import ru.yandex.ci.storage.api.RawDataPublicApiGrpc;
import ru.yandex.ci.storage.api.SearchPublicApiGrpc;
import ru.yandex.ci.storage.api.StorageFrontApiServiceGrpc;
import ru.yandex.ci.storage.api.StorageProxyApiServiceGrpc;
import ru.yandex.ci.storage.api.controllers.StorageApiController;
import ru.yandex.ci.storage.api.controllers.StorageFrontApiController;
import ru.yandex.ci.storage.api.controllers.StorageFrontHistoryApiController;
import ru.yandex.ci.storage.api.controllers.StorageFrontTestsApiController;
import ru.yandex.ci.storage.api.controllers.StorageMaintenanceController;
import ru.yandex.ci.storage.api.controllers.StorageProxyApiController;
import ru.yandex.ci.storage.api.controllers.public_api.CommonPublicApiController;
import ru.yandex.ci.storage.api.controllers.public_api.RawDataPublicApiController;
import ru.yandex.ci.storage.api.controllers.public_api.SearchPublicApiController;
import ru.yandex.ci.util.SpringParseUtils;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        TvmClientConfig.class,
        StorageApiConfig.class
})
public class StorageApiGrpcConfig {
    private static final Set<String> PUBLIC_API = Set.of(
            StorageProxyApiServiceGrpc.SERVICE_NAME,
            CommonPublicApiGrpc.SERVICE_NAME,
            SearchPublicApiGrpc.SERVICE_NAME,
            RawDataPublicApiGrpc.SERVICE_NAME
    );

    @Autowired
    private CollectorRegistry collectorRegistry;

    @Bean
    public TvmTargetClientId blackboxTvmClientId(@Value("${storage.blackboxTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    public BlackboxClient blackbox(
            @Value("${storage.blackbox.blackboxHost}") String blackboxHost,
            TvmClient tvmClient,
            TvmTargetClientId blackboxTvmClientId,
            CallsMonitorSource callsMonitorSource
    ) {
        var properties = HttpClientProperties.builder()
                .endpoint(blackboxHost)
                .authProvider(new TvmAuthProvider(tvmClient, blackboxTvmClientId.getId()))
                .callsMonitorSource(callsMonitorSource)
                .build();
        return BlackboxClientImpl.create(properties);
    }

    @Bean
    public YandexAuthInterceptor tvmAuthInterceptor(
            TvmClient tvmClient,
            BlackboxClient blackbox,
            @Value("${storage.tvmAuthInterceptor.debugAuth}") boolean debugAuth
    ) {

        // Пока фронт не умеет передавать service-ticket-ы
        Predicate<MethodDescriptor<?, ?>> callByFrontend = method ->
                StorageFrontApiServiceGrpc.SERVICE_NAME.equals(method.getServiceName());

        // skip auth
        var publicApi = (Predicate<MethodDescriptor<?, ?>>) api -> PUBLIC_API.contains(api.getServiceName());

        return new YandexAuthInterceptor(
                AuthSettings.builder()
                        .tvmClient(tvmClient)
                        .blackbox(blackbox)
                        // все методы фронта ждут только user-ticket
                        .mandatoryUserTicket(method -> !publicApi.test(method) && callByFrontend.test(method))
                        // все методы бэкенда ждут только service-ticket
                        .mandatoryServiceTicket(method -> !publicApi.test(method) && !callByFrontend.test(method))
                        .debug(debugAuth)
                        .build());
    }

    @Bean
    public ServerLoggingInterceptor loggingInterceptor() {
        return new ServerLoggingInterceptor();
    }

    @Bean
    public MonitoringServerInterceptor monitoringServerInterceptor(
            @Value("${storage.monitoringServerInterceptor.histogramBuckets}") String histogramBuckets
    ) {
        return MonitoringServerInterceptor.create(me.dinowernli.grpc.prometheus.Configuration.allMetrics()
                .withCollectorRegistry(collectorRegistry)
                .withLatencyBuckets(SpringParseUtils.parseToDoubleArray(histogramBuckets)));
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ServerBuilder<?> storageServerBuilder(
            @Value("${storage.server.threads}") int threads,
            @Value("${storage.server.grpcPort}") int grpcPort,
            @Value("${storage.server.publicGrpcPort}") int pubicGrpcPort,
            @Value("${storage.server.maxInboundMessageSizeMb}") int maxInboundMessageSizeMb
    ) {
        return NettyServerBuilder.forPort(grpcPort)
                .addListenAddress(new InetSocketAddress(pubicGrpcPort))
                .maxInboundMessageSize(maxInboundMessageSizeMb * 1024 * 1024)
                .executor(
                        Executors.newFixedThreadPool(
                                threads,
                                new ThreadFactoryBuilder()
                                        .setDaemon(true)
                                        .setNameFormat("netty-%d")
                                        .build()
                        )
                );
    }

    @Bean(initMethod = "start", destroyMethod = "shutdown")
    public Server storageServer(
            ServerBuilder<?> storageServerBuilder,
            StorageApiController storageApiController,
            StorageFrontApiController storageFrontApiController,
            StorageFrontHistoryApiController historyApiController,
            CommonPublicApiController commonPublicApiController,
            SearchPublicApiController searchPublicApiController,
            RawDataPublicApiController rawDataPublicApiController,
            StorageMaintenanceController storageMaintenanceController,
            StorageProxyApiController storageProxyApiController,
            StorageFrontTestsApiController storageFrontTestsApiController,
            YandexAuthInterceptor yandexAuthInterceptor,
            MonitoringServerInterceptor monitoringServerInterceptor,
            ServerLoggingInterceptor loggingInterceptor
    ) {
        return storageServerBuilder
                .addService(ProtoReflectionService.newInstance())
                .addService(storageApiController)
                .addService(storageFrontApiController)
                .addService(storageMaintenanceController)
                .addService(storageProxyApiController)
                .addService(historyApiController)
                .addService(storageFrontTestsApiController)
                .addService(commonPublicApiController)
                .addService(searchPublicApiController)
                .addService(rawDataPublicApiController)
                .intercept(ExceptionInterceptor.instance(CiExceptionsConverter.instance()))
                .intercept(yandexAuthInterceptor)
                .intercept(monitoringServerInterceptor)
                .intercept(loggingInterceptor)
                .intercept(ServerInfoInterceptor.instance())
                .build();
    }

}
