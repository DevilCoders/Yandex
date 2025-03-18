package ru.yandex.ci.client.charts;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.Map;

import io.netty.handler.codec.http.HttpMethod;
import org.apache.commons.io.IOUtils;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;
import org.mockserver.model.JsonBody;

import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.charts.model.ChartsCommentRequest;
import ru.yandex.ci.client.charts.model.ChartsCommentType;
import ru.yandex.ci.client.charts.model.ChartsCreateCommentResponse;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

@ExtendWith(MockServerExtension.class)
class ChartsClientTest {
    private final MockServerClient server;
    private ChartsClient chartsClient;

    ChartsClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        chartsClient = ChartsClient.create(HttpClientPropertiesStub.of(server));
    }

    @Test
    public void testCreateComment() {
        server.when(request("/api/v1/comments").withMethod(HttpMethod.POST.name())
                                        .withBody(JsonBody.json(resource("/create_comment_request.json"))))
              .respond(response(resource("/create_comment_response.json")));

        Instant createTimestamp = Instant.ofEpochSecond(1582709400);

        ChartsCommentRequest request = new ChartsCommentRequest(
                "Users/localTestUser/localTestChartsChannel",
                ChartsCommentType.REGION,
                createTimestamp,
                Instant.ofEpochSecond(1582795800),
                "Test comment",
                Map.of("color", "blue", "visible", true)
        );

        ChartsCreateCommentResponse response = chartsClient.createComment(request);

        assertThat(response.getCreatorLogin()).isEqualTo("localTestUser");
        assertThat(response.getId()).isEqualTo("812bf5aa-62af-11ea-8ac2-63f18b8c4a8a");
        assertThat(response.getCreatedDate()).isEqualTo(createTimestamp);
    }

    private static String resource(String name) {
        try {
            return IOUtils.toString(ChartsClientTest.class.getResourceAsStream(name), StandardCharsets.UTF_8);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
