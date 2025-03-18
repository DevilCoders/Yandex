package ru.yandex.ci.client.tvm.grpc;

import java.util.concurrent.Executor;

import javax.annotation.Nonnull;

import io.grpc.CallCredentials;
import io.grpc.Metadata;

public class OAuthCallCredentials extends CallCredentials {

    public static final Metadata.Key<String> AUTH_HEADER = Metadata.Key.of(
            "authorization", Metadata.ASCII_STRING_MARSHALLER
    );

    private static final String OAUTH_PREFIX = "OAuth ";

    private final Metadata authHeaders;

    public OAuthCallCredentials(@Nonnull String token) {
        this(token, false);
    }

    public OAuthCallCredentials(@Nonnull String token, boolean addPrefix) {
        Metadata headers = new Metadata();
        headers.put(AUTH_HEADER, addPrefix ? OAUTH_PREFIX + token : token);
        authHeaders = headers;
    }

    @Override
    public void applyRequestMetadata(RequestInfo requestInfo, Executor appExecutor, MetadataApplier applier) {
        applier.apply(authHeaders);
    }

    @Override
    public void thisUsesUnstableApi() {
        //Очень жаль ¯\_(ツ)_/¯
    }
}
