package yandex.cloud.ti.grpc;

import java.util.UUID;

import io.grpc.CallOptions;
import io.grpc.Channel;
import io.grpc.ClientCall;
import io.grpc.ClientInterceptor;
import io.grpc.ForwardingClientCall;
import io.grpc.Metadata;
import io.grpc.MethodDescriptor;
import io.grpc.stub.AbstractStub;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.grpc.GrpcHeaders;

public class IdempotencyKeyInterceptor implements ClientInterceptor {

    public static final @NotNull CallOptions.Key<String> IDEMPOTENCY_KEY_OPTION_KEY = CallOptions.Key.create("idempotency-key");


    @Override
    public <ReqT, RespT> ClientCall<ReqT, RespT> interceptCall(
            MethodDescriptor<ReqT, RespT> method,
            CallOptions callOptions,
            Channel next
    ) {
        return new ForwardingClientCall.SimpleForwardingClientCall<>(next.newCall(method, callOptions)) {
            @Override
            public void start(ClientCall.Listener<RespT> responseListener, Metadata headers) {
                String idempotencyKey = callOptions.getOption(IDEMPOTENCY_KEY_OPTION_KEY);
                if (idempotencyKey != null) {
                    headers.put(GrpcHeaders.IDEMPOTENCY_KEY, idempotencyKey);
                }
                super.start(responseListener, headers);
            }
        };
    }


    public static <S extends AbstractStub<S>> @NotNull S withIdempotencyKey(@NotNull S stub) {
        return withIdempotencyKey(stub, UUID.randomUUID().toString());
    }

    public static <S extends AbstractStub<S>> @NotNull S withIdempotencyKey(@NotNull S stub, @Nullable String idempotencyKey) {
        if (idempotencyKey == null) {
            return stub;
        }
        return stub.withOption(IDEMPOTENCY_KEY_OPTION_KEY, idempotencyKey);
    }

}
