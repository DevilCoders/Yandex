package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.Map;
import java.util.stream.Stream;

import com.google.gson.JsonObject;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.HttpStatusCode;
import org.mockserver.model.JsonBody;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ActiveProfiles;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.ContextHierarchy;
import org.springframework.test.context.TestExecutionListeners;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.staff.StaffClient;
import ru.yandex.ci.client.staff.StaffOfficial;
import ru.yandex.ci.client.staff.StaffPerson;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.autocheck.AutocheckConstants.PostCommits;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.spring.ci.AutocheckForStorageIntegrationTestConfig;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.spring.storage.StorageIntegrationTestConfig;
import ru.yandex.ci.engine.pcm.PCMService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.storage.api.check.ApiCheckService;
import ru.yandex.ci.storage.api.controllers.StorageApiController;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.test.LoggingTestListener;
import ru.yandex.ci.test.YdbCleanupTestListener;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.ci.test.random.TestRandomUtils;
import ru.yandex.passport.tvmauth.TvmClient;

import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.when;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

@ExtendWith(SpringExtension.class)
@ActiveProfiles(CiProfile.UNIT_TEST_PROFILE)
@ContextHierarchy(
        {
                @ContextConfiguration(
                        name = "storage",
                        classes = {
                                StorageIntegrationTestConfig.class
                        }
                ),
                @ContextConfiguration(
                        name = "ci",
                        classes = {
                                AutocheckForStorageIntegrationTestConfig.class
                        }),
        }
)

@TestExecutionListeners(
        value = {LoggingTestListener.class, YdbCleanupTestListener.class},
        mergeMode = TestExecutionListeners.MergeMode.MERGE_WITH_DEFAULTS
)
@Timeout(value = 10)
public class CiStorageIntegrationTestBase {

    @Autowired
    protected CiDb db;

    @Autowired
    protected CiStorageDb ciStorageDb;

    @MockBean
    protected PCMService pcmService;

    @Mock
    protected ArcService arcService;

    private static final long SEED = -1321952185L;


    @Autowired
    protected StartAutocheckJob startAutocheckJob;

    @Autowired
    protected TvmClient tvmClient;

    @Autowired
    protected ArcanumClientImpl arcanumClient;

    @Autowired
    protected OverridableClock clock;

    @Autowired
    protected ClientAndServer staffServer;

    @Autowired
    protected StaffClient staffClient;

    @Autowired
    protected AutocheckBlacklistService autocheckBlacklistService;

    @Autowired
    protected StorageApiClient storageApiClient;

    @Autowired
    protected StorageApiController storageApiController;

    @Autowired
    protected ApiCheckService apiCheckService;

    @BeforeEach
    void setUp() {
        mockStaffResponses(false);
        autocheckBlacklistService.resetCache();
        doReturn(TestData.TRUNK_COMMIT_3.getRevision())
                .when(arcService).getLastRevisionInBranch(eq(ArcBranch.trunk()));
    }

    protected void setAutocheckEnabled() {
        db.currentOrTx(() ->
                db.keyValue().setValue(PostCommits.NAMESPACE, PostCommits.ENABLED, true)
        );
    }

    protected void mockStaffResponses(boolean isRobot) {
        when(staffClient.getStaffPerson(anyString())).thenReturn(new StaffPerson("", new StaffOfficial("", false)));

        Stream.of(TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_2)
                .map(ArcCommit::getAuthor)
                .forEach(author -> staffServer
                        .when(request("/v3/persons")
                                .withMethod(HttpMethod.GET.name())
                                .withQueryStringParameter("login", author)
                                .withQueryStringParameter("_one", "1")
                        )
                        .respond(response().withStatusCode(HttpStatusCode.OK_200.code())
                                .withBody(JsonBody.json("""
                                        {
                                            "login": "%s",
                                            "official": {
                                                "affiliation": "yandex",
                                                "is_robot": %s
                                            }
                                        }
                                        """.formatted(author, isRobot)
                                )))
                );
    }

    protected static TestJobContext createJobContext(CiProcessId processId, JsonObject flowVars) {
        final EnhancedRandom enhancedRandom = TestRandomUtils.enhancedRandom(SEED);

        var flowLaunchEntity = enhancedRandom.nextObject(FlowLaunchEntity.class).toBuilder()
                .processId(processId.asString())
                .launchNumber(42)
                .id(FlowLaunchId.of("flow-id"))
                .flowInfo(
                        enhancedRandom.nextObject(LaunchFlowInfo.class).toBuilder()
                                .flowId(FlowFullId.of(processId.getPath(), "autocheck"))
                                .flowVars(JobResource.optional(
                                        JobResourceType.of(PropertiesSubstitutor.FLOW_VARS_KEY), flowVars
                                ))
                                .build()
                )
                .vcsInfo(
                        LaunchVcsInfo.builder()
                                .previousRevision(TestData.TRUNK_COMMIT_2.toOrderedTrunkArcRevision())
                                .revision(TestData.TRUNK_COMMIT_3.toOrderedTrunkArcRevision())
                                .commit(TestData.TRUNK_COMMIT_3)
                                .build()
                )
                .jobs(Map.of())
                .build();

        return new TestJobContext(flowLaunchEntity);
    }

}
