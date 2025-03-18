package ru.yandex.ci.client.abc;

import java.io.Closeable;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Stream;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.HttpResponse;

import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

public class AbcTestServer implements Closeable {

    private static final ObjectMapper MAPPER = new ObjectMapper();

    private final ClientAndServer server = new ClientAndServer();

    {
        server.when(request("/api/v4/services/"))
                .respond(this::services);
    }

    private final Map<String, AbcServiceInfo> services = new ConcurrentHashMap<>();

    private HttpResponse services(HttpRequest request) {
        var slugs = request.getFirstQueryStringParameter("slug__in");

        var serviceList = slugs.isEmpty()
                ? List.copyOf(services.values())
                :
                Stream.of(slugs.split(","))
                        .map(services::get)
                        .filter(Objects::nonNull)
                        .toList();
        return response()
                .withBody(toJson(new AbcClient.AbcServicesResponse(serviceList, null)), StandardCharsets.UTF_8);
    }

    public void reset() {
        services.clear();
    }

    public void addServices(AbcServiceInfo... services) {
        for (var service : services) {
            this.services.put(service.getSlug(), service);
        }
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
