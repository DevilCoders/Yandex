package ru.yandex.ci.client.tvm.grpc;

import io.grpc.Attributes;
import io.grpc.ForwardingServerCall;
import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;

public class CallAttributesInterceptor implements ServerInterceptor {

    private Attributes.Builder attributeOverrides = Attributes.newBuilder();

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(ServerCall<ReqT, RespT> call,
                                                                 Metadata headers,
                                                                 ServerCallHandler<ReqT, RespT> next) {
        var proxiedCall = new ForwardingServerCall.SimpleForwardingServerCall<>(call) {
            @Override
            public Attributes getAttributes() {
                Attributes original = super.getAttributes();
                return Attributes.newBuilder()
                        .setAll(original)
                        .setAll(attributeOverrides.build())
                        .build();
            }
        };
        return next.startCall(proxiedCall, headers);
    }

    public <T> void set(Attributes.Key<T> key, T value) {
        attributeOverrides.set(key, value);
    }

    public void reset() {
        attributeOverrides = Attributes.newBuilder();
    }
}
