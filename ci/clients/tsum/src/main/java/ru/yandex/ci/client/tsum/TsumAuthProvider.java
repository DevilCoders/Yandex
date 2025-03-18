package ru.yandex.ci.client.tsum;

import okhttp3.Request;

import ru.yandex.ci.client.base.http.AuthProvider;

public class TsumAuthProvider implements AuthProvider {
    private static final String AUTH_HEADER_NAME = "Authorization";

    private final String token;

    public TsumAuthProvider(String token) {
        this.token = token;
    }

    @Override
    public void addAuth(Request.Builder request) {
        request.header(AUTH_HEADER_NAME, token);
    }
}
