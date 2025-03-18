package ru.yandex.ci.client.base.http;

import javax.annotation.Nonnull;

import okhttp3.Request;

public class OAuthProvider implements AuthProvider {
    private static final String OAUTH_HEADER_NAME = "Authorization";

    private final String header;

    public OAuthProvider(@Nonnull String oAuthToken) {
        this.header = "OAuth " + oAuthToken;
    }

    @Override
    public void addAuth(Request.Builder request) {
        request.header(OAUTH_HEADER_NAME, header);
    }

    public String getHeader() {
        return header;
    }
}
