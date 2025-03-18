package ru.yandex.ci.engine;

import java.nio.file.Path;
import java.time.Clock;
import java.time.Instant;
import java.time.ZoneOffset;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.context.ApplicationContext;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.arcanum.ArcanumTestServer;
import ru.yandex.ci.client.calendar.CalendarClient;
import ru.yandex.ci.client.sandbox.ProxySandboxClient;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.DelegationResultList;
import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.client.yav.model.DelegatingTokenResponse;
import ru.yandex.ci.client.yav.model.YavResponse;
import ru.yandex.ci.common.temporal.spring.TemporalTestConfig;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.abc.AbcServiceStub;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.te.TestenvDegradationManager;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.branch.BranchNameGenerator;
import ru.yandex.ci.engine.config.ConfigParseService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.config.validation.InputOutputTaskValidator;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommits;
import ru.yandex.ci.engine.discovery.DiscoveryServicePullRequests;
import ru.yandex.ci.engine.discovery.arc_reflog.ReflogProcessorService;
import ru.yandex.ci.engine.flow.SecurityDelegationService;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.OnCommitLaunchService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseService;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.engine.pcm.PCMSelector;
import ru.yandex.ci.engine.pcm.PCMService;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.spring.EngineTestConfig;
import ru.yandex.ci.engine.spring.tasks.EngineTasksConfig;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.scheduler.OnetimeTask;
import ru.yandex.commune.bazinga.test.BazingaTaskManagerStub;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@SuppressWarnings("NotNullFieldNotInitialized")
@ContextConfiguration(classes = {
        EngineTasksConfig.class,
        EngineTestConfig.class,
        TemporalTestConfig.class,
})
public class EngineTestBase extends YdbCiTestBase {

    protected static final Instant NOW = Clock.fixed(Instant.ofEpochSecond(1605098091), ZoneOffset.UTC).instant();

    @Autowired
    protected ArcanumClientImpl arcanumClient;

    @Autowired
    protected ArcanumTestServer arcanumTestServer;

    @MockBean
    protected TestenvClient testenvClient;

    @MockBean
    protected TestenvDegradationManager testenvDegradationManager;

    @SpyBean
    protected PullRequestService pullRequestService;

    @Autowired
    protected SandboxClient sandboxClient;

    @Autowired
    protected SandboxClient securitySandboxClient;

    @Autowired
    protected ProxySandboxClient proxySandboxClient;

    @Autowired
    protected YavClient yavClient;

    @SpyBean
    protected InputOutputTaskValidator taskValidator;

    @Autowired
    protected CalendarClient calendarClient;

    @Autowired
    protected OverridableClock clock;

    @Autowired
    protected LaunchService launchService;

    @Autowired
    protected OnCommitLaunchService onCommitLaunchService;

    @Autowired
    protected ConfigurationService configurationService;

    @Autowired
    protected SecurityDelegationService securityDelegationService;

    @Autowired
    protected ConfigParseService configParseService;

    @Autowired
    protected DiscoveryServicePullRequests discoveryServicePullRequests;

    @Autowired
    protected DiscoveryServicePostCommits discoveryServicePostCommits;

    @Autowired
    protected ArcService arcService;

    @Autowired
    protected AbcService abcService;

    @Autowired
    protected AutoReleaseService autoReleaseService;

    @Autowired
    protected DiscoveryProgressService discoveryProgressService;

    @SpyBean
    private BazingaTaskManager bazingaTaskManager;

    @SpyBean
    protected BranchNameGenerator branchNameGenerator;

    @Autowired
    protected ApplicationContext applicationContext;

    @Autowired
    protected ReflogProcessorService reflogProcessorService;

    @MockBean
    protected PCMService pcmService;

    @SpyBean
    protected PCMSelector pcmSelector;

    @Autowired
    protected EngineTester engineTester;

    protected BazingaTaskManagerStub bazingaTaskManagerStub;
    protected ArcServiceStub arcServiceStub;
    protected AbcServiceStub abcServiceStub;

