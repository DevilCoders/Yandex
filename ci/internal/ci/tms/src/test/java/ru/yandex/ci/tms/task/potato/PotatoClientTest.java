package ru.yandex.ci.tms.task.potato;

import java.util.Map;

import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.tms.task.potato.client.Status;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static ru.yandex.ci.test.TestUtils.textResource;

@ExtendWith(MockServerExtension.class)
class PotatoClientTest extends CommonTestBase {

    private final MockServerClient server;
    private PotatoClient client;

    PotatoClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        client = PotatoClientImpl.create(HttpClientPropertiesStub.of(server));
    }

    @Test
    public void healthCheck() {
        server.when(request("/telemetry/healthcheck").withMethod(HttpMethod.GET.name()))
                .respond(response(textResource("potato/health-with-ts.json")));

        assertThat(client.healthCheck(null))
                .isEqualTo(Map.of(
                        "potato.errorHandler", new Status(true, null),
                        "rootLoader.parsing.yaml.github-enterprise/data-ui/potato", new Status(false, 1611746448152L)
                ));
    }

    @Test
    public void healthCheckWithNamespace() {
        server.when(request("/telemetry/healthcheck")
                .withQueryStringParameter("namespace", "rootLoader.parsing.yaml.*")
                .withMethod(HttpMethod.GET.name()))
                .respond(response(textResource("potato/health-filtered.json")));

        assertThat(client.healthCheck("rootLoader.parsing.yaml.*"))
                .isEqualTo(Map.of(
                        "rootLoader.parsing.yaml.github-enterprise/data-ui/potato", new Status(true, 1611750679123L)
                ));
    }
}
