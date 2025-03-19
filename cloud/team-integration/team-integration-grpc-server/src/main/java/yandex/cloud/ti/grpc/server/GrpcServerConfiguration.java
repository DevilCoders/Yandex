package yandex.cloud.ti.grpc.server;

import java.util.Collection;
import java.util.List;

import io.grpc.BindableService;
import io.grpc.Server;
import io.grpc.ServerInterceptor;
import io.grpc.ServerTransportFilter;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;
import yandex.cloud.grpc.HeadersInterceptor;
import yandex.cloud.grpc.LoggingInterceptor;
import yandex.cloud.grpc.MetricsInterceptor;
import yandex.cloud.grpc.ValidationInterceptor;
import yandex.cloud.grpc.server.GrpcServerConfig;
import yandex.cloud.grpc.server.GrpcServerFactory;
import yandex.cloud.iam.client.tvm.grpc.TvmAuthenticationInterceptor;
import yandex.cloud.log.RidInterceptor;

public class GrpcServerConfiguration extends Configuration {

    private final @NotNull String applicationName;
    private final @NotNull Collection<Class<? extends BindableService>> serviceClasses;
    private final @NotNull Collection<ServerTransportFilter> serverTransportFilters;


    public GrpcServerConfiguration(
            @NotNull String applicationName,
            @NotNull Collection<Class<? extends BindableService>> serviceClasses,
            @NotNull Collection<ServerTransportFilter> serverTransportFilters
    ) {
        this.applicationName = applicationName;
        this.serviceClasses = List.copyOf(serviceClasses);
        this.serverTransportFilters = List.copyOf(serverTransportFilters);
    }


    @Override
    protected void configure() {
        put(Server.class, this::grpcServer);
    }

    protected @NotNull Server grpcServer() {
        GrpcServerFactory grpcServerFactory = GrpcServerFactory
                .newInstance(
                        applicationName,
                        grpcServerConfig(),
                        serverInterceptors(),
                        services()
                );
        for (ServerTransportFilter serverTransportFilter : serverTransportFilters) {
            grpcServerFactory.addTransportFilter(serverTransportFilter);
        }
        return grpcServerFactory.create();
    }

    protected @NotNull GrpcServerConfig grpcServerConfig() {
        return get(GrpcServerConfig.class);
    }

    protected @NotNull List<ServerInterceptor> serverInterceptors() {
        return List.of(
                get(TvmAuthenticationInterceptor.class),
                new ValidationInterceptor(),
                LoggingInterceptor.server(applicationName),
                MetricsInterceptor.server(applicationName),
                RidInterceptor.server(),
                new HeadersInterceptor()
        );
    }

    protected @NotNull BindableService[] services() {
        return serviceClasses.stream()
                .map(this::get)
                .toArray(BindableService[]::new);
    }

}
