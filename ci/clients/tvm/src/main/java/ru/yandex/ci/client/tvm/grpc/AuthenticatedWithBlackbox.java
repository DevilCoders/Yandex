package ru.yandex.ci.client.tvm.grpc;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.blackbox.BlackboxClient;

import static ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor.AUTHENTICATED_USER;
import static ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor.UID_KEY;
import static ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor.USER_TICKET;
import static ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor.unAuthenticated;
import static ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor.withUser;

@RequiredArgsConstructor
@Slf4j
public class AuthenticatedWithBlackbox implements ServerInterceptor {

    @Nonnull
    private final BlackboxClient blackbox;

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next
    ) {
        if (AUTHENTICATED_USER.get() != null) {
            return next.startCall(call, headers); // Пользователь уже идентифицирован
        }

        @Nullable
        var uid = UID_KEY.get();
        if (uid == null || uid <= 0) {
            return next.startCall(call, headers); // Идентификация не требуется
        }

        @Nullable
        var userTicket = USER_TICKET.get();
        var ipAddress = YandexAuthInterceptor.getUserIp(call);

        @Nullable
        var login = getUserLogin(uid, ipAddress);
        if (login == null) {
            return unAuthenticated(call, headers,
                    "Unable to find login for user " + uid + " and IpAddress " + ipAddress);
        }

        var user = new AuthenticatedUser(userTicket, login, ipAddress);
        log.info("Authenticated: {}", user);
        return withUser(call, headers, next, user);
    }


    @Nullable
    private String getUserLogin(long uuid, String userIpAddress) {
        return blackbox.getUserInfo(userIpAddress, uuid).getLogin();
    }
}