    @BeforeEach
    protected void mockNow() {
        bazingaTaskManagerStub = (BazingaTaskManagerStub) bazingaTaskManager;
        if (arcService instanceof ArcServiceStub stub) {
            arcServiceStub = stub;
        }
        abcServiceStub = (AbcServiceStub) abcService;
        clock.setTime(NOW);
    }

    @BeforeEach
    protected void resetArcanumServer() {
        arcanumTestServer.reset();
        arcanumTestServer.mockSetMergeRequirement();
        arcanumTestServer.mockSetMergeRequirementStatus();
        arcanumTestServer.mockCreateReviewRequestComment();
    }

    @AfterEach
    protected void resetEngineTest() {
        bazingaTaskManagerStub.clearTasks();
    }

    @SafeVarargs
    protected final void executeBazingaTasks(Class<? extends OnetimeTask>... taskIds) {
        TaskId[] ids = Stream.of(taskIds)
                .map(TaskId::from)
                .toArray(TaskId[]::new);
        bazingaTaskManagerStub.executeTasks(applicationContext, ids);
    }

    protected void discoveryToR2() {
        discovery(TestData.TRUNK_COMMIT_2);
        if (arcService.isFileExists(AutocheckConstants.AUTOCHECK_A_YAML_PATH, TestData.TRUNK_COMMIT_2)) {
            delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);
        }
    }

    protected void discoveryToR3() {
        discoveryToR2();
        discovery(TestData.TRUNK_COMMIT_3);
    }

    protected void discoveryToR4() {
        discoveryToR3();
        discovery(TestData.TRUNK_COMMIT_4);
    }

    protected void discoveryToR5() {
        discoveryToR4();
        discovery(TestData.TRUNK_COMMIT_5);
    }

    protected void discoveryToR6() {
        discoveryToR5();
        discovery(TestData.TRUNK_COMMIT_6);
    }

    protected void discoveryToR7() {
        discoveryToR6();
        discovery(TestData.TRUNK_COMMIT_7);
    }

    protected void discoveryToR8() {
        discoveryToR7();
        discovery(TestData.TRUNK_COMMIT_8);
    }

    protected void saveCommit(ArcCommit commit) {
        db.currentOrTx(() -> db.arcCommit().save(commit));
    }

    protected void discovery(ArcCommit trunkCommit) {
        saveCommit(trunkCommit);
        OrderedArcRevision revision = trunkCommit.toOrderedTrunkArcRevision();
        discoveryServicePostCommits.processPostCommit(revision.getBranch(), revision.toRevision(), false);
    }

    protected void delegateToken(Path configPath) {
        delegateToken(configPath, ArcBranch.trunk());
    }

    protected void delegateToken(Path configPath, ArcBranch branch) {
        try {
            engineTester.delegateToken(configPath, branch);
        } catch (YavDelegationException e) {
            throw new RuntimeException(e);
        }
    }

    protected void mockYav() {
        engineTester.mockYav();
    }

    protected void mockSandboxDelegationAny() {
        when(securitySandboxClient.delegateYavSecret(any(), any()))
                .thenReturn(new DelegationResultList.DelegationResult(true, "ok", "delegated-secret"));
    }

    protected void mockYavAny() {
        when(yavClient.createDelegatingToken(anyString(), anyString(), anyString(), anyString()))
                .thenReturn(new DelegatingTokenResponse(YavResponse.Status.OK, null, null,
                        "tkn-test", "tkn-uid"));
    }

    protected void verifyYavDelegated(String secret, int times) {
        verify(yavClient, times(2)).createDelegatingToken(anyString(), eq(secret), anyString(), anyString());
    }

    protected void mockArcanumGetReviewRequestData() {
        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds1.json");
    }

    protected void mockValidationSuccessful() {
        doReturn(new InputOutputTaskValidator.Report(Map.of(), new LinkedHashMap<>()))
                .when(taskValidator).validate(any(), any(), any());
    }

    protected void initDiffSets() {
        var list = List.of(TestData.DIFF_SET_1, TestData.DIFF_SET_2, TestData.DIFF_SET_3);
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(list));
    }
}

