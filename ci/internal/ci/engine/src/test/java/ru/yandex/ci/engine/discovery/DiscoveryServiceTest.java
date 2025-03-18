package ru.yandex.ci.engine.discovery;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvFileSource;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.arc.api.Repo;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.arcanum.util.BodyVerificationMode;
import ru.yandex.ci.core.abc.Abc;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowDescription;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchUserData;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.event.LaunchEventTask;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.engine.launch.LaunchStartTask;
import ru.yandex.ci.engine.pr.CreatePrCommentTask;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

public class DiscoveryServiceTest extends EngineTestBase {

    private Path autocheckYaml;

    @Autowired
    protected DiscoveryServicePullRequestsTriggers triggers;

    @BeforeEach
    void setUp() {
        mockYav();
        mockValidationSuccessful();
        mockArcanumGetReviewRequestData();
        arcServiceStub.resetAndInitTestData();
        autocheckYaml = AutocheckConstants.AUTOCHECK_A_YAML_PATH;
    }

    @Test
    public void testAuthors() {
        var bundle = Mockito.mock(ConfigBundle.class);
        when(bundle.getConfigPath()).thenReturn(Path.of("a.yaml"));
        abcServiceStub.addService(Abc.TE);

        OrderedArcRevision revision = trunkRevision();
        ArcCommit commit = ArcCommit.builder().author(TestData.CI_USER).build();

        DiscoveryContext context = DiscoveryContext.builder()
                .revision(revision)
                .commit(commit)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(List.of(""))
                .discoveryType(DiscoveryType.DIR)
                .configBundle(bundle)
                .build();

        FilterConfig.Discovery discovery = FilterConfig.Discovery.DIR;

        FilterConfig authorFilter = FilterConfig.builder()
                .discovery(discovery)
                .notAuthorServices(List.of("junk", "testenv"))
                .build();

        assertThat(triggers.isMatchesAnyFilter(context, List.of(authorFilter))).isTrue();

        FilterConfig wrongAuthorFilter = FilterConfig.builder()
                .discovery(discovery)
                .notAuthorServices(List.of("ci", "junk", "testenv"))
                .build();

        assertThat(triggers.isMatchesAnyFilter(context, List.of(wrongAuthorFilter))).isFalse();
    }

    @Test
    public void filterWrongAuthorTest() {
        abcServiceStub.addService(Abc.TE);

        OrderedArcRevision revision = trunkRevision();
        ArcCommit commit = arcCommit(revision);

        DiscoveryContext context = DiscoveryContext.builder()
                .revision(revision)
                .commit(commit)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(List.of(""))
                .discoveryType(DiscoveryType.DIR)
                .build();

        FilterConfig.Discovery discovery = FilterConfig.Discovery.DIR;

        // filter doesn't suite cause of commit has author=ci
        FilterConfig invalidServiceFilterConfig = FilterConfig.builder()
                .discovery(discovery)
                .authorServices(List.of("testenv"))
                .build();

        // filter doesn't suite cause of commit has discoveryType=dir
        FilterConfig invalidDiscoveryFilterConfig = FilterConfig.builder()
                .discovery(FilterConfig.Discovery.GRAPH)
                .authorServices(List.of("ci"))
                .build();

        assertThat(
                triggers.isMatchesAnyFilter(
                        context,
                        List.of(invalidServiceFilterConfig, invalidDiscoveryFilterConfig)
                )
        ).isFalse();
    }

    @Test
    public void testNotAuthors() {
        var bundle = Mockito.mock(ConfigBundle.class);
        when(bundle.getConfigPath()).thenReturn(Path.of("a.yaml"));

        OrderedArcRevision revision = trunkRevision();
        ArcCommit commit = ArcCommit.builder().author(TestData.CI_USER).build();

        DiscoveryContext context = DiscoveryContext.builder()
                .revision(revision)
                .commit(commit)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(List.of(""))
                .discoveryType(DiscoveryType.DIR)
                .configBundle(bundle)
                .build();

        FilterConfig.Discovery discovery = FilterConfig.Discovery.DIR;

        FilterConfig notAuthorsFilter = FilterConfig.builder()
                .discovery(discovery)
                .notAuthors(List.of("ci"))
                .build();

        assertThat(triggers.isMatchesAnyFilter(context, List.of(notAuthorsFilter))).isTrue();

        FilterConfig wrongNotAuthorsFilter = FilterConfig.builder()
                .discovery(discovery)
                .notAuthors(List.of(TestData.CI_USER))
                .build();

        assertThat(triggers.isMatchesAnyFilter(context, List.of(wrongNotAuthorsFilter))).isFalse();
    }

    private static OrderedArcRevision trunkRevision() {
        return OrderedArcRevision.fromHash("abc", ArcBranch.trunk(), 1L, 0);
    }

