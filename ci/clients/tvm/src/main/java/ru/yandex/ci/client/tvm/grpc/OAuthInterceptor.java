package ru.yandex.ci.client.tvm.grpc;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.blackbox.BlackboxClient;

@AllArgsConstructor
@Slf4j
public class OAuthInterceptor implements ServerInterceptor {

    private static final String TOKEN_START = "AQAD-";

    private final BlackboxClient blackbox;

    @Nullable
    private final String requiredScope;

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(
            ServerCall<ReqT, RespT> call,
            Metadata headers,
            ServerCallHandler<ReqT, RespT> next) {

        if (Strings.isNullOrEmpty(requiredScope)) {
            log.debug("OAuth not enabled");
            return next.startCall(call, headers);
        }

        var authHeader = headers.get(OAuthCallCredentials.AUTH_HEADER);
        if (authHeader == null) {
            log.debug("No OAuth header");
            return next.startCall(call, headers);
        }
        var token = extractToken(authHeader);
        var userIp = YandexAuthInterceptor.getUserIp(call);

        try {
            String login = getUserLogin(token, userIp);
            var user = new AuthenticatedUser(null, login, userIp);
            log.info("Authenticating user {} using OAuth.", user);
            return YandexAuthInterceptor.withUser(call, headers, next, user);
        } catch (Exception e) {
            log.info("Failed to auth user using OAuth.", e);
            return YandexAuthInterceptor.unAuthenticated(call, headers, e.getMessage());
        }
    }

    private String extractToken(String authHeader) {
        int startIndex = authHeader.indexOf(TOKEN_START);
        if (startIndex <= 0) {
            return authHeader;
        }
        return authHeader.substring(startIndex);
    }

    private String getUserLogin(String oauthToken, String userIp) {
        var response = blackbox.getOAuth(userIp, oauthToken);
        var login = response.getLogin();
        var oAuthInfo = response.getOauth();

        Preconditions.checkState(
                oAuthInfo.getScope().contains(requiredScope),
                "No required scope (%s) in token for user %s. Got scopes: %s",
                requiredScope, login, oAuthInfo.getScope()
        );

        return login;
    }
}
