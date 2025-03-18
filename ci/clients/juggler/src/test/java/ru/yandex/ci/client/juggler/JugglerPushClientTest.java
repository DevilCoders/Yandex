package ru.yandex.ci.client.juggler;

import java.time.Instant;
import java.util.List;

import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;
import org.mockserver.model.HttpStatusCode;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.juggler.model.RawEvent;
import ru.yandex.ci.client.juggler.model.RawEventsResponse;
import ru.yandex.ci.client.juggler.model.Status;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static org.mockserver.model.JsonBody.json;

@ExtendWith(MockServerExtension.class)
class JugglerPushClientTest {
    private final MockServerClient server;
    private JugglerPushClient client;

    JugglerPushClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        client = JugglerPushClientImpl.create(HttpClientPropertiesStub.of(server));
    }

    @Test
    void pushRawEvents() {
        server.when(
                request("/events")
                        .withMethod(HttpMethod.POST.name())
                        .withBody(json("""
                                { "events" : [
                                    {
                                          "host" : "ci-release-status-ci",
                                          "service" : "release-ci",
                                          "instance" : "ci/a.yaml",
                                          "status" : "WARN",
                                          "description" : "Manual trigger",
                                          "open_time" : 1627480001
                                    },
                                    {
                                          "host" : "ci-release-status-ci",
                                          "service" : "ayamler-release",
                                          "status" : "OK",
                                          "description" : "Ok!",
                                          "tags": ["tag1", "tag2"],
                                          "open_time" : 1627480002,
                                          "heartbeat": 900
                                    }
                                ]}
                                """
                        ))).respond(
                response("""
                        {
                            "accepted_events": 2,
                            "events": [ { "code": 200 }, { "code": 200 } ],
                            "success": true
                        }
                        """)
                        .withStatusCode(HttpStatusCode.OK_200.code())
        );
        var response = client.push(List.of(
                RawEvent.builder()
                        .host("ci-release-status-ci")
                        .service("release-ci")
                        .instance("ci/a.yaml")
                        .status(Status.WARN)
                        .description("Manual trigger")
                        .openTime(Instant.ofEpochSecond(1627480001))
                        .build(),
                RawEvent.builder()
                        .host("ci-release-status-ci")
                        .service("ayamler-release")
                        .status(Status.OK)
                        .description("Ok!")
                        .tags(List.of("tag1", "tag2"))
                        .openTime(Instant.ofEpochSecond(1627480002))
                        .heartbeat(900)
                        .build()
        ));
        assertThat(response).isEqualTo(new RawEventsResponse(
                2, true,
                List.of(
                        new RawEventsResponse.Event(200),
                        new RawEventsResponse.Event(200)
                )
        ));
    }
}
