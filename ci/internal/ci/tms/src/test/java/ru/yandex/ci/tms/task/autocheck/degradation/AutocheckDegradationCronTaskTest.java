package ru.yandex.ci.tms.task.autocheck.degradation;

import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

import io.netty.handler.codec.http.HttpHeaderNames;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.Header;
import org.mockserver.model.HttpStatusCode;
import org.mockserver.model.JsonBody;
import org.mockserver.verify.VerificationTimes;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.engine.spring.TestenvDegradationTestConfig;
import ru.yandex.ci.flow.spring.FlowZkTestConfig;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.tms.data.autocheck.SemaphoreId;
import ru.yandex.ci.tms.spring.CiTmsPropertiesTestConfig;
import ru.yandex.ci.tms.spring.clients.SandboxClientTestConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.AutocheckDegradationCronTaskConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.AutocheckDegradationMonitoringsSourceConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.AutocheckDegradationNotificationsManagerConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.AutocheckDegradationPostcommitManagerConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.AutocheckDegradationTaskInactivityConditionsCheckerConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.ChartsClientTestConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.JugglerClientTestConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.SolomonClientTestConfig;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static org.mockserver.model.JsonBody.json;
import static ru.yandex.ci.tms.task.autocheck.degradation.TestAutocheckDegradationUtils.getHttpResponseFromResource;
import static ru.yandex.ci.tms.task.autocheck.degradation.TestAutocheckDegradationUtils.setUpPostcommitsSemaphores;

@ContextConfiguration(classes = {
        CommonTestConfig.class,
        SandboxClientTestConfig.class,
        SolomonClientTestConfig.class,
        JugglerClientTestConfig.class,
        TestenvDegradationTestConfig.class,
        ChartsClientTestConfig.class,
        AutocheckDegradationMonitoringsSourceConfig.class,
        AutocheckDegradationNotificationsManagerConfig.class,
        AutocheckDegradationTaskInactivityConditionsCheckerConfig.class,
        AutocheckDegradationPostcommitManagerConfig.class,
        AutocheckDegradationCronTaskConfig.class,
        FlowZkTestConfig.class,
        CiTmsPropertiesTestConfig.class
})
class AutocheckDegradationCronTaskTest extends YdbCiTestBase {

    @Autowired
    public ClientAndServer sandboxServer;

    @Autowired
    public ClientAndServer solomonServer;

    @Autowired
    public ClientAndServer jugglerServer;

    @Autowired
    public ClientAndServer testenvServer;

    @Autowired
    public ClientAndServer chartsServer;

    @MockBean
    public BazingaTaskManager bazingaTaskManager;

    @Autowired
    public AutocheckDegradationCronTask task;

    @Autowired
    public AutocheckDegradationStateKeeper stateKeeper;


    @BeforeEach
    void setUp() {
        sandboxServer.reset();
        sandboxServer.when(request("/v1.0/semaphore/\\d+").withMethod(HttpMethod.PUT.name()))
                .respond(response().withBody(json("""
                        {}
                        """)).withStatusCode(HttpStatusCode.OK_200.code()));

        solomonServer.reset();
        jugglerServer.reset();
        jugglerServer.when(request("/v2/mutes/get_mutes"))
                .respond(response().withHeader(HttpHeaders.CONTENT_TYPE, "application/json")
                        .withBody("""
                                {
                                  "total": 0,
                                  "items": []
                                }
                                """));
        jugglerServer.when(request("/v2/mutes/set_mutes"))
                .respond(response().withStatusCode(HttpStatusCode.OK_200.code()));
        jugglerServer.when(request("/v2/mutes/remove_mutes"))
                .respond(response().withStatusCode(HttpStatusCode.OK_200.code()));

        testenvServer.reset();

        testenvServer.when(request("/handlers/\\w+").withMethod(HttpMethod.GET.name()))
                .respond(response().withStatusCode(HttpStatusCode.OK_200.code()));

        chartsServer.when(request("/api/v1/comments").withMethod(HttpMethod.GET.name()))
                .respond(response()
                        .withStatusCode(HttpStatusCode.OK_200.code())
                        .withBody("[]"));
        chartsServer.when(request("/api/v1/comments").withMethod(HttpMethod.POST.name()))
                .respond(response()
                        .withStatusCode(HttpStatusCode.OK_200.code())
                        .withBody("""
                                {
                                    "id":"testCommentId",
                                    "creatorLogin":"robot-ci",
                                    "createdDate":"2020-02-28T15:16:30.000Z"
                                }
                                """));
    }

