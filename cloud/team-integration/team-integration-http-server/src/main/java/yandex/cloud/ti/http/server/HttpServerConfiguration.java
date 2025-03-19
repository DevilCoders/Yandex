package yandex.cloud.ti.http.server;

import java.util.Collection;
import java.util.List;

import javax.servlet.Filter;
import javax.servlet.Servlet;

import lombok.NonNull;
import org.eclipse.jetty.security.Authenticator;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.common.httpserver.HttpServer;
import yandex.cloud.common.httpserver.HttpServerConfig;
import yandex.cloud.di.Configuration;

public class HttpServerConfiguration extends Configuration {

    private final @NonNull List<Class<? extends Filter>> filters;
    private final @NonNull List<Class<? extends Servlet>> servlets;


    public HttpServerConfiguration(
            @NonNull Collection<Class<? extends Filter>> filters,
            @NonNull Collection<Class<? extends Servlet>> servlets
    ) {
        this.filters = List.copyOf(filters);
        this.servlets = List.copyOf(servlets);
    }


    @Override
    protected void configure() {
        put(HttpServer.class, this::httpServer);
    }

    protected @NotNull HttpServer httpServer() {
        return new HttpServer(
                getHttpServerConfig(),
                filters,
                servlets,
                getAuthenticator()
        );
    }

    protected @NotNull HttpServerConfig getHttpServerConfig() {
        return get(HttpServerConfig.class);
    }

    protected @Nullable Authenticator getAuthenticator() {
        return get(Authenticator.class);
    }

}
