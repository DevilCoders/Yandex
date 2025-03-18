package ru.yandex.ci.api.spring;

import java.util.HashSet;
import java.util.function.Predicate;

import io.grpc.MethodDescriptor;
import io.grpc.Server;
import io.grpc.ServerBuilder;
import io.grpc.protobuf.services.ProtoReflectionService;
import io.prometheus.client.CollectorRegistry;
import me.dinowernli.grpc.prometheus.MonitoringServerInterceptor;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.api.controllers.admin.GraphDiscoveryAdminController;
import ru.yandex.ci.api.controllers.autocheck.AutocheckController;
import ru.yandex.ci.api.controllers.frontend.FlowController;
import ru.yandex.ci.api.controllers.frontend.OnCommitFlowController;
import ru.yandex.ci.api.controllers.frontend.ProjectController;
import ru.yandex.ci.api.controllers.frontend.ReleaseController;
import ru.yandex.ci.api.controllers.frontend.TimelineController;
import ru.yandex.ci.api.controllers.internal.InternalApiController;
import ru.yandex.ci.api.controllers.internal.SecurityApiController;
import ru.yandex.ci.api.controllers.storage.StorageFlowController;
import ru.yandex.ci.api.internal.InternalApiGrpc;
import ru.yandex.ci.api.internal.frontend.project.ProjectServiceGrpc;
import ru.yandex.ci.api.storage.StorageFlowServiceGrpc;
import ru.yandex.ci.autocheck.api.AutocheckServiceGrpc;
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
import ru.yandex.ci.util.SpringParseUtils;
import ru.yandex.passport.tvmauth.TvmClient;

@Configuration
@Import({
        AdminApiConfig.class,
        AutocheckApiConfig.class,
        FrontendApiConfig.class,
        SecurityApiConfig.class
})
public class CiApiGrpcConfig {

    @Bean
    public TvmTargetClientId blackboxTvmClientId(@Value("${ci.blackboxTvmClientId.tvmId}") int tvmId) {
        return new TvmTargetClientId(tvmId);
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public BlackboxClient blackbox(
            @Value("${ci.blackbox.host}") String host,
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
            TvmClient tvmClient,
            BlackboxClient blackbox,
            TvmTargetClientId blackboxTvmClientId,
            @Value("${ci.tvmAuthInterceptor.oAuthScope}") String oAuthScope,
            @Value("${ci.tvmAuthInterceptor.debugAuth}") boolean debugAuth
    ) {

        var allowMethodsWithoutUserTicket = new HashSet<MethodDescriptor<?, ?>>();
        allowMethodsWithoutUserTicket.add(ProjectServiceGrpc.getGetConfigStatesMethod());
        allowMethodsWithoutUserTicket.addAll(StorageFlowServiceGrpc.getServiceDescriptor().getMethods());

        Predicate<MethodDescriptor<?, ?>> userTicketRequired =
                method -> !allowMethodsWithoutUserTicket.contains(method);

        return new YandexAuthInterceptor(
                AuthSettings.builder()
                        .tvmClient(tvmClient)
                        .blackbox(blackbox)
                        .mandatoryUserTicket(userTicketRequired)
                        .mandatoryServiceTicket(AuthSettings.REQUIRED)
                        .oAuthScope(oAuthScope)
                        .ignoreAuthMethodsList(
                                AutocheckServiceGrpc.getGetFastTargetsMethod(),
                                AutocheckServiceGrpc.getGetAutocheckInfoMethod(),
                                InternalApiGrpc.getUpdateTaskletProgressMethod(),
                                InternalApiGrpc.getGetCommitsMethod()
                        )
                        .debug(debugAuth)
                        .build());
    }

    @Bean
    public ServerLoggingInterceptor loggingInterceptor() {
        return new ServerLoggingInterceptor(
                InternalApiGrpc.getPingMethod()
        );
    }

    @Bean
    public MonitoringServerInterceptor monitoringServerInterceptor(
            CollectorRegistry collectorRegistry,
            @Value("${ci.monitoringServerInterceptor.histogramBuckets}") String histogramBuckets) {
        return MonitoringServerInterceptor.create(me.dinowernli.grpc.prometheus.Configuration.allMetrics()
                .withCollectorRegistry(collectorRegistry)
                .withLatencyBuckets(SpringParseUtils.parseToDoubleArray(histogramBuckets)));
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public ServerBuilder<?> serverBuilder(
            @Value("${ci.serverBuilder.grpcPort}") int grpcPort,
            @Value("${ci.serverBuilder.maxInboundMessageSizeMb}") int maxInboundMessageSizeMb
    ) {
        return ServerBuilder.forPort(grpcPort)
                .maxInboundMessageSize(maxInboundMessageSizeMb * 1024 * 1024);
    }


    @Bean(initMethod = "start", destroyMethod = "shutdown")
    public Server server(
            ServerBuilder<?> serverBuilder,
            InternalApiController internalApiController,
            AutocheckController autocheckController,
            FlowController flowController,
            GraphDiscoveryAdminController graphDiscoveryAdminController,
            OnCommitFlowController onCommitFlowController,
            ProjectController projectController,
            ReleaseController releaseController,
            SecurityApiController securityApiController,
            TimelineController timelineController,
            StorageFlowController storageFlowController,
            MonitoringServerInterceptor monitoringServerInterceptor,
            ServerLoggingInterceptor loggingInterceptor,
            YandexAuthInterceptor yandexAuthInterceptor
    ) {
        return serverBuilder
                .addService(ProtoReflectionService.newInstance())
                .addService(autocheckController)
                .addService(flowController)
                .addService(graphDiscoveryAdminController)
                .addService(internalApiController)
                .addService(onCommitFlowController)
                .addService(projectController)
                .addService(releaseController)
                .addService(securityApiController)
                .addService(timelineController)
                .addService(storageFlowController)
                .intercept(ExceptionInterceptor.instance(CiExceptionsConverter.instance()))
                .intercept(yandexAuthInterceptor)
                .intercept(monitoringServerInterceptor)
                .intercept(loggingInterceptor)
                .intercept(ServerInfoInterceptor.instance())
                .build();
    }
}
