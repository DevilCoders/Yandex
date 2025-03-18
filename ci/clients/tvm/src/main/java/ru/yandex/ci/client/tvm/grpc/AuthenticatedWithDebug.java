package ru.yandex.ci.client.tvm.grpc;

import java.util.Optional;

import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import static ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor.UID_KEY;

@Slf4j
@RequiredArgsConstructor
public class AuthenticatedWithDebug implements ServerInterceptor {

    public static final Metadata.Key<String> DEBUG_AUTH_USER_HEADER =
            Metadata.Key.of("user", Metadata.ASCII_STRING_MARSHALLER);

    private static final String DEFAULT_DEBUG_USER_NAME = "user42";

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next) {
        if (UID_KEY.get() != null) {
            return next.startCall(call, headers);
        }

        var debugUserName = Optional
                .ofNullable(headers.get(DEBUG_AUTH_USER_HEADER))
                .orElse(DEFAULT_DEBUG_USER_NAME);

        var user = new AuthenticatedUser("debug", debugUserName, "localhost");
        log.info("Authenticated by DEBUG user: {}", user);
        return YandexAuthInterceptor.withUser(call, headers, next, user);
    }
}