    private static ArcCommit arcCommit(OrderedArcRevision revision) {
        return arcCommit(revision, "");
    }

    private static ArcCommit arcCommit(OrderedArcRevision revision, String message) {
        return ArcCommit.builder()
                .id(revision.toCommitId())
                .author(TestData.CI_USER)
                .message(message)
                .createTime(null)
                .parents(List.of())
                .svnRevision(1L)
                .build();
    }

    @Test
    public void filterWrongStQueueTest() {
        OrderedArcRevision revision = trunkRevision();
        ArcCommit commit = arcCommit(revision, "CI-818");

        DiscoveryContext context = DiscoveryContext.builder()
                .revision(revision)
                .commit(commit)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(List.of())
                .discoveryType(DiscoveryType.DIR)
                .build();

        FilterConfig.Discovery discovery = FilterConfig.Discovery.DIR;

        List<FilterConfig> filterConfigs = List.of(
                FilterConfig.builder()
                        .discovery(discovery)
                        .stQueues(List.of("TESTENV"))
                        .build()
        );

        assertThat(
                triggers.isMatchesAnyFilter(context, filterConfigs)
        ).isFalse();
    }

    @Test
    public void filterWrongPathsTest() {
        OrderedArcRevision revision = trunkRevision();
        ArcCommit commit = arcCommit(revision);

        var bundle = Mockito.mock(ConfigBundle.class);
        when(bundle.getConfigPath()).thenReturn(Path.of("a.yaml"));

        DiscoveryContext context = DiscoveryContext.builder()
                .revision(revision)
                .commit(commit)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(List.of("a/b/c.py"))
                .discoveryType(DiscoveryType.DIR)
                .configBundle(bundle)
                .build();

        FilterConfig.Discovery discovery = FilterConfig.Discovery.DIR;

        List<FilterConfig> filterConfigs = List.of(
                FilterConfig.builder()
                        .discovery(discovery)
                        .subPaths(List.of("**.java", "ya.make"))
                        .build()
        );

        assertThat(
                triggers.isMatchesAnyFilter(context, filterConfigs)
        ).isFalse();
    }

    @Test
    public void filterCorrectCommitTest() {
        OrderedArcRevision revision = trunkRevision();
        ArcCommit commit = arcCommit(revision, "CI-818");

        var bundle = Mockito.mock(ConfigBundle.class);
        when(bundle.getConfigPath()).thenReturn(Path.of("a.yaml"));

        DiscoveryContext context = DiscoveryContext.builder()
                .revision(revision)
                .commit(commit)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(List.of("ci/test/test.java", "ya.make"))
                .discoveryType(DiscoveryType.DIR)
                .configBundle(bundle)
                .build();

        FilterConfig.Discovery discovery = FilterConfig.Discovery.DIR;


        FilterConfig invalidServiceFilterConfig = FilterConfig.builder()
                .discovery(discovery)
                .stQueues(List.of("CI"))
                .authorServices(List.of("testenv"))
                .subPaths(List.of("ya.make"))
                .build();

        FilterConfig invalidDiscoveryFilterConfig = FilterConfig.builder()
                .discovery(FilterConfig.Discovery.GRAPH)
                .authorServices(List.of("testenv"))
                .build();

        FilterConfig correctFilterConfig = FilterConfig.builder()
                .discovery(discovery)
                .authorServices(List.of("ci"))
                .subPaths(List.of("**.java"))
                .build();


        assertThat(
                triggers.isMatchesAnyFilter(
                        context,
                        List.of(invalidDiscoveryFilterConfig, invalidServiceFilterConfig, correctFilterConfig)
                )
        ).isTrue();
    }

    @Test
    void filterSubPathTest() {

        var bundle = Mockito.mock(ConfigBundle.class);
        when(bundle.getConfigPath()).thenReturn(Path.of("a/a.yaml"));

        DiscoveryContext discoveryContext = DiscoveryContext.builder()
                .revision(TestData.TRUNK_R2)
                .commit(TestData.TRUNK_COMMIT_2)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(List.of("a/b/file1.txt", "c/d/file1.txt"))
                .discoveryType(DiscoveryType.DIR)
                .configBundle(bundle)
                .build();

        FilterConfig filterConfig = FilterConfig.builder()
                .discovery(FilterConfig.Discovery.ANY)
                .subPaths(List.of("b/**"))
                .build();

        assertThat(
                triggers.isMatchesAnyFilter(
                        discoveryContext,
                        List.of(filterConfig)
                )
        ).isTrue();

        when(bundle.getConfigPath()).thenReturn(Path.of("c.yaml"));

        assertThat(
                triggers.isMatchesAnyFilter(
                        discoveryContext,
                        List.of(filterConfig)
                )
        ).isFalse();

    }

