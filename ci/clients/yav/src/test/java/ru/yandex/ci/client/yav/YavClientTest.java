package ru.yandex.ci.client.yav;

import java.util.List;

import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.client.yav.model.DelegatingTokenResponse;
import ru.yandex.ci.client.yav.model.YavResponse;
import ru.yandex.ci.client.yav.model.YavSecret;
import ru.yandex.ci.client.yav.model.YavSecret.KeyValue;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.passport.tvmauth.TvmClient;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.when;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static org.mockserver.model.JsonBody.json;

@ExtendWith(MockitoExtension.class)
@ExtendWith(MockServerExtension.class)
public class YavClientTest {
    private static final int YAV_TVM_ID = 1;
    private static final int SERVICE_TVM_ID = 2;

    private final MockServerClient server;

    @Mock
    private TvmClient tvmClient;

    private YavClient yavClient;

    YavClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    void setUp() {
        server.reset();
        var properties = HttpClientProperties.builder()
                .endpoint("http:/" + server.remoteAddress() + "/")
                .authProvider(new TvmAuthProvider(tvmClient, YAV_TVM_ID))
                .build();
        this.yavClient = YavClientImpl.create(properties, SERVICE_TVM_ID);
    }

    @Test
    void noSecretInfoInToString() {
        var values = List.of(
                new KeyValue("key1", "value1"),
                new KeyValue("key2", "value2"),
                new KeyValue("key3", "value3"));

        YavSecret secret = new YavSecret(
                YavResponse.Status.OK, "code", "message", "secretVersion", values
        );

        String toString = secret.toString();

        assertThat(toString).doesNotContain(List.of("value1", "value2", "value3"));
    }

    @Test
    void checkResponseStatusTest() {
        YavSecret noError = new YavSecret(
                YavResponse.Status.WARNING, "code", "message", "secretVersion", List.of()
        );

        Assertions.assertDoesNotThrow(() -> YavClientImpl.checkResponseStatus(noError, "no error"));

        YavSecret error = new YavSecret(
                YavResponse.Status.ERROR, "code", "message", "secretVersion", List.of()
        );
        Assertions.assertThrows(IllegalStateException.class, () -> YavClientImpl.checkResponseStatus(error, "error"));
    }

    @Test
    void getSecretByDelegatingToken() {
        when(tvmClient.getServiceTicketFor(YAV_TVM_ID)).thenReturn("some-ticket");
        server.when(request("/1/tokens/").withMethod(HttpMethod.POST.name())
                .withHeader("X-Ya-Service-Ticket", "some-ticket")
                .withBody(json(resource("get-secret-request.json"))))
                .respond(response(resource("get-secret-response.json")));
        var secret = yavClient.getSecretByDelegatingToken("token-1", "version-1", "signature-1");

        var values = List.of(
                new KeyValue("key1", "value1"),
                new KeyValue("key2", "value2"),
                new KeyValue("key3", "value3"));
        YavSecret expected = new YavSecret(
                YavResponse.Status.OK, "code", "message", "secretVersion", values
        );
        assertThat(secret)
                .isEqualTo(expected);
    }

    @Test
    void createDelegatingToken() {
        when(tvmClient.getServiceTicketFor(YAV_TVM_ID)).thenReturn("some-ticket");
        server.when(request("/1/secrets/uuid-i1/tokens/").withMethod(HttpMethod.POST.name())
                .withHeader("Content-Type", "application/x-www-form-urlencoded")
                .withHeader("X-Ya-Service-Ticket", "some-ticket")
                .withHeader("X-Ya-User-Ticket", "user-ticket-1")
                .withBody(resource("create-token-request.txt")))
                .respond(response(resource("create-token-response.json")));
        var response = yavClient.createDelegatingToken("user-ticket-1",
                "uuid-i1", "sign-1 test value", "comment special for request");
        assertThat(response)
                .isEqualTo(new DelegatingTokenResponse(YavResponse.Status.OK, "code", "message",
                        "token-out-1", "token-uuid-out-1"));
    }

    private static String resource(String name) {
        return ResourceUtils.textResource(name);
    }
}
