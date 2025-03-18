package ru.yandex.ci.tms.task.autocheck.degradation;

import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.HttpRequest;
import org.mockserver.model.HttpResponse;
import org.mockserver.model.HttpStatusCode;
import org.mockserver.model.JsonSchemaBody;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.tms.spring.CiTmsPropertiesTestConfig;
import ru.yandex.ci.tms.spring.clients.SandboxClientTestConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.AutocheckDegradationPostcommitManagerConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.AutocheckDegradationTaskInactivityConditionsCheckerConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.mockserver.model.JsonBody.json;

@ContextConfiguration(classes = {
        SandboxClientTestConfig.class,
        AutocheckDegradationTaskInactivityConditionsCheckerConfig.class,
        AutocheckDegradationPostcommitManagerConfig.class,
        CiTmsPropertiesTestConfig.class
})
public class AutocheckDegradationPostcommitManagerTest extends YdbCiTestBase {
    @Autowired
    public AutocheckDegradationPostcommitManager semaphoresManager;

    @Autowired
    public ClientAndServer sandboxServer;

    @MockBean
    public BazingaTaskManager bazingaTaskManager;

    @BeforeEach
    void setUp() {
        sandboxServer.clear(HttpRequest.request("/v1.0/semaphore/\\d+").withMethod(HttpMethod.GET.name()));
        sandboxServer.clear(HttpRequest.request("/v1.0/semaphore/\\d+").withMethod(HttpMethod.PUT.name()));
        sandboxServer.clear(HttpRequest.request("/v1.0/semaphore/\\d+/audit"));
        sandboxServer.when(HttpRequest.request("/v1.0/semaphore/\\d+").withMethod(HttpMethod.PUT.name()))
                .respond(HttpResponse.response().withBody(json("{}")).withStatusCode(HttpStatusCode.OK_200.code()));

    }

    @Test
    void testAutocheckSemaphoreIncreaseByInflightPrecommitsLevel() {
        TestAutocheckDegradationUtils.setUpPostcommitsSemaphores(sandboxServer, 0, 0, 0,
                0, 20, 10, TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN);

        AutocheckDegradationMonitoringsSource.MonitoringState monitoring =
                new AutocheckDegradationMonitoringsSource.MonitoringState(false, AutomaticDegradation.enabled(), 110);

        for (int i = 1; i <= 2; ++i) {
            semaphoresManager.changeSemaphoresInNormalMode(monitoring);

            sandboxServer.verify(HttpRequest.request("/v1.0/semaphore/12622386")
                    .withMethod(HttpMethod.PUT.name())
                    .withBody(new JsonSchemaBody(
                            "{\"capacity\":" + i * 200 + ", " +
                                    "\"event\":\"100 \\u003c\\u003d inflight precommits \\u003c 125 " +
                                    "-\\u003e max recommended capacity: 800\"}")));

            TestAutocheckDegradationUtils.setUpPostcommitsSemaphores(sandboxServer, i * 200, 0, 0,
                    0, 20, 10, TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN);
        }
    }

    @Test
    void testAutocheckSemaphoreDecreaseByInflightPrecommitsLevel() {
        TestAutocheckDegradationUtils.setUpPostcommitsSemaphores(sandboxServer, 1600, 0, 0,
                0, 20, 10, TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN);
        AutocheckDegradationMonitoringsSource.MonitoringState monitoring =
                new AutocheckDegradationMonitoringsSource.MonitoringState(false, AutomaticDegradation.enabled(), 100);

        semaphoresManager.changeSemaphoresInNormalMode(monitoring);

        sandboxServer.verify(HttpRequest.request("/v1.0/semaphore/12622386")
                .withMethod(HttpMethod.PUT.name())
                .withBody(new JsonSchemaBody(
                        "{\"capacity\":800, " +
                                "\"event\":\"100 \\u003c\\u003d inflight precommits \\u003c 125 " +
                                "-\\u003e max recommended capacity: 800\"}")));
    }

    @Test
    void testAutocheckSemaphoreDecreaseAfterManualChange() {
        TestAutocheckDegradationUtils.setUpPostcommitsSemaphores(sandboxServer, 1000, 0, 0,
                0, 20, 10, "user", TestAutocheckDegradationUtils.ROBOT_LOGIN);
        var monitoring = new AutocheckDegradationMonitoringsSource.MonitoringState(
                false, AutomaticDegradation.enabled(), 100
        );

        semaphoresManager.changeSemaphoresInNormalMode(monitoring);

        sandboxServer.verify(HttpRequest.request("/v1.0/semaphore/12622386")
                .withMethod(HttpMethod.PUT.name())
                .withBody(new JsonSchemaBody(
                        "{\"capacity\":800, " +
                                "\"event\":\"100 \\u003c\\u003d inflight precommits \\u003c 125 " +
                                "-\\u003e max recommended capacity: 800\"}")));
    }
}