    @Test
    void filterBranchName() {

        var bundle = Mockito.mock(ConfigBundle.class);
        when(bundle.getConfigPath()).thenReturn(Path.of("a/a.yaml"));

        DiscoveryContext discoveryContext = DiscoveryContext.builder()
                .revision(TestData.TRUNK_R2)
                .commit(TestData.TRUNK_COMMIT_2)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(List.of("a/b/file1.txt", "c/d/file1.txt"))
                .discoveryType(DiscoveryType.DIR)
                .configBundle(bundle)
                .build();

        discoveryContext = discoveryContext.withFeatureBranch(ArcBranch.ofBranchName("users/username/feature-CI-1"));

        FilterConfig filterConfig = FilterConfig.builder()
                .discovery(FilterConfig.Discovery.ANY)
                .featureBranches(List.of("**/feature-**"))
                .notFeatureBranch("**/feature-X**")
                .build();

        assertThat(
                triggers.isMatchesAnyFilter(
                        discoveryContext,
                        List.of(filterConfig)
                )
        ).isTrue();

        discoveryContext = discoveryContext.withFeatureBranch(ArcBranch.ofBranchName("user/feature-X1"));

        assertThat(
                triggers.isMatchesAnyFilter(
                        discoveryContext,
                        List.of(filterConfig)
                )
        ).isFalse();

        discoveryContext = discoveryContext.withFeatureBranch(ArcBranch.ofBranchName("users/username/test"));
        assertThat(
                triggers.isMatchesAnyFilter(
                        discoveryContext,
                        List.of(filterConfig)
                )
        ).isFalse();

        discoveryContext = discoveryContext.withFeatureBranch(null);
        assertThat(
                triggers.isMatchesAnyFilter(
                        discoveryContext,
                        List.of(filterConfig)
                )
        ).isFalse();

    }

    @Test
    void getAffectedPathsCalledEvenNoPathFilters() {
        var bundle = Mockito.mock(ConfigBundle.class);
        when(bundle.getConfigPath()).thenReturn(Path.of("a.yaml"));

        var affectedPathsCalled = new AtomicBoolean();
        DiscoveryContext discoveryContext = DiscoveryContext.builder()
                .revision(TestData.TRUNK_R2)
                .commit(TestData.TRUNK_COMMIT_2)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(() -> {
                    affectedPathsCalled.set(true);
                    return List.of("");
                })
                .discoveryType(DiscoveryType.DIR)
                .configBundle(bundle)
                .build();

        assertThat(
                triggers.isMatchesAnyFilter(
                        discoveryContext,
                        List.of(FilterConfig.builder().build())
                )
        ).isTrue();
        assertThat(affectedPathsCalled).isTrue();
    }

    @Test
    void aYamlArcanumMessages() {
        discoveryToR2();

        PullRequestDiffSet diffSet = TestData.DIFF_SET_5;
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(diffSet));
        discoveryServicePullRequests.processDiffSet(diffSet, false);

        executeBazingaTasks(CreatePrCommentTask.class);

