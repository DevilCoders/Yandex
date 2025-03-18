package ru.yandex.ci.client.base.http;

import org.mockserver.client.MockServerClient;
import org.mockserver.integration.ClientAndServer;

public class HttpClientPropertiesStub {

    private HttpClientPropertiesStub() {
    }

    public static HttpClientProperties of(ClientAndServer clientAndServer) {
        return HttpClientProperties.ofEndpoint("http://localhost:" + clientAndServer.getLocalPort());
    }

    public static HttpClientProperties of(MockServerClient mockServerClient) {
        return HttpClientProperties.ofEndpoint("http:/" + mockServerClient.remoteAddress());
    }
}
