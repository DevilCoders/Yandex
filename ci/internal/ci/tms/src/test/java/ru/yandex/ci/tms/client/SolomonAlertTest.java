package ru.yandex.ci.tms.client;

import io.netty.handler.codec.http.HttpHeaderNames;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.Header;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.HttpResponse;
import org.mockserver.model.HttpStatusCode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.tms.data.SolomonAlert;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.SolomonClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;

@ContextConfiguration(classes = {
        SolomonClientTestConfig.class
})
public class SolomonAlertTest extends CommonTestBase {
    @Autowired
    public ClientAndServer solomonServer;

    @Autowired
    public SolomonClient solomonClient;

    @BeforeEach
    void setUp() {
        solomonServer.clear(HttpRequest.request("/api/v2/projects/testProjectId/alerts/testAlertId/state/evaluation"));
        solomonServer.clear(HttpRequest.request("/api/v2/projects/testProjectId/alerts/testAlertIdException/state" +
                "/evaluation"));
    }

    @Test
    void getStatusAlarm() {
        solomonServer.when(HttpRequest.request("/api/v2/projects/testProjectId/alerts/testAlertId/state/evaluation")
                .withMethod(HttpMethod.GET.name()))
                .respond(HttpResponse.response()
                        .withStatusCode(HttpStatusCode.OK_200.code())
                        .withHeader(Header.header(HttpHeaderNames.CONTENT_TYPE.toString(),
                                MediaType.APPLICATION_JSON.toString()))
                        .withBody("{\"status\": {\"code\": \"alarm\"}}"));

        SolomonAlert alert = new SolomonAlert("testProjectId", "testAlertId");

        assertThat(solomonClient.getAlertStatusCode(alert))
                .isEqualTo(SolomonAlert.Status.ALARM);
    }

    @Test
    void getStatusUnrecognizedThrowException() {
        SolomonAlert alert = new SolomonAlert("testProjectId", "testAlertIdException");

        Assertions.assertThatThrownBy(() -> solomonClient.getAlertStatusCode(alert))
                .isInstanceOf(RuntimeException.class)
                .hasMessageStartingWith("Wrong http code 404 for url ");
    }
}
