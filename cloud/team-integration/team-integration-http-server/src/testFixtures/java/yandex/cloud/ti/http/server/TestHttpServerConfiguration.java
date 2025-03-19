package yandex.cloud.ti.http.server;

import java.util.Collection;

import javax.servlet.Filter;
import javax.servlet.Servlet;

import lombok.NonNull;
import org.eclipse.jetty.security.Authenticator;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.common.httpserver.HttpServerConfig;

public class TestHttpServerConfiguration extends HttpServerConfiguration {

    public TestHttpServerConfiguration(
            @NonNull Collection<Class<? extends Filter>> filters,
            @NonNull Collection<Class<? extends Servlet>> servlets
    ) {
        super(filters, servlets);
    }


    @Override
    protected @NotNull HttpServerConfig getHttpServerConfig() {
        return HttpServerConfig.create(0);
    }

    @Override
    protected @Nullable Authenticator getAuthenticator() {
        return null;
    }

}
