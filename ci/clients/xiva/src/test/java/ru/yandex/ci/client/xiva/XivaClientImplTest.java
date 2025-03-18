package ru.yandex.ci.client.xiva;

import com.fasterxml.jackson.databind.ObjectMapper;
import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;
import org.mockserver.model.JsonBody;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;

import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static org.mockserver.model.HttpStatusCode.OK_200;

@ExtendWith(MockitoExtension.class)
@ExtendWith(MockServerExtension.class)
class XivaClientImplTest {

    private static final ObjectMapper MAPPER = new ObjectMapper();

    private final MockServerClient server;
    private XivaClient xivaClient;

    XivaClientImplTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        xivaClient = XivaClientImpl.create("ci", HttpClientPropertiesStub.of(server));
    }

    @Test
    void send() {
        server.when(request("/v2/send")
                .withMethod(HttpMethod.POST.name())
                .withQueryStringParameter("service", "ci")
                .withQueryStringParameter("topic", "topic-1")
                .withQueryStringParameter("event", "event title")
                .withBody(JsonBody.json("""
                        {
                            "payload": {
                                "field1": 1,
                                "type": "release-updated"
                            }
                        }"""
                )))
                .respond(response().withStatusCode(OK_200.code()));

        xivaClient.send("topic-1", SendRequest.create(
                MAPPER.createObjectNode()
                        .put("field1", 1)
                        .put("type", "release-updated")
        ), "event title");
    }
}
