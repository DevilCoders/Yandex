package yandex.cloud.ti.rm.client;

import java.util.List;
import java.util.function.Supplier;

import io.grpc.CallCredentials;
import io.grpc.Channel;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.client.ClientConfig;
import yandex.cloud.grpc.client.retry.MethodName;
import yandex.cloud.priv.resourcemanager.v1.CloudServiceGrpc;
import yandex.cloud.priv.resourcemanager.v1.FolderServiceGrpc;
import yandex.cloud.priv.resourcemanager.v1.OperationServiceGrpc;
import yandex.cloud.ti.grpc.ChannelHelper;
import yandex.cloud.ti.grpc.IdempotencyKeyInterceptor;
import yandex.cloud.ti.grpc.TokenCallCredentials;

final class ResourceManagerClientFactory {

    static ResourceManagerClient createResourceManagerClient(
            @NotNull String applicationName,
            @NotNull ClientConfig clientConfig,
            @NotNull Supplier<String> tokenSupplier
    ) {
        return createResourceManagerClient(
                configureChannel(
                        applicationName,
                        ChannelHelper.createNettyChannelBuilder(clientConfig)
                ),
                tokenSupplier
        );
    }

    static @NotNull ResourceManagerClient createResourceManagerClient(
            @NotNull Channel channel,
            @NotNull Supplier<String> tokenSupplier
    ) {
        return createResourceManagerClient(
                channel,
                new TokenCallCredentials(tokenSupplier)
        );
    }

    private static @NotNull ResourceManagerClient createResourceManagerClient(
            @NotNull Channel channel,
            @NotNull CallCredentials callCredentials
    ) {
        return new ResourceManagerClientImpl(
                CloudServiceGrpc.newBlockingStub(channel)
                        .withCallCredentials(callCredentials),
                FolderServiceGrpc.newBlockingStub(channel)
                        .withCallCredentials(callCredentials),
                OperationServiceGrpc.newBlockingStub(channel)
                        .withCallCredentials(callCredentials)
        );
    }

    private static ManagedChannelBuilder<?> configureChannelBuilder(
            @NotNull String applicationName,
            @NotNull ManagedChannelBuilder<?> channelBuilder
    ) {
        channelBuilder = ChannelHelper.configure(
                channelBuilder,
                applicationName,
                "resource-manager",
                new IdempotencyKeyInterceptor()
        );
        channelBuilder = ChannelHelper.configureDefaultRetries(channelBuilder, List.of(
                MethodName.builder()
                        .service(CloudServiceGrpc.SERVICE_NAME)
                        .build(),
                MethodName.builder()
                        .service(FolderServiceGrpc.SERVICE_NAME)
                        .build(),
                MethodName.builder()
                        .service(OperationServiceGrpc.SERVICE_NAME)
                        .build()
        ));
        return channelBuilder;
    }

    static ManagedChannel configureChannel(
            @NotNull String applicationName,
            @NotNull ManagedChannelBuilder<?> channelBuilder
    ) {
        return configureChannelBuilder(applicationName, channelBuilder).build();
    }


    private ResourceManagerClientFactory() {
    }

}
