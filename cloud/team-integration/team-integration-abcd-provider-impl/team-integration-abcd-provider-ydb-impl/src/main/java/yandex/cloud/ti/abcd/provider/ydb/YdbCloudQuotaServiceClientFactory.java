package yandex.cloud.ti.abcd.provider.ydb;

import java.util.List;
import java.util.function.Supplier;

import io.grpc.CallCredentials;
import io.grpc.Channel;
import io.grpc.ManagedChannelBuilder;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.client.ClientConfig;
import yandex.cloud.grpc.client.InProcessClientConfig;
import yandex.cloud.grpc.client.retry.MethodName;
import yandex.cloud.priv.ydb.v1.QuotaServiceGrpc;
import yandex.cloud.ti.abcd.provider.CloudQuotaServiceClient;
import yandex.cloud.ti.abcd.provider.CloudQuotaServiceClientFactory;
import yandex.cloud.ti.grpc.ChannelHelper;
import yandex.cloud.ti.grpc.IdempotencyKeyInterceptor;
import yandex.cloud.ti.grpc.TokenCallCredentials;

public class YdbCloudQuotaServiceClientFactory implements CloudQuotaServiceClientFactory {

    @Override
    public @NotNull String getQuotaServiceName() {
        return QuotaServiceGrpc.SERVICE_NAME;
    }

    @Override
    public @NotNull CloudQuotaServiceClient createCloudQuotaServiceClient(
            @NotNull String applicationName,
            @NotNull ClientConfig clientConfig,
            @NotNull Supplier<String> tokenSupplier
    ) {
        return create(applicationName, clientConfig, tokenSupplier);
    }


    public static YdbCloudQuotaServiceClientImpl create(
            @NotNull String applicationName,
            @NotNull ClientConfig clientConfig,
            @NotNull Supplier<String> tokenSupplier
    ) {
        return createYdbCloudQuotaServiceClient(
                applicationName,
                ChannelHelper.createNettyChannelBuilder(clientConfig),
                new TokenCallCredentials(tokenSupplier)
        );
    }

    public static YdbCloudQuotaServiceClientImpl create(
            @NotNull String applicationName,
            @NotNull InProcessClientConfig inProcessClientConfig,
            @NotNull Supplier<String> tokenSupplier
    ) {
        return createYdbCloudQuotaServiceClient(
                applicationName,
                ChannelHelper.createInProcessChannelBuilder(inProcessClientConfig.getChannelName()),
                new TokenCallCredentials(tokenSupplier)
        );
    }

    private static @NotNull YdbCloudQuotaServiceClientImpl createYdbCloudQuotaServiceClient(
            @NotNull String applicationName,
            @NotNull ManagedChannelBuilder<?> channelBuilder,
            @NotNull CallCredentials callCredentials
    ) {
        channelBuilder = ChannelHelper.configure(
                channelBuilder,
                applicationName,
                "ydb-cp-client",
                new IdempotencyKeyInterceptor()
        );
        channelBuilder = ChannelHelper.configureDefaultRetries(channelBuilder, List.of(
                MethodName.builder()
                        .service(QuotaServiceGrpc.SERVICE_NAME)
                        .build()
        ));
        // todo default deadline interceptor
        Channel channel = channelBuilder.build();

        QuotaServiceGrpc.QuotaServiceBlockingStub stub = QuotaServiceGrpc.newBlockingStub(channel)
                .withCallCredentials(callCredentials);

        return new YdbCloudQuotaServiceClientImpl(stub);
    }

}
