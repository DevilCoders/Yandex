package yandex.cloud.ti.grpc.server;

import java.util.Collection;
import java.util.List;

import io.grpc.BindableService;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.server.GrpcServerConfig;

public class TestGrpcServerConfiguration extends GrpcServerConfiguration {

    public TestGrpcServerConfiguration(
            @NotNull String applicationName,
            @NotNull Collection<Class<? extends BindableService>> serviceClasses
    ) {
        super(
                applicationName,
                serviceClasses,
                List.of()
        );
    }


    @Override
    protected @NotNull GrpcServerConfig grpcServerConfig() {
        return GrpcServerConfig.forTesting();
    }

}