        var wrongExt = """
                Предупреждение от CI
                ==
                Обратите внимание, что в пулл-реквесте был затронут файл ***pr/a.yml***.
                Он имеет расширение **.yml**, однако только файлы **a.yaml** являются репозиторными конфигами Аркадии.
                Возможно, имя было выбрано ошибочно. Переименуйте в **a.yaml**, \
                в противном случае файл не будет обработан CI.

                ---
                CI Warning
                ==
                Please note that following file was affected in PR: ***pr/a.yml***.
                Files **a.yml** are not Arcadia repository configs, only **a.yaml** are.
                Probably name **a.yml** was selected by mistake, change it to **a.yaml**.
                CI doesn't process **a.yml** files.""";
        arcanumTestServer.verifyCreateReviewRequestComment(42, wrongExt);
    }

    @Test
    void delayedPreCommitLaunch() throws YavDelegationException {
        discoveryToR2();

        PullRequestDiffSet diffSet = TestData.DIFF_SET_1;
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(diffSet));

        discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_1, false);

        Launch expectedDelayedLaunch = launch(diffSet, LaunchRuntimeInfo.ofRuntimeSandboxOwner("CI"),
                LaunchState.Status.DELAYED);

        OrderedArcRevision configRevision = expectedDelayedLaunch.getFlowInfo().getConfigRevision();

        db.currentOrReadOnly(() ->
                assertThat(db.launches().get(expectedDelayedLaunch.getLaunchId()))
                        .isEqualTo(expectedDelayedLaunch)
        );

        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchEventTask) &&
                        expectedDelayedLaunch.getLaunchId().equals(
                                ((LaunchEventTask.Params) task.getParameters()).getLaunchId()
                        ) &&
                        LaunchState.Status.DELAYED.equals(
                                ((LaunchEventTask.Params) task.getParameters()).getLaunchStatus()
                        )
        ));

        Path configPath = expectedDelayedLaunch.getLaunchId().getProcessId().getPath();

        assertThat(db.currentOrReadOnly(() -> db.launches().getDelayedLaunchIds(configPath, configRevision)))
                .isEqualTo(List.of(expectedDelayedLaunch.getId()));

        securityDelegationService.delegateYavTokens(
                configurationService.getConfig(configPath, configRevision),
                TestData.USER_TICKET,
                "andreevdm"
        );

        launchService.startDelayedLaunches(configPath, configRevision.toRevision());
        Mockito.verify(bazingaTaskManagerStub).schedule(Mockito.any(LaunchStartTask.class));

        Launch expectedStaringLaunch = launch(
                diffSet,
                LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"),
                LaunchState.Status.STARTING
        );

        db.currentOrReadOnly(() ->
                assertThat(db.launches().findOptional(expectedDelayedLaunch.getLaunchId()))
                        .contains(expectedStaringLaunch)
        );
    }

    private Launch launch(PullRequestDiffSet diffSet,
                          LaunchRuntimeInfo runtimeInfo,
                          LaunchState.Status status) {

        CiProcessId ciProcessId = CiProcessId.ofFlow(TestData.PR_NEW_CONFIG_SAWMILL_FLOW_ID);
        LaunchId expectedLaunchId = LaunchId.of(ciProcessId, 1);

        return Launch.builder()
                .launchId(expectedLaunchId)
                .title("Woodcutter #1")
                .type(Launch.Type.USER)
                .notifyPullRequest(true)
                .project("ci")
                .flowInfo(
                        LaunchFlowInfo.builder()
                                .configRevision(diffSet.getOrderedMergeRevision())
                                .flowId(TestData.PR_NEW_CONFIG_SAWMILL_FLOW_ID)
                                .runtimeInfo(runtimeInfo)
                                .flowDescription(
                                        new LaunchFlowDescription(
                                                "Woodcutter",
                                                "sawmill flow",
                                                Common.FlowType.FT_DEFAULT,
                                                null)
                                )
                                .build()
                )
                .hasDisplacement(false)
                .triggeredBy("andreevdm")
                .created(NOW)
                .activityChanged(NOW)
                .status(status)
                .statusText("")
                .vcsInfo(
                        LaunchVcsInfo.builder()
                                .revision(diffSet.getOrderedMergeRevision())
                                .previousRevision(
                                        arcService.getCommit(diffSet.getVcsInfo().getUpstreamRevision())
                                                .toOrderedTrunkArcRevision()
                                )
                                .pullRequestInfo(new LaunchPullRequestInfo(
                                        diffSet.getPullRequestId(),
                                        diffSet.getDiffSetId(),
                                        TestData.CI_USER,
                                        "CI-42 New CI configs",
                                        "Description",
                                        ArcanumMergeRequirementId.of("CI", "pr/new: Woodcutter"),
                                        diffSet.getVcsInfo(),
                                        diffSet.getIssues(),
                                        diffSet.getLabels(),
                                        null
                                ))
                                .releaseVcsInfo(null)
                                .commit(TestData.DS1_COMMIT
                                        .withParents(List.of(TestData.TRUNK_R2.getCommitId()))
                                )
                                .build()
                )
                .userData(new LaunchUserData(List.of(), false))
                .version(Version.major(Integer.toString(expectedLaunchId.getNumber())))
                .build();
    }

    @ParameterizedTest
    @CsvFileSource(delimiter = ';', resources = "value-meets-criteria.csv")
    void valueMeetsCriteria(String positiveSamples, String negativeSamples, boolean expected) {
        boolean actual = DiscoveryServiceFilters.valueMeetsCriteria(
                commaSeparatedStringToList(positiveSamples),
                commaSeparatedStringToList(negativeSamples),
                "found"::equals);
        assertThat(actual)
                .withFailMessage(
                        "input (positiveSamples \"%s\", negativeSamples: \"%s\" got %s, expected %s",
                        positiveSamples, negativeSamples, actual, expected
                )
                .isEqualTo(expected);
    }

    private List<String> commaSeparatedStringToList(@Nullable String source) {
        if (source == null) {
            return List.of();
        }
        return List.of(source.split(",\\s*"));
    }

    @Test
    void notifyPullRequestAboutSecurityProblems_whenOwnerEditsConfig() {
        arcServiceStub.reset();
        arcServiceStub.addFirstCommit();

        ArcCommit trunkCommitR2 = TestData.TRUNK_COMMIT_2.withAuthor(TestData.CI_USER);
        PullRequestDiffSet pullRequestDiffSet = TestData.DIFF_SET_1.withAuthor(TestData.CI_USER);
        ArcCommit prCommit = TestData.REVISION_COMMIT.withAuthor(TestData.CI_USER);
        ArcCommit prMergeCommit = TestData.DS1_COMMIT.withAuthor(TestData.CI_USER);

        // prepare TRUNK_COMMIT_1
        var noTriggeredFlowsAndLargeAutostart = Path.of("notify-pr-about-security-problems/" +
                "no-triggered-flows-and-large-autostart/a.yaml");
        var hasTriggeredFlows = Path.of("notify-pr-about-security-problems/has-triggered-flows/a.yaml");
        var hasLargeAutostart = Path.of("notify-pr-about-security-problems/has-autocheck-large-autostart/a.yaml");
        var deleteCiSection = Path.of("notify-pr-about-security-problems/delete-ci-section/a.yaml");

        arcServiceStub.addCommit(trunkCommitR2, TestData.TRUNK_COMMIT_1,
                Map.of(
                        noTriggeredFlowsAndLargeAutostart, Repo.ChangelistResponse.ChangeType.Add,
                        hasTriggeredFlows, Repo.ChangelistResponse.ChangeType.Add,
                        hasLargeAutostart, Repo.ChangelistResponse.ChangeType.Add,
                        deleteCiSection, Repo.ChangelistResponse.ChangeType.Add,
                        autocheckYaml, Repo.ChangelistResponse.ChangeType.Add
                )
        );
        discovery(trunkCommitR2);

        delegateToken(noTriggeredFlowsAndLargeAutostart);
        delegateToken(hasTriggeredFlows);
        delegateToken(hasLargeAutostart);
        assertThat(
                Stream.of(noTriggeredFlowsAndLargeAutostart, hasTriggeredFlows, hasLargeAutostart)
                        .map(path -> configurationService.getLastConfig(path, ArcBranch.trunk()).getStatus())
                        .collect(Collectors.toSet())
        ).isEqualTo(Set.of(ConfigStatus.READY));

        // prepare pull request
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(pullRequestDiffSet));
        var changes = Map.of(
                noTriggeredFlowsAndLargeAutostart, Repo.ChangelistResponse.ChangeType.Modify,
                hasTriggeredFlows, Repo.ChangelistResponse.ChangeType.Modify,
                hasLargeAutostart, Repo.ChangelistResponse.ChangeType.Modify,
                deleteCiSection, Repo.ChangelistResponse.ChangeType.Modify
        );
        arcServiceStub.addCommit(prCommit, trunkCommitR2, changes);
        arcServiceStub.addCommit(prMergeCommit, trunkCommitR2, changes);

        // test method
        discoveryServicePullRequests.processDiffSet(pullRequestDiffSet);

        // verify
        verify(pullRequestService, times(0)).sendSecurityProblemComment(eq(42L), any());
        assertThat(
                Stream.of(noTriggeredFlowsAndLargeAutostart, hasTriggeredFlows, hasLargeAutostart)
                        .map(path -> configurationService.getLastConfig(path, ArcBranch.trunk()).getStatus())
                        .collect(Collectors.toSet())
        ).isEqualTo(Set.of(ConfigStatus.READY));

        assertThat(
                Stream.of(deleteCiSection)
                        .map(path -> configurationService.getLastConfig(path, ArcBranch.trunk()).getStatus())
                        .collect(Collectors.toSet())
        ).isEqualTo(Set.of(ConfigStatus.SECURITY_PROBLEM));

    }

    @Test
    void doNotSendValidationStatusToArcanum_aboutNotCiConfigs_whenConfigIsNotChanged() {
        arcServiceStub.reset();
        arcServiceStub.addFirstCommit();

        ArcCommit trunkCommitR1 = TestData.TRUNK_COMMIT_2.withAuthor(TestData.CI_USER);
        PullRequestDiffSet pullRequestDiffSet = TestData.DIFF_SET_1.withAuthor(TestData.CI_USER);
        ArcCommit prCommit = TestData.REVISION_COMMIT.withAuthor(TestData.CI_USER);
        ArcCommit prMergeCommit = TestData.DS1_COMMIT.withAuthor(TestData.CI_USER);

        // prepare TRUNK_COMMIT_1
        var notCiConfigNotModified = Path.of("not/ci/not-modified/a.yaml");

        arcServiceStub.addCommit(
                trunkCommitR1, TestData.TRUNK_COMMIT_1,
                Map.of(
                        notCiConfigNotModified, Repo.ChangelistResponse.ChangeType.Add,
                        autocheckYaml, Repo.ChangelistResponse.ChangeType.Add
                )
        );
        discovery(trunkCommitR1);

        assertThat(
                configurationService.getLastConfig(notCiConfigNotModified, ArcBranch.trunk()).getStatus()
        ).isEqualByComparingTo(ConfigStatus.NOT_CI);

        // prepare pull request
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(pullRequestDiffSet));
        var changes = Map.of(
                Path.of("not/ci/not-modified/dummy"), Repo.ChangelistResponse.ChangeType.Add
        );
        arcServiceStub.addCommit(prCommit, trunkCommitR1, changes);
        arcServiceStub.addCommit(prMergeCommit, trunkCommitR1, changes);

        // test method
        discoveryServicePullRequests.processDiffSet(pullRequestDiffSet);

        // verify
        verify(pullRequestService, times(0)).sendConfigBypassResult(eq(42L), eq(1L), any());
    }

    @Test
    void sendValidationStatusToArcanum_aboutNotCiConfigs_whenConfigChanged() {
        arcServiceStub.reset();
        arcServiceStub.addFirstCommit();

        ArcCommit trunkCommitR1 = TestData.TRUNK_COMMIT_2.withAuthor(TestData.CI_USER);
        PullRequestDiffSet pullRequestDiffSet = TestData.DIFF_SET_1.withAuthor(TestData.CI_USER);
        ArcCommit prCommit = TestData.REVISION_COMMIT.withAuthor(TestData.CI_USER);
        ArcCommit prMergeCommit = TestData.DS1_COMMIT.withAuthor(TestData.CI_USER);

        // prepare TRUNK_COMMIT_1
        var notCiConfigModified = Path.of("not/ci/modified/a.yaml");
        var notCiConfigAdded = Path.of("not/ci/added/a.yaml");

        arcServiceStub.addCommit(
                trunkCommitR1, TestData.TRUNK_COMMIT_1,
                Map.of(notCiConfigModified, Repo.ChangelistResponse.ChangeType.Add)
        );
        discovery(trunkCommitR1);

        assertThat(
                configurationService.getLastConfig(notCiConfigModified, ArcBranch.trunk()).getStatus()
        ).isEqualByComparingTo(ConfigStatus.NOT_CI);

        // prepare pull request
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(pullRequestDiffSet));

        var changes = Map.of(
                notCiConfigModified, Repo.ChangelistResponse.ChangeType.Modify,
                notCiConfigAdded, Repo.ChangelistResponse.ChangeType.Add,
                autocheckYaml, Repo.ChangelistResponse.ChangeType.Add
        );
        arcServiceStub.addCommit(prCommit, trunkCommitR1, changes);
        arcServiceStub.addCommit(prMergeCommit, trunkCommitR1, changes);

        // test method
        discoveryServicePullRequests.processDiffSet(pullRequestDiffSet);

        // verify
        verify(pullRequestService, times(2)).sendConfigBypassResult(eq(42L), eq(1L), any());
    }

    @Test
    void notifyPullRequestAboutSecurityProblems_whenNotOwnerEditsConfig() {
        arcServiceStub.reset();
        arcServiceStub.addFirstCommit();

        ArcCommit trunkCommitR1 = TestData.TRUNK_COMMIT_2.withAuthor(TestData.CI_USER);
        PullRequestDiffSet pullRequestDiffSet = TestData.DIFF_SET_1.withAuthor("not_owner");
        ArcCommit prCommit = TestData.REVISION_COMMIT.withAuthor("not_owner");
        ArcCommit prMergeCommit = TestData.DS1_COMMIT.withAuthor("not_owner");

        // prepare TRUNK_COMMIT_1
        var noTriggeredFlowsAndLargeAutostart = Path.of("notify-pr-about-security-problems/" +
                "no-triggered-flows-and-large-autostart/a.yaml");
        var hasTriggeredFlows = Path.of("notify-pr-about-security-problems/has-triggered-flows/a.yaml");
        var hasLargeAutostart = Path.of("notify-pr-about-security-problems/has-autocheck-large-autostart/a.yaml");
        var deleteCiSection = Path.of("notify-pr-about-security-problems/delete-ci-section/a.yaml");

        arcServiceStub.addCommit(trunkCommitR1, TestData.TRUNK_COMMIT_1,
                Map.of(
                        noTriggeredFlowsAndLargeAutostart, Repo.ChangelistResponse.ChangeType.Add,
                        hasTriggeredFlows, Repo.ChangelistResponse.ChangeType.Add,
                        hasLargeAutostart, Repo.ChangelistResponse.ChangeType.Add,
                        deleteCiSection, Repo.ChangelistResponse.ChangeType.Add,
                        autocheckYaml, Repo.ChangelistResponse.ChangeType.Add
                )
        );
        discovery(trunkCommitR1);

        delegateToken(noTriggeredFlowsAndLargeAutostart);
        delegateToken(hasTriggeredFlows);
        delegateToken(hasLargeAutostart);
        assertThat(
                Stream.of(noTriggeredFlowsAndLargeAutostart, hasTriggeredFlows, hasLargeAutostart)
                        .map(path -> configurationService.getLastConfig(path, ArcBranch.trunk()).getStatus())
                        .collect(Collectors.toSet())
        ).isEqualTo(Set.of(ConfigStatus.READY));

        // prepare pull request
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(pullRequestDiffSet));
        var changes = Map.of(
                noTriggeredFlowsAndLargeAutostart, Repo.ChangelistResponse.ChangeType.Modify,
                hasTriggeredFlows, Repo.ChangelistResponse.ChangeType.Modify,
                hasLargeAutostart, Repo.ChangelistResponse.ChangeType.Modify,
                deleteCiSection, Repo.ChangelistResponse.ChangeType.Modify
        );
        arcServiceStub.addCommit(prCommit, trunkCommitR1, changes);
        arcServiceStub.addCommit(prMergeCommit, trunkCommitR1, changes);

        // test method
        discoveryServicePullRequests.processDiffSet(pullRequestDiffSet);

        // verify
        verify(pullRequestService, times(1)).sendSecurityProblemComment(
                eq(42L), argThat(it -> noTriggeredFlowsAndLargeAutostart.equals(it.getConfigPath()))
        );
        verify(pullRequestService, times(1)).sendSecurityProblemComment(
                eq(42L), argThat(it -> hasTriggeredFlows.equals(it.getConfigPath()))
        );
        verify(pullRequestService, times(1)).sendSecurityProblemComment(
                eq(42L), argThat(it -> hasLargeAutostart.equals(it.getConfigPath()))
        );
        verify(pullRequestService, times(0)).sendSecurityProblemComment(
                eq(42L), argThat(it -> deleteCiSection.equals(it.getConfigPath()))
        );
    }

    @Test
    void notifyPullRequestAboutSecurityProblems_whenOwnerEditsConfigAndTrunkConfigHasNoToken() {
        arcServiceStub.reset();
        arcServiceStub.addFirstCommit();

        ArcCommit trunkCommitR1 = TestData.TRUNK_COMMIT_2.withAuthor(TestData.CI_USER);
        PullRequestDiffSet pullRequestDiffSet = TestData.DIFF_SET_1.withAuthor(TestData.CI_USER);
        ArcCommit prCommit = TestData.REVISION_COMMIT.withAuthor(TestData.CI_USER);
        ArcCommit prMergeCommit = TestData.DS1_COMMIT.withAuthor(TestData.CI_USER);

        // prepare TRUNK_COMMIT_1
        var noTriggeredFlowsAndLargeAutostart = Path.of("notify-pr-about-security-problems/" +
                "no-triggered-flows-and-large-autostart/a.yaml");
        var hasTriggeredFlows = Path.of("notify-pr-about-security-problems/has-triggered-flows/a.yaml");
        var hasLargeAutostart = Path.of("notify-pr-about-security-problems/has-autocheck-large-autostart/a.yaml");
        var deleteCiSection = Path.of("notify-pr-about-security-problems/delete-ci-section/a.yaml");

        arcServiceStub.addCommit(trunkCommitR1, TestData.TRUNK_COMMIT_1,
                Map.of(
                        noTriggeredFlowsAndLargeAutostart, Repo.ChangelistResponse.ChangeType.Add,
                        hasTriggeredFlows, Repo.ChangelistResponse.ChangeType.Add,
                        hasLargeAutostart, Repo.ChangelistResponse.ChangeType.Add,
                        deleteCiSection, Repo.ChangelistResponse.ChangeType.Add,
                        autocheckYaml, Repo.ChangelistResponse.ChangeType.Add
                )
        );
        discovery(trunkCommitR1);

        assertThat(
                Stream.of(noTriggeredFlowsAndLargeAutostart, hasTriggeredFlows, hasLargeAutostart)
                        .map(path -> configurationService.getLastConfig(path, ArcBranch.trunk()).getStatus())
                        .collect(Collectors.toSet())
        ).isEqualTo(Set.of(ConfigStatus.SECURITY_PROBLEM));

        // prepare pull request
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(pullRequestDiffSet));
        var changes = Map.of(
                noTriggeredFlowsAndLargeAutostart, Repo.ChangelistResponse.ChangeType.Modify,
                hasTriggeredFlows, Repo.ChangelistResponse.ChangeType.Modify,
                hasLargeAutostart, Repo.ChangelistResponse.ChangeType.Modify,
                deleteCiSection, Repo.ChangelistResponse.ChangeType.Modify
        );
        arcServiceStub.addCommit(prCommit, trunkCommitR1, changes);
        arcServiceStub.addCommit(prMergeCommit, trunkCommitR1, changes);

        // test method
        discoveryServicePullRequests.processDiffSet(pullRequestDiffSet);

        // verify
        verify(pullRequestService, times(1)).sendSecurityProblemComment(
                eq(42L), argThat(it -> noTriggeredFlowsAndLargeAutostart.equals(it.getConfigPath()))
        );
        verify(pullRequestService, times(1)).sendSecurityProblemComment(
                eq(42L), argThat(it -> hasTriggeredFlows.equals(it.getConfigPath()))
        );
        verify(pullRequestService, times(1)).sendSecurityProblemComment(
                eq(42L), argThat(it -> hasLargeAutostart.equals(it.getConfigPath()))
        );
        verify(pullRequestService, times(0)).sendSecurityProblemComment(
                eq(42L), argThat(it -> deleteCiSection.equals(it.getConfigPath()))
        );
    }

    @Test
    void notifyPullRequestAboutSecurityProblems_whenNotOwnerEditsConfigAndTrunkConfigHasNoToken() {
        arcServiceStub.reset();
        arcServiceStub.addFirstCommit();

        ArcCommit trunkCommitR1 = TestData.TRUNK_COMMIT_2.withAuthor(TestData.CI_USER);
        PullRequestDiffSet pullRequestDiffSet = TestData.DIFF_SET_1.withAuthor("not_owner");
        ArcCommit prCommit = TestData.REVISION_COMMIT.withAuthor("not_owner");
        ArcCommit prMergeCommit = TestData.DS1_COMMIT.withAuthor("not_owner");

        // prepare TRUNK_COMMIT_1
        var noTriggeredFlowsAndLargeAutostart = Path.of("notify-pr-about-security-problems/" +
                "no-triggered-flows-and-large-autostart/a.yaml");
        var hasTriggeredFlows = Path.of("notify-pr-about-security-problems/has-triggered-flows/a.yaml");
        var hasLargeAutostart = Path.of("notify-pr-about-security-problems/has-autocheck-large-autostart/a.yaml");
        var deleteCiSection = Path.of("notify-pr-about-security-problems/delete-ci-section/a.yaml");

        arcServiceStub.addCommit(trunkCommitR1, TestData.TRUNK_COMMIT_1,
                Map.of(
                        noTriggeredFlowsAndLargeAutostart, Repo.ChangelistResponse.ChangeType.Add,
                        hasTriggeredFlows, Repo.ChangelistResponse.ChangeType.Add,
                        hasLargeAutostart, Repo.ChangelistResponse.ChangeType.Add,
                        deleteCiSection, Repo.ChangelistResponse.ChangeType.Add,
                        autocheckYaml, Repo.ChangelistResponse.ChangeType.Add
                )
        );
        discovery(trunkCommitR1);

        assertThat(
                Stream.of(noTriggeredFlowsAndLargeAutostart, hasTriggeredFlows, hasLargeAutostart)
                        .map(path -> configurationService.getLastConfig(path, ArcBranch.trunk()).getStatus())
                        .collect(Collectors.toSet())
        ).isEqualTo(Set.of(ConfigStatus.SECURITY_PROBLEM));

        // prepare pull request
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(pullRequestDiffSet));
        var changes = Map.of(
                noTriggeredFlowsAndLargeAutostart, Repo.ChangelistResponse.ChangeType.Modify,
                hasTriggeredFlows, Repo.ChangelistResponse.ChangeType.Modify,
                hasLargeAutostart, Repo.ChangelistResponse.ChangeType.Modify,
                deleteCiSection, Repo.ChangelistResponse.ChangeType.Modify
        );
        arcServiceStub.addCommit(prCommit, trunkCommitR1, changes);
        arcServiceStub.addCommit(prMergeCommit, trunkCommitR1, changes);

        // test method
        discoveryServicePullRequests.processDiffSet(pullRequestDiffSet);

        // verify
        verify(pullRequestService, times(1)).sendSecurityProblemComment(
                eq(42L), argThat(it -> noTriggeredFlowsAndLargeAutostart.equals(it.getConfigPath()))
        );
        verify(pullRequestService, times(1)).sendSecurityProblemComment(
                eq(42L), argThat(it -> hasTriggeredFlows.equals(it.getConfigPath()))
        );
        verify(pullRequestService, times(1)).sendSecurityProblemComment(
                eq(42L), argThat(it -> hasLargeAutostart.equals(it.getConfigPath()))
        );
        verify(pullRequestService, times(0)).sendSecurityProblemComment(
                eq(42L), argThat(it -> deleteCiSection.equals(it.getConfigPath()))
        );

        executeBazingaTasks(CreatePrCommentTask.class);
        arcanumTestServer.verifyCreateReviewRequestComment(42, ".*", BodyVerificationMode.REGEXP);

        var delegationRequired = TestUtils.textResource("discovery/delegation.txt").trim();
        arcanumTestServer.verifyCreateReviewRequestComment(42, delegationRequired);
    }

}
