package ru.yandex.ci.client.abc;

import java.io.Closeable;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.HttpResponse;

import ru.yandex.ci.client.abc.AbcTableClient.LoginAndSlug;

import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

public class AbcTableTestServer implements Closeable {

    private static final ObjectMapper MAPPER = new ObjectMapper();

    private final ClientAndServer server = new ClientAndServer();

    {
        server.when(request("/api/v3/select_rows"))
                .respond(this::services);
    }

    private final Set<LoginAndSlug> mapping = ConcurrentHashMap.newKeySet();

    private HttpResponse services(HttpRequest request) {
        var body = mapping.stream()
                .map(AbcTableTestServer::toJson)
                .collect(Collectors.joining("\n"));
        return response()
                .withBody(body, StandardCharsets.UTF_8);
    }

    public void reset() {
        mapping.clear();
    }

    public void addMap(String login, String slug) {
        mapping.add(new LoginAndSlug(login, slug));
    }

    public ClientAndServer getServer() {
        return server;
    }

    @Override
    public void close() {
        server.close();
    }

    private static String toJson(Object obj) {
        try {
            return MAPPER.writeValueAsString(obj);
        } catch (JsonProcessingException e) {
            throw new UncheckedIOException(e);
        }
    }
}
