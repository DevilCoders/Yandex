package ru.yandex.ci.storage.core.grpc;

import io.grpc.Metadata;

public class StorageTrailers {
    public static final Metadata.Key<String> ERROR = Metadata.Key.of("ci-error", Metadata.ASCII_STRING_MARSHALLER);

    private StorageTrailers() {

    }
}
