package yandex.cloud.ti.yt.abcd.client;

import java.util.List;

import io.grpc.CallCredentials;
import io.grpc.Channel;
import io.grpc.ManagedChannelBuilder;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.client.retry.MethodName;
import yandex.cloud.iam.client.tvm.TvmClient;
import yandex.cloud.ti.grpc.ChannelHelper;
import yandex.cloud.ti.tvm.grpc.TvmCallCredentials;

import ru.yandex.intranet.d.backend.service.proto.FolderServiceGrpc;

final class FolderServiceGrpcClientFactory {

    static FolderServiceGrpcClient create(
            @NotNull String applicationName,
            @NotNull TeamAbcdClientProperties clientConfig,
            @NotNull TvmClient tvmClient
    ) {
        return createAbcFolderClient(
                applicationName,
                ChannelHelper.createNettyChannelBuilder(clientConfig.endpoint()),
                new TvmCallCredentials(tvmClient, clientConfig.tvmId())
        );
    }

    private static @NotNull FolderServiceGrpcClient createAbcFolderClient(
            @NotNull String applicationName,
            @NotNull ManagedChannelBuilder<?> channelBuilder,
            @NotNull CallCredentials callCredentials
    ) {
        channelBuilder = ChannelHelper.configure(
                channelBuilder,
                applicationName,
                "team-abcd-client"
        );
        channelBuilder = ChannelHelper.configureDefaultRetries(channelBuilder, List.of(
                MethodName.builder()
                        .service(FolderServiceGrpc.SERVICE_NAME)
                        .build()
        ));
        Channel channel = channelBuilder.build();
        // todo this channel and callCredentials should be used by other abcd grpc stubs
        //  retry config should know about all such stubs
        FolderServiceGrpc.FolderServiceBlockingStub stub = FolderServiceGrpc.newBlockingStub(channel)
                .withCallCredentials(callCredentials);

        // todo IDEMPOTENCY vs retries
        //  a call site can decide to add idempotency key with stub.withOption()
        //  option can generate the key value randomly, or use the provided one?
        //      provided - useful only if the method can be retried with exactly the same parameters by call site or business logic?

        return new FolderServiceGrpcClient(stub);
    }


    private FolderServiceGrpcClientFactory() {
    }

}
