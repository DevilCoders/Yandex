package ru.yandex.ci.common.grpc;

public class GrpcClientPropertiesStub {

    private GrpcClientPropertiesStub() {
    }

    public static GrpcClientProperties of(String channelName) {
        return GrpcClientProperties.ofEndpoint(GrpcClientProperties.INPROCESS_PREFIX + channelName).toBuilder()
                .userAgent("inprocess")
                .maxRetryAttempts(1)
                .build();
    }

    public static GrpcClientProperties of(String channelName, GrpcCleanupExtension grpcCleanupExtension) {
        return of(channelName).toBuilder()
                .listener(grpcCleanupExtension::register)
                .build();
    }


}
