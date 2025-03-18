package ru.yandex.ci.tms.client;

import io.netty.handler.codec.http.HttpHeaderNames;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.Header;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.HttpResponse;
import org.mockserver.model.HttpStatusCode;
import org.mockserver.model.JsonBody;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.SolomonClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;

@ContextConfiguration(classes = {
        SolomonClientTestConfig.class
})
public class SolomonPushTest extends CommonTestBase {
    @Autowired
    public ClientAndServer solomonServer;

    @Autowired
    public SolomonClient solomonClient;

    @BeforeEach
    void setUp() {
        solomonServer.clear(HttpRequest.request("/api/v2/push"));
    }

    @Test
    void push() {
        solomonServer.when(HttpRequest.request("/api/v2/push")
                        .withMethod(HttpMethod.POST.name())
                        .withQueryStringParameter("project", "ci-metrics")
                        .withQueryStringParameter("cluster", "testing")
                        .withQueryStringParameter("service", "common")
                        .withBody(JsonBody.json("""
                                {"metrics" : [ {
                                    "labels" : {
                                      "project" : "ci",
                                      "path" : "ci/registry"
                                    },
                                    "value" : 456.7
                                  } ]
                                }
                                 """))
                )
                .respond(HttpResponse.response()
                        .withStatusCode(HttpStatusCode.OK_200.code())
                        .withHeader(
                                Header.header(HttpHeaderNames.CONTENT_TYPE.toString(),
                                        MediaType.APPLICATION_JSON_VALUE
                                ))
                        .withBody("""
                                {"status":"OK","sensorsProcessed":98}
                                """));

        assertThat(solomonClient.push(new SolomonClient.RequiredMetricLabels("ci-metrics", "testing", "common"),
                SolomonMetrics.builder()
                        .metric(SolomonMetric.builder()
                                .label("project", "ci")
                                .label("path", "ci/registry")
                                .value(456.7)
                                .build())
                        .build()))
                .isEqualTo(new SolomonClient.PushResponse(null, 98, SolomonClient.PushStatus.OK));
    }

}
