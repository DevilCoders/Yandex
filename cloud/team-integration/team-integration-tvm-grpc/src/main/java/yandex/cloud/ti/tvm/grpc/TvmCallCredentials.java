package yandex.cloud.ti.tvm.grpc;

import java.util.concurrent.Executor;

import io.grpc.CallCredentials;
import io.grpc.Metadata;
import io.grpc.Status;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.GrpcHeaders;
import yandex.cloud.iam.client.tvm.TvmClient;

public class TvmCallCredentials extends CallCredentials {

    private final @NotNull TvmClient tvmClient;
    private final int tvmClientId;


    public TvmCallCredentials(
            @NotNull TvmClient tvmClient,
            int tvmClientId
    ) {
        this.tvmClient = tvmClient;
        this.tvmClientId = tvmClientId;
    }


    @Override
    public void applyRequestMetadata(RequestInfo requestInfo, Executor appExecutor, MetadataApplier applier) {
        appExecutor.execute(() -> applyMetadata(applier));
    }

    private void applyMetadata(@NotNull MetadataApplier applier) {
        try {
            applier.apply(createMetadata());
        } catch (RuntimeException e) {
            applier.fail(Status.UNAUTHENTICATED.withCause(e));
        }
    }

    private @NotNull Metadata createMetadata() {
        Metadata metadata = new Metadata();
        metadata.put(GrpcHeaders.X_YA_SERVICE_TICKET, tvmClient.getServiceTicket(tvmClientId));
        return metadata;
    }


    @Override
    public void thisUsesUnstableApi() {
    }

}
