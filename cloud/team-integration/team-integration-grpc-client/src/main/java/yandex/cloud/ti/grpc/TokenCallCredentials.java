package yandex.cloud.ti.grpc;

import java.util.concurrent.Executor;
import java.util.function.Supplier;

import io.grpc.CallCredentials;
import io.grpc.Metadata;
import io.grpc.Status;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.grpc.GrpcHeaders;

public class TokenCallCredentials extends CallCredentials {

    private final @NotNull Supplier<String> tokenSupplier;


    public TokenCallCredentials(
            @NotNull Supplier<String> tokenSupplier
    ) {
        this.tokenSupplier = tokenSupplier;
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
        metadata.put(GrpcHeaders.AUTHORIZATION, "Bearer " + tokenSupplier.get());
        return metadata;
    }


    @Override
    public void thisUsesUnstableApi() {
    }

}
