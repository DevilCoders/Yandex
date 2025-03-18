package ru.yandex.ci.tms.client;

import io.netty.handler.codec.http.HttpHeaderNames;
import org.junit.jupiter.api.Assertions;
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
import ru.yandex.ci.core.te.TestenvDegradationManager;
import ru.yandex.ci.engine.spring.TestenvDegradationTestConfig;
import ru.yandex.ci.util.ResourceUtils;

@ContextConfiguration(classes = {
        TestenvDegradationTestConfig.class
})
public class TestenvDegradationManagerTest extends CommonTestBase {
    @Autowired
    public ClientAndServer teServer;

    @Autowired
    public TestenvDegradationManager teManager;

    @BeforeEach
    void setUp() {
        teServer.clear(HttpRequest.request("/api/te/v1.0/autocheck/precommit/service_level"));
        teServer.when(HttpRequest.request("/handlers/\\w+").withMethod(HttpMethod.GET.name()))
                .respond(HttpResponse.response().withStatusCode(HttpStatusCode.OK_200.code()));
    }

    @Test
    void testTestenvDegradationManagerGetStatus() {
        teServer.when(HttpRequest.request("/api/te/v1.0/autocheck/precommit/service_level")
                .withMethod(HttpMethod.GET.name()))
                .respond(HttpResponse.response()
                        .withStatusCode(HttpStatusCode.OK_200.code())
                        .withHeader(Header.header(HttpHeaderNames.CONTENT_TYPE.toString(),
                                MediaType.APPLICATION_JSON.toString()))
                        .withBody(ResourceUtils.textResource("server_responses/service_level_all_enabled.json")));

        Assertions.assertEquals(3, teManager.getPrecommitPlatformStatuses().getPlatforms().size());
    }
}
