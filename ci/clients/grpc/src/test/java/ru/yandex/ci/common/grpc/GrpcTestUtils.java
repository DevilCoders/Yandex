package ru.yandex.ci.common.grpc;

import java.io.IOException;
import java.util.List;

import io.grpc.BindableService;
import io.grpc.ManagedChannel;
import io.grpc.Server;
import io.grpc.inprocess.InProcessChannelBuilder;
import io.grpc.inprocess.InProcessServerBuilder;
import org.assertj.core.util.CanIgnoreReturnValue;
import org.mockito.Mockito;

import ru.yandex.ci.client.blackbox.BlackboxClient;
import ru.yandex.ci.client.tvm.grpc.AuthSettings;
import ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor;
import ru.yandex.passport.tvmauth.TvmClient;

public class GrpcTestUtils {
    private GrpcTestUtils() {
    }

    public static ManagedChannel buildChannel(BindableService service) {
        String serverName = InProcessServerBuilder.generateName();
        createAndStartServer(service, serverName);
        return InProcessChannelBuilder.forName(serverName).directExecutor().build();
    }

    @Deprecated // DO NOT USE THIS
    @CanIgnoreReturnValue
    private static Server createAndStartServer(BindableService service, String serverName) {
        Server server = InProcessServerBuilder.forName(serverName)
                .addService(service)
                .intercept(ExceptionInterceptor.instance())
                .intercept(new YandexAuthInterceptor(AuthSettings.builder()
                        .tvmClient(Mockito.mock(TvmClient.class))
                        .blackbox(Mockito.mock(BlackboxClient.class))
                        .debug(true)
                        .build()))
                .build();

        try {
            server.start();
        } catch (IOException e) {
            throw new RuntimeException("Failed to start", e);
        }
        return server;
    }

    public static Server createAndStartServer(String serverName, List<BindableService> services) {
        var serverBuilder = InProcessServerBuilder.forName(serverName)
                .intercept(ExceptionInterceptor.instance());
        for (var service : services) {
            serverBuilder.addService(service);
        }
        var server = serverBuilder.build();
        try {
            server.start();
        } catch (IOException e) {
            throw new RuntimeException("Failed to start", e);
        }
        return server;
    }

}
