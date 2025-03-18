package ru.yandex.ci.client.sandbox;

import java.io.IOException;
import java.nio.charset.StandardCharsets;

import com.google.common.io.ByteStreams;
import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

@ExtendWith(MockServerExtension.class)
public class ProxySandboxClientTest {

    private final MockServerClient server;
    private ProxySandboxClient sandboxClient;

    ProxySandboxClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        sandboxClient = ProxySandboxClientImpl.create(HttpClientPropertiesStub.of(server));
    }

    @Test
    void testDownloadResource() throws IOException {
        long resourceId = 111222333444L;
        String resource = "some-resource";

        server.when(request("/" + resourceId).withMethod(HttpMethod.GET.name()))
                .respond(response(resource));

        try (var response = sandboxClient.downloadResource(resourceId)) {
            assertThat(new String(ByteStreams.toByteArray(response.getStream()), StandardCharsets.UTF_8))
                    .isEqualTo(resource);
        }
    }
}
