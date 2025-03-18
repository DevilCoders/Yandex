package ru.yandex.ci.client.trendbox;

import java.time.Instant;

import io.netty.handler.codec.http.HttpMethod;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;
import org.mockserver.model.HttpStatusCode;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.trendbox.model.TrendboxScpType;
import ru.yandex.ci.client.trendbox.model.TrendboxWorkflow;
import ru.yandex.ci.util.ResourceUtils;

import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

@ExtendWith(MockServerExtension.class)
public class TrendboxClientTest {

    private final MockServerClient server;
    private TrendboxClient trendboxClient;

    public TrendboxClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        trendboxClient = TrendboxClient.create(HttpClientPropertiesStub.of(server));
    }

    @Test
    void getWorkflows() {
        server.when(
                request("/api/v1/workflows")
                        .withMethod(HttpMethod.GET.name())
                        .withQueryStringParameter("page", String.valueOf(1)))
                .respond(
                        response(resource("workflows.json"))
                                .withStatusCode(HttpStatusCode.OK_200.code())
                );

        server.when(
                request("/api/v1/workflows")
                        .withMethod(HttpMethod.GET.name())
                        .withQueryStringParameter("page", String.valueOf(2)))
                .respond(
                        response(resource("workflows-page3.json")) //Empty
                                .withStatusCode(HttpStatusCode.OK_200.code())
                );

        Assertions.assertThat(trendboxClient.getWorkflows()).containsExactly(
                new TrendboxWorkflow(
                        TrendboxScpType.ARCADIA,
                        "arcadia:/frontend",
                        "build",
                        Instant.parse("2021-04-01T12:53:49.071Z"),
                        Instant.parse("2021-04-15T12:50:14.503Z"),
                        Instant.parse("2021-04-15T12:45:35.096Z")
                ),
                new TrendboxWorkflow(
                        TrendboxScpType.GITHUB,
                        "git@github.yandex-team.ru:IMS/vh-selfservice.git",
                        "tkit",
                        Instant.parse("2021-04-01T12:54:00.088Z"),
                        Instant.parse("2021-04-12T19:42:50.546Z"),
                        Instant.parse("2021-04-12T19:57:42.387Z")
                )
        );
    }

    @Test
    void getWorkflowsPaging() {

        for (int page = 1; page <= 3; page++) {
            server.when(
                    request("/api/v1/workflows")
                            .withMethod(HttpMethod.GET.name())
                            .withQueryStringParameter("page", String.valueOf(page)))
                    .respond(
                            response(resource("workflows-page" + page + ".json"))
                                    .withStatusCode(HttpStatusCode.OK_200.code())
                    );
        }

        Assertions.assertThat(trendboxClient.getWorkflows()).containsExactly(
                new TrendboxWorkflow(
                        TrendboxScpType.ARCADIA,
                        "arcadia:/frontend",
                        "build",
                        Instant.parse("2021-04-01T12:53:49.071Z"),
                        Instant.parse("2021-04-15T12:50:14.503Z"),
                        Instant.parse("2021-04-15T12:45:35.096Z")
                ),
                new TrendboxWorkflow(
                        TrendboxScpType.GITHUB,
                        "git@github.yandex-team.ru:IMS/vh-selfservice.git",
                        "tkit",
                        Instant.parse("2021-04-01T12:54:00.088Z"),
                        Instant.parse("2021-04-12T19:42:50.546Z"),
                        Instant.parse("2021-04-12T19:57:42.387Z")
                )
        );
    }

    private static String resource(String name) {
        return ResourceUtils.textResource(name);
    }
}
