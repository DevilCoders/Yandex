package ru.yandex.ci.client.infra;

import java.time.Instant;
import java.util.Map;

import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.OAuthProvider;
import ru.yandex.ci.client.infra.event.EventDto;
import ru.yandex.ci.client.infra.event.SeverityDto;
import ru.yandex.ci.client.infra.event.TypeDto;
import ru.yandex.ci.util.ResourceUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

@ExtendWith(MockServerExtension.class)
@ExtendWith(MockitoExtension.class)
class InfraClientTest {

    private final MockServerClient server;
    private InfraClient client;

    InfraClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        var properties = HttpClientProperties.builder()
                .endpoint("http:/" + server.remoteAddress() + "/")
                .authProvider(new OAuthProvider("token"))
                .build();
        client = InfraClient.create(properties);
    }

    @Test
    void getEvents() {
        server.when(request("/v1/events")
                        .withMethod(HttpMethod.GET.name())
                        .withQueryStringParameter("from", "100")
                        .withQueryStringParameter("to", "200")
                )
                .respond(response(resource("getEvents.response.json")));

        var events = client.getEvents(1, Instant.ofEpochSecond(100), Instant.ofEpochSecond(200));

        assertThat(events).containsExactly(expectedEventDto());
    }

    @Test
    void createEvent() {
        server.when(request("/v1/events")
                        .withMethod(HttpMethod.POST.name())
                )
                .respond(response(resource("createEvent.response.json")));
        assertThat(client.createEvent(expectedEventDto()))
                .isEqualTo(expectedEventDto());
    }

    private static EventDto expectedEventDto() {
        return EventDto.builder()
                .id(6763911L)
                .serviceId(51)
                .environmentId(3131)
                .title("Release CI #3011")
                .description("https://a.yandex-team.ru/projects/ci/ci/releases/flow?dir=ci&id=ci-release" +
                        "&version=3011")
                .type(TypeDto.MAINTENANCE)
                .severity(SeverityDto.MINOR)
                .startTime(1658852422L)
                .finishTime(1658853235L)
                .tickets("")
                .meta(Map.of(
                        "ci_deduplicate_id",
                        "flowLaunch: d8268d257d3680e07ede76847fa6de43318baed89b908684952058c598e59813 job: " +
                                "create-infra"
                ))
                .man(false)
                .myt(false)
                .sas(false)
                .vla(false)
                .iva(false)
                .vlx(false)
                .build();
    }

    private static String resource(String name) {
        return ResourceUtils.textResource(name);
    }

}