    @Test
    void testEnablePostcommitAndSanitizersDegradation() throws Exception {
        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level").withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_all_enabled.json"));

        setUpPostcommitsSemaphores(sandboxServer, 1000, 0, 10, 0, 20, 10, "user",
                TestAutocheckDegradationUtils.ROBOT_LOGIN);
        setUpTriggersToDegradationNeeded();
        setUpInflightPrecommitsAlert(110);

        task.execute(null);

        sandboxServer.verify(request("/v1.0/semaphore/12622386")
                .withMethod(HttpMethod.PUT.name())
                .withBody(json("""
                        {"capacity":0,"event":"Degradation enabled"}
                        """)));
        sandboxServer.verify(request("/v1.0/semaphore/16101546")
                .withMethod(HttpMethod.PUT.name())
                .withBody(json("""
                        {"capacity":0,"event":"Degradation enabled"}
                        """)));
        testenvServer.verify(request("/handlers/enableFallbackModeSanitizers"));

        verifyNoJugglerMutesCreated();
    }


    @Test
    void testDisablePostcommitsAndSanitizersDegradation() throws Exception {
        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK_FOR_BRANCH, 10);

        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level").withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_gcc_and_ios_enabled.json"));

        setUpPostcommitsSemaphores(
                sandboxServer, 0, 0, 0, 0, 0, 0,
                TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN
        );
        setUpTriggersToNoDegradationNeeded();
        setUpInflightPrecommitsAlert(110);

        task.execute(null);
        testenvServer.verify(request("/handlers/disableFallbackModeSanitizers"));

        testenvServer.clear(request("/api/te/v1.0/autocheck/precommit/service_level"));
        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level").withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_all_enabled.json"));

        for (int i = 1; i <= 4; ++i) {
            task.execute(null);

            sandboxServer.verify(request("/v1.0/semaphore/12622386")
                    .withMethod(HttpMethod.PUT.name())
                    .withBody(json("""
                            {
                            "capacity":%s,
                            "event":"100 \\u003c\\u003d inflight precommits \\u003c 125 \
                            -\\u003e max recommended capacity: 800"}
                            """.formatted(i * 200))));

            setUpPostcommitsSemaphores(sandboxServer, i * 200, 0, 0,
                    0, 20, 10, TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN);
        }

        task.execute(null);
        sandboxServer.verify(request("/v1.0/semaphore/16101546")
                .withMethod(HttpMethod.PUT.name())
                .withBody(json("""
                        {
                            "capacity":10,
                            "event":"target capacity: 10"
                        }
                        """)));

        verifyNoJugglerMutesCreated();
    }

    @Test
    void testDisablePostcommitsAndSanitizersDegradation_whenPessimizedPrecommitSemaphoreNotZero() throws Exception {
        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK_FOR_BRANCH, 10);

        testenvServer.when(request("/api/te/v1.0/autocheck/precommit/service_level")
                .withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_gcc_and_ios_enabled.json"));

        setUpPostcommitsSemaphores(
                sandboxServer, 0, 0, 0, 0,
                5, 2, //  pezzimized_precommit semaphore value(2) > capacity(5)
                TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN
        );
        setUpTriggersToNoDegradationNeeded();
        setUpInflightPrecommitsAlert(110);

        task.execute(null);
        testenvServer.verify(request("/handlers/disableFallbackModeSanitizers"));

        testenvServer.clear(request("/api/te/v1.0/autocheck/precommit/service_level"));
        testenvServer.when(request("/api/te/v1.0/autocheck/precommit/service_level")
                .withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_all_enabled.json"));

        for (int i = 1; i <= 4; ++i) {
            task.execute(null);

            sandboxServer.verify(request("/v1.0/semaphore/12622386")
                    .withMethod(HttpMethod.PUT.name())
                    .withBody(json("""
                            {
                            "capacity":%s,
                            "event":"100 \\u003c\\u003d inflight precommits \\u003c 125 \
                            -\\u003e max recommended capacity: 800"}
                            """.formatted(i * 200))));

            setUpPostcommitsSemaphores(sandboxServer, i * 200, 0, 0,
                    0, 20, 10, TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN);
        }

        task.execute(null);
        sandboxServer.verify(request("/v1.0/semaphore/16101546")
                .withMethod(HttpMethod.PUT.name())
                .withBody(json("""
                        {
                            "capacity":10,
                            "event":"target capacity: 10"
                        }
                        """)));

        verifyNoJugglerMutesCreated();
    }

    @Test
    void dontDisablePostcommitsDegradation_whenPessimizedPrecommitSemaphoreValueMoreThen10() throws Exception {
        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK_FOR_BRANCH, 10);

        testenvServer.when(request("/api/te/v1.0/autocheck/precommit/service_level")
                .withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_gcc_and_ios_enabled.json"));

        setUpPostcommitsSemaphores(
                sandboxServer, 0, 0, 0, 0,
                20, 12, //  pezzimized_precommit semaphore value(12) > threshold(10)
                TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN
        );
        setUpTriggersToNoDegradationNeeded();
        setUpInflightPrecommitsAlert(110);

        task.execute(null);
        testenvServer.verify(request("/handlers/disableFallbackModeSanitizers"));

        testenvServer.clear(request("/api/te/v1.0/autocheck/precommit/service_level"));
        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level")
                        .withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_all_enabled.json"));

        // 12622386 - AUTOCHECK semaphore
        sandboxServer.verify(request("/v1.0/semaphore/12622386").withMethod(HttpMethod.PUT.name()),
                VerificationTimes.exactly(0));

        task.execute(null);
        // 16101546 - AUTOCHECK_FOR_BRANCH semaphore
        sandboxServer.verify(request("/v1.0/semaphore/16101546").withMethod(HttpMethod.PUT.name()),
                VerificationTimes.exactly(0));

        verifyNoJugglerMutesCreated();
    }

    @Test
    void dontDisablePostcommitsDegradation_whenFullPessimizedPrecommitSemaphore() throws Exception {
        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK_FOR_BRANCH, 10);

        testenvServer.when(request("/api/te/v1.0/autocheck/precommit/service_level")
                .withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_gcc_and_ios_enabled.json"));

        setUpPostcommitsSemaphores(
                sandboxServer, 0, 0, 0, 0,
                5, 5, //  pezzimized_precommit semaphore value(5) == capacity(5)
                TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN
        );
        setUpTriggersToNoDegradationNeeded();
        setUpInflightPrecommitsAlert(110);

        task.execute(null);
        testenvServer.verify(request("/handlers/disableFallbackModeSanitizers"));

        testenvServer.clear(request("/api/te/v1.0/autocheck/precommit/service_level"));
        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level")
                        .withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_all_enabled.json"));

        // 12622386 - AUTOCHECK semaphore
        sandboxServer.verify(request("/v1.0/semaphore/12622386").withMethod(HttpMethod.PUT.name()),
                VerificationTimes.exactly(0));

        task.execute(null);
        // 16101546 - AUTOCHECK_FOR_BRANCH semaphore
        sandboxServer.verify(request("/v1.0/semaphore/16101546").withMethod(HttpMethod.PUT.name()),
                VerificationTimes.exactly(0));

        verifyNoJugglerMutesCreated();
    }

    @Test
    void testPlatformDegradationEnabledManually() throws Exception {
        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK, 1000);
        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK_FOR_BRANCH, 10);

        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level").withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_all_disabled.json"));

        setUpPostcommitsSemaphores(sandboxServer, 0, 0, 0,
                0, 20, 10, TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN);
        setUpTriggersToNoDegradationNeeded();
        setUpInflightPrecommitsAlert(110);

        task.execute(null);

        verifyNoJugglerMutesCreated();
    }

    @Test
    void testHoldTriggerState() throws Exception {

        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK, 1000);
        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK_FOR_BRANCH, 10);
        stateKeeper.setPreviousTriggerState(true);

        testenvServer.clear(request("/handlers/\\w+"));
        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level").withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_gcc_and_ios_enabled.json"));

        setUpPostcommitsSemaphores(sandboxServer, 0, 0, 0,
                0, 20, 10, TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN);
        setUpTriggerAutocheckDegradationFullOKWithWarn();
        setUpTriggerAutocheckBlockingDegradationCRIT();
        setUpInflightPrecommitsAlert(110);

        task.execute(null);

        verifyNoJugglerMutesCreated();
    }

    @Test
    void testHoldUntriggerState() throws Exception {
        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK, 1000);
        stateKeeper.setSemaphoreValue(SemaphoreId.AUTOCHECK_FOR_BRANCH, 10);
        stateKeeper.setPreviousTriggerState(false);

        testenvServer.clear(request("/handlers/\\w+"));
        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level").withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_all_enabled.json"));

        setUpPostcommitsSemaphores(sandboxServer, 0, 0, 0,
                0, 20, 10, TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN);
        setUpTriggerAutocheckDegradationFullOKWithWarn();
        setUpTriggerAutocheckBlockingDegradationCRIT();
        setUpInflightPrecommitsAlert(110);

        task.execute(null);

        verifyNoJugglerMutesCreated();
    }

    @Test
    void testDisablePostcommitsAndSanitizersDegradation_whenGetInflightPrecommitsReturnsEmptyString() {
        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level").withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_gcc_and_ios_enabled.json"));

        setUpPostcommitsSemaphores(
                sandboxServer, 0, 0, 0, 0, 0, 0,
                TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN
        );
        setUpTriggersToNoDegradationNeeded();
        setUpInflightPrecommitsAlert("");

        Assertions.assertThrows(
                IllegalStateException.class,
                () -> task.execute(null),
                "Task must throw exception if inflight precommits is null"
        );

        testenvServer.verify(request("/handlers/disableFallbackModeSanitizers"), VerificationTimes.exactly(0));

        sandboxServer.verify(request("/v1.0/semaphore/\\d").withMethod(HttpMethod.PUT.name()),
                VerificationTimes.exactly(0));

        verifyNoJugglerMutesCreated();
    }

    @Test
    void testDisablePostcommitsAndSanitizersDegradation_whenGetInflightPrecommitsReturnsNull() {
        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level").withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_gcc_and_ios_enabled.json"));

        setUpPostcommitsSemaphores(
                sandboxServer, 0, 0, 0, 0, 0, 0,
                TestAutocheckDegradationUtils.ROBOT_LOGIN, TestAutocheckDegradationUtils.ROBOT_LOGIN
        );
        setUpTriggersToNoDegradationNeeded();
        setUpInflightPrecommitsAlert(null);

        Assertions.assertThrows(
                IllegalStateException.class,
                () -> task.execute(null),
                "Task must throw exception if inflight precommits is null"
        );

        testenvServer.verify(request("/handlers/disableFallbackModeSanitizers"), VerificationTimes.exactly(0));

        sandboxServer.verify(request("/v1.0/semaphore/\\d").withMethod(HttpMethod.PUT.name()),
                VerificationTimes.exactly(0));

        verifyNoJugglerMutesCreated();
    }

    @Test
    void noDegradationNeeded_whenNotEnoughInflightPrecommits() throws Exception {
        testenvServer.when(
                request("/api/te/v1.0/autocheck/precommit/service_level").withMethod(HttpMethod.GET.name())
        ).respond(getHttpResponseFromResource("server_responses/service_level_all_enabled.json"));

        setUpPostcommitsSemaphores(
                sandboxServer, 1000, 50, 10, 0, 20, 0,
                "user",
                TestAutocheckDegradationUtils.ROBOT_LOGIN
        );
        setUpTriggerAutocheckDegradationFullCRIT();
        setUpTriggerAutocheckBlockingDegradationOK();
        setUpInflightPrecommitsAlert(10);

        task.execute(null);

        testenvServer.verify(request("/handlers/disableFallbackModeSanitizers"), VerificationTimes.exactly(0));
        sandboxServer.verify(request("/v1.0/semaphore/\\d").withMethod(HttpMethod.PUT.name()),
                VerificationTimes.exactly(0));

        verifyJugglerMutesCreated();
        verifyNoJugglerMutesRemoved();
    }

    private void setUpTriggersToNoDegradationNeeded() {
        setUpTriggerAutocheckDegradationFullOK();
        setUpTriggerAutocheckBlockingDegradationCRIT();
    }

    private void setUpTriggersToDegradationNeeded() {
        setUpTriggerAutocheckDegradationFullCRIT();
        setUpTriggerAutocheckBlockingDegradationCRIT();
    }

    private void setUpTriggerAutocheckDegradationFullOK() {
        jugglerServer.when(request("/v2/checks/get_checks_state")
                .withMethod(HttpMethod.POST.name())
                .withBody(json(ResourceUtils.textResource(
                        "server_requests/juggler_get_checks_state_by_tags.json"))))
                .respond(getHttpResponseFromResource(
                        "server_responses/juggler_get_checks_state_by_tags_ok.json"));
    }

    private void setUpTriggerAutocheckDegradationFullOKWithWarn() {
        jugglerServer.when(request("/v2/checks/get_checks_state")
                .withMethod(HttpMethod.POST.name())
                .withBody(json(ResourceUtils.textResource(
                        "server_requests/juggler_get_checks_state_by_tags.json"))))
                .respond(getHttpResponseFromResource(
                        "server_responses/juggler_get_checks_state_by_tags_ok_with_warn.json"
                ));
    }

    private void setUpTriggerAutocheckDegradationFullCRIT() {
        jugglerServer.when(request("/v2/checks/get_checks_state")
                .withMethod(HttpMethod.POST.name())
                .withBody(json(ResourceUtils.textResource(
                        "server_requests/juggler_get_checks_state_by_tags.json"))))
                .respond(getHttpResponseFromResource(
                        "server_responses/juggler_get_checks_state_by_tags_crit.json"
                ));
    }

    private void setUpTriggerAutocheckBlockingDegradationOK() {
        jugglerServer.when(request("/v2/checks/get_checks_state")
                .withMethod(HttpMethod.POST.name())
                .withBody(json(ResourceUtils.textResource(
                        "server_requests/juggler_autocheck_blocking_degradation.json"))))
                .respond(getHttpResponseFromResource(
                        "server_responses/juggler_autocheck_blocking_degradation_ok.json"));
    }

    private void setUpTriggerAutocheckBlockingDegradationCRIT() {
        jugglerServer.when(request("/v2/checks/get_checks_state")
                .withMethod(HttpMethod.POST.name())
                .withBody(json(ResourceUtils.textResource(
                        "server_requests/juggler_autocheck_blocking_degradation.json"))))
                .respond(getHttpResponseFromResource(
                        "server_responses/juggler_autocheck_blocking_degradation_crit.json"));
    }

    private void setUpInflightPrecommitsAlert(float inflightPrecommitsAlertValue) {
        setUpInflightPrecommitsAlert(String.valueOf(inflightPrecommitsAlertValue));
    }

    private void setUpInflightPrecommitsAlert(@Nullable String inflightPrecommitsAlertValue) {
        solomonServer.when(request(
                "/api/v2/projects/testProjectId/alerts/testInflightPrecommitsId/state/evaluation"
                ).withMethod(HttpMethod.GET.name())
        ).respond(response()
                .withStatusCode(HttpStatusCode.OK_200.code())
                .withHeader(Header.header(
                        HttpHeaderNames.CONTENT_TYPE.toString(),
                        MediaType.APPLICATION_JSON.toString()
                )).withBody("""
                        {
                          "alertId": "testInflightPrecommitsId",
                          "projectId": "testProjectId",
                          "status": {
                            "code": "OK",
                            "annotations": {"""
                        + (inflightPrecommitsAlertValue != null
                        ? "\"inflight_value\": \"%s\"".formatted(inflightPrecommitsAlertValue)
                        : "")
                        + """
                            }
                          }
                        }
                        """)
        );
    }

    private void verifyNoJugglerMutesRemoved() {
        jugglerServer.verify(request("/v2/mutes/remove_mutes").withMethod("POST"), VerificationTimes.exactly(0));
    }

    private void verifyJugglerMutesCreated() {
        jugglerServer.verify(
                request("/v2/mutes/set_mutes")
                        .withMethod("POST")
                        .withBody(JsonBody.json(Map.of(
                                "description", "muted automatically",
                                "filters", List.of(Map.of(
                                        "namespace", "devtools.ci",
                                        "tags", List.of("notify")
                                ))
                        ))),
                VerificationTimes.atLeast(1)
        );
    }

    private void verifyNoJugglerMutesCreated() {
        jugglerServer.verify(request("/v2/mutes/set_mutes").withMethod("POST"), VerificationTimes.exactly(0));
    }
}
