package ru.yandex.ci.api.misc;

import io.grpc.Status;

import ru.yandex.ci.client.tvm.grpc.AuthenticatedUser;
import ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor;

public class AuthUtils {
    private AuthUtils() {
    }

    public static AuthenticatedUser getUser() {
        return YandexAuthInterceptor.getAuthenticatedUser()
                .orElseThrow(Status.UNAUTHENTICATED::asRuntimeException);
    }

    public static String getUsername() {
        return getUser().getLogin();
    }
}
