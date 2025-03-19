package yandex.cloud.ti.tvm.http;

import java.io.IOException;
import java.security.Principal;
import java.util.Set;

import javax.security.auth.Subject;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import lombok.Value;
import lombok.extern.log4j.Log4j2;
import org.eclipse.jetty.security.Authenticator;
import org.eclipse.jetty.security.DefaultUserIdentity;
import org.eclipse.jetty.security.UserAuthentication;
import org.eclipse.jetty.server.Authentication;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.http.Headers;
import yandex.cloud.iam.client.tvm.TvmClient;
import yandex.cloud.iam.client.tvm.config.TvmClientConfig;

import ru.yandex.passport.tvmauth.TicketStatus;

@Log4j2
public class TvmAuthenticator implements Authenticator {

    private static final String AUTH_METHOD_NAME = "TVM";
    private static final String[] TVM_ROLES = { "USER", "TVM_USER" };

    private @NotNull TvmClient tvmClient;
    private @NotNull TvmClientConfig tvmClientConfig;


    public TvmAuthenticator(
            @NotNull TvmClient tvmClient,
            @NotNull TvmClientConfig tvmClientConfig
    ) {
        this.tvmClient = tvmClient;
        this.tvmClientConfig = tvmClientConfig;
    }


    @Override
    public void setConfiguration(AuthConfiguration authConfiguration) {
    }

    @Override
    public String getAuthMethod() {
        return AUTH_METHOD_NAME;
    }

    @Override
    public void prepareRequest(ServletRequest servletRequest) {
    }

    @Override
    public Authentication validateRequest(ServletRequest request, ServletResponse response, boolean mandatory) {
        if (!mandatory) {
            return Authentication.NOT_CHECKED;
        }

        var httpServletRequest = (HttpServletRequest) request;
        var ticket = httpServletRequest.getHeader(Headers.X_YA_SERVICE_TICKET);
        if (ticket == null) {
            return sendUnauthorizedError(response, "TVM header " + Headers.X_YA_SERVICE_TICKET + " is missing");
        }

        var checkResult = tvmClient.checkServiceTicket(ticket);
        if (checkResult.getStatus() != TicketStatus.OK) {
            return sendUnauthorizedError(response, "TVM status: " + checkResult.getStatus());
        }

        var tvmCallee = checkResult.getSrc();
        if (tvmClientConfig != null &&
                tvmClientConfig.getAllowedClients() != null
                && !tvmClientConfig.getAllowedClients().containsKey(tvmCallee)) {
            return sendUnauthorizedError(response, "TVM client not accepted: " + tvmCallee);
        }

        log.trace("TVM callee: {}", tvmCallee);
        var principal = new SimplePrincipal(Integer.toString(tvmCallee));
        var subject = new Subject(true, Set.of(principal), Set.of(), Set.of());
        return new UserAuthentication(getAuthMethod(), new DefaultUserIdentity(subject, principal, TVM_ROLES));
    }

    private static Authentication sendUnauthorizedError(ServletResponse response, String message) {
        var httpServletResponse = (HttpServletResponse) response;
        log.debug(message);
        try {
            httpServletResponse.sendError(HttpServletResponse.SC_UNAUTHORIZED, message);
        } catch (IOException ex) {
            // The client may have already gone - just log it and forget
            log.warn("Failed to send error response", ex);
        }
        return Authentication.SEND_FAILURE;
    }

    @Override
    public boolean secureResponse(ServletRequest request, ServletResponse response, boolean mandatory, Authentication.User validatedUser) {
        return true;
    }

    @Value
    private static class SimplePrincipal implements Principal {

        String name;

    }

}
