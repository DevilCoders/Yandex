package ru.yandex.ci.client.tvm.grpc;

import java.util.concurrent.Executor;

import javax.annotation.Nonnull;

import io.grpc.CallCredentials;
import io.grpc.Metadata;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.client.tvm.TvmHeaders;
import ru.yandex.passport.tvmauth.TvmClient;

@RequiredArgsConstructor
public class TvmCallCredentials extends CallCredentials {

    public static final Metadata.Key<String> TVM_SERVICE_TICKET =
            Metadata.Key.of(TvmHeaders.SERVICE_TICKET, Metadata.ASCII_STRING_MARSHALLER);

    @Nonnull
    private final TvmClient tvmClient;
    private final int destinationTvmId;

    @Override
    public void applyRequestMetadata(RequestInfo requestInfo, Executor appExecutor, MetadataApplier applier) {
        Metadata headers = new Metadata();
        headers.put(TVM_SERVICE_TICKET, tvmClient.getServiceTicketFor(destinationTvmId));

        applier.apply(headers);
    }

    @Override
    public void thisUsesUnstableApi() {
        //Очень жаль ¯\_(ツ)_/¯
    }

}
