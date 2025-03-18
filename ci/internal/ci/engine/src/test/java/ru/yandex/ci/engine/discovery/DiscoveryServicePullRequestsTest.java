package ru.yandex.ci.engine.discovery;

import java.nio.file.Path;
import java.util.List;

import com.google.errorprone.annotations.CanIgnoreReturnValue;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockserver.verify.VerificationTimes;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.arcanum.UpdateCheckStatusRequest;
import ru.yandex.ci.client.arcanum.util.BodyVerificationMode;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePullRequests;
import ru.yandex.ci.engine.launch.LaunchStartTask;
import ru.yandex.ci.engine.pr.CreatePrCommentTask;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

class DiscoveryServicePullRequestsTest extends EngineTestBase {

    @BeforeEach
    void setUp() {
        mockYav();
        mockValidationSuccessful();
        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds1.json");

        discovery(TestData.TRUNK_COMMIT_R2R1);
        delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);

    }

    @AfterEach
    void tearDown() {
        arcServiceStub.resetAndInitTestData();
    }

    @Test
    void processDiffSet_shouldSendProcessedByCiMergeRequirementStatus() {

        var diffSet = TestData.DIFF_SET_1;
        db.currentOrTx(() -> {
                    db.pullRequestDiffSetTable().save(diffSet);

                    // Save one config state, make sure it will not be overwritten
                    db.configStates().save(ConfigState.builder()
                            .configPath(Path.of("a/b/c/a.yaml"))
                            .status(ConfigState.Status.OK)
                            .build());

                }
        );

        verifyConfigStateRegistered();

        var launchIds = discoveryServicePullRequests.processDiffSet(diffSet, true);

        verifyDiscoveryComplete(diffSet);
        verifyNoBranchAutocheckLaunch(diffSet);
        verifyNoBranchAutocheckErrors(diffSet);
        verifyPrecommitAutocheckLaunchCreated(launchIds);
        verifyNoDefaultArcanumChecksSkipped(diffSet);

        verifyConfigStateRegistered("pr/change/a.yaml", "pr/new/a.yaml");
    }

    @Test
    void processDiffSet_withNoAutocheckSettings() {
        var diffSet = TestData.DIFF_SET_UNKNOWN_R6;
        delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);

        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds6.json");
        db.currentOrTx(() ->
                db.pullRequestDiffSetTable().save(diffSet)
        );

        var launchIds = discoveryServicePullRequests.processDiffSet(diffSet, true);

        verifyDiscoveryComplete(diffSet);
        verifyNoBranchAutocheckLaunch(diffSet);
        verifyNoBranchAutocheckErrors(diffSet);
        verifyNoBazingaTasks(TestData.TRUNK_COMMIT_R2R1);
        verifyNoPrecommitAutocheckLaunchCreated(launchIds);
        verifyDefaultArcanumChecksSkipped(diffSet);
        verifyConfigStateRegistered();
    }

    @Test
    void testBranchAutocheck_whenAutocheckSettingsInvalid() {
        var diffSet = TestData.DIFF_SET_INVALID_R6;
        delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);

        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds6.json");
        db.currentOrTx(() ->
                db.pullRequestDiffSetTable().save(diffSet)
        );

        var launchIds = discoveryServicePullRequests.processDiffSet(diffSet, true);
        executeBazingaTasks(CreatePrCommentTask.class);

        verifyDiscoveryComplete(diffSet);
        verifyAutocheckLaunchStatusInArcanum(diffSet, "r1", ArcanumMergeRequirementDto.Status.FAILURE);
        verifyAutocheckError(diffSet, """
                Autocheck Error
                ==
                Unable to start autocheck for this PR: Skip autocheck from CI, \
                branch config config/branches/releases/ci-invalid.yaml is invalid. Problems: \
                [ConfigProblem(level=CRIT, title={
                  "level" : "error",
                  "schema" : {
                    "loadingURI" : "#",
                    "pointer" : "/properties/ci"
                  },
                  "instance" : {
                    "pointer" : "/ci"
                  },
                  "domain" : "validation",
                  "keyword" : "additionalProperties",
                  "message" : "object instance has properties which are not allowed by the schema: \
                [\\"autocheck-invalid\\"]",
                  "unwanted" : [ "autocheck-invalid" ]
                }, description=)]
                """);
        verifyNoBazingaTasks(TestData.TRUNK_COMMIT_R2R1);
        verifyNoPrecommitAutocheckLaunchCreated(launchIds);
        verifyNoDefaultArcanumChecksSkipped(diffSet);
        verifyFailedValidationRequirementStatus(diffSet,
                "ci/missing-task-definition/a.yaml",
                "invalid/internal-exception/a.yaml",
                "invalid/schema/a.yaml",
                "invalid/unknown-abc/a.yaml",
                "ci/with-sandbox-template-task/a.yaml",
                "invalid/yaml-parse/a.yaml");
        verifyConfigStateRegistered();
    }

    @Test
    void testBranchAutocheck_whenAutocheckUndelegatedSettings() {
        var diffSet = TestData.DIFF_SET_UNDELEGATED_R6;
        delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);

        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds6.json");
        db.currentOrTx(() ->
                db.pullRequestDiffSetTable().save(diffSet)
        );

        var launchIds = discoveryServicePullRequests.processDiffSet(diffSet, true);

        verifyDiscoveryComplete(diffSet);
        verifyAutocheckLaunchStatusInArcanum(diffSet, "r1", ArcanumMergeRequirementDto.Status.PENDING);
        verifyNoBranchAutocheckErrors(diffSet);
        verifyScheduledBazingaTask(TestData.TRUNK_COMMIT_R2R1);
        verifyNoPrecommitAutocheckLaunchCreated(launchIds);
        verifyNoDefaultArcanumChecksSkipped(diffSet);
        verifyConfigStateRegistered();

        // TODO: test autocheckService.createAutocheckLaunch
    }

    @Test
    void testBranchAutocheck_whenAutocheckUndelegatedWithLargeSettings() {
        var diffSet = TestData.DIFF_SET_UNDELEGATED_AUTO_LARGE_R6;
        delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);

        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds6.json");
        db.currentOrTx(() ->
                db.pullRequestDiffSetTable().save(diffSet)
        );

        var launchIds = discoveryServicePullRequests.processDiffSet(diffSet, true);
        executeBazingaTasks(CreatePrCommentTask.class);

        verifyDiscoveryComplete(diffSet);
        verifyAutocheckLaunchStatusInArcanum(diffSet, "r1", ArcanumMergeRequirementDto.Status.FAILURE);
        verifyAutocheckError(diffSet, """
                Autocheck Error
                ==
                Unable to start autocheck for this PR: Skip autocheck from CI, \
                branch config config/branches/releases/ci-undelegated-auto-large.yaml is invalid. \
                Problems: [ConfigProblem(level=CRIT, title=Delegated config is required when \
                large autostart settings are provided, description=)]
                """);
        verifyNoBazingaTasks(TestData.TRUNK_COMMIT_R2R1);
        verifyNoPrecommitAutocheckLaunchCreated(launchIds);
        verifyNoDefaultArcanumChecksSkipped(diffSet);
        verifyFailedValidationRequirementStatus(diffSet,
                "ci/missing-task-definition/a.yaml",
                "invalid/internal-exception/a.yaml",
                "invalid/schema/a.yaml",
                "invalid/unknown-abc/a.yaml",
                "ci/with-sandbox-template-task/a.yaml",
                "invalid/yaml-parse/a.yaml");
        verifyConfigStateRegistered();
    }

    @Test
    void testBranchAutocheck() {
        var commit = TestData.TRUNK_COMMIT_R2R2;
        var diffSet = TestData.DIFF_SET_R6;

        discovery(commit);
        delegateToken(AutocheckConstants.AUTOCHECK_A_YAML_PATH);

        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds6.json");
        db.currentOrTx(() ->
                db.pullRequestDiffSetTable().save(diffSet)
        );

        var launchIds = discoveryServicePullRequests.processDiffSet(diffSet, true);

        verifyDiscoveryComplete(diffSet);
        verifyAutocheckLaunchStatusInArcanum(diffSet, "r2", ArcanumMergeRequirementDto.Status.PENDING);
        verifyNoBranchAutocheckErrors(diffSet);
        verifyScheduledBazingaTask(commit);
        verifyNoPrecommitAutocheckLaunchCreated(launchIds);
        verifyNoDefaultArcanumChecksSkipped(diffSet);
        verifyConfigStateRegistered();

        // TODO: test autocheckService.createAutocheckLaunch
    }

    @Test
    void useAutocheckAYamlFromPrIfChanged() {
        var diffSet = TestData.DIFF_SET_1;
        db.currentOrTx(() ->
                db.pullRequestDiffSetTable().save(diffSet)
        );

        var launchIds = discoveryServicePullRequests.processDiffSet(diffSet, true);

        verifyDiscoveryComplete(diffSet);
        verifyNoBranchAutocheckLaunch(diffSet);
        verifyNoBranchAutocheckErrors(diffSet);
        verifyNoDefaultArcanumChecksSkipped(diffSet);
        var autocheckLaunchId = verifyPrecommitAutocheckLaunchCreated(launchIds);
        var launch = db.currentOrReadOnly(() -> db.launches().get(autocheckLaunchId));
        assertThat(launch.getFlowInfo().getConfigRevision())
                .isEqualTo(diffSet.getOrderedMergeRevision());
        verifyConfigStateRegistered("a/b/c/a.yaml", "pr/change/a.yaml", "pr/new/a.yaml");
    }

    private void verifyDiscoveryComplete(PullRequestDiffSet diffSet) {
        arcanumTestServer.verifySetMergeRequirementStatus(
                diffSet.getPullRequestId(),
                diffSet.getDiffSetId(),
                UpdateCheckStatusRequest.builder()
                        .requirementId(ArcanumMergeRequirementId.of("arcanum", "processed_by_ci"))
                        .status(ArcanumMergeRequirementDto.Status.SUCCESS)
                        .build()
        );
    }

    private void verifyNoBranchAutocheckLaunch(PullRequestDiffSet diffSet) {
        for (String rev : List.of("r1", "r2")) {
            arcanumTestServer.verifySetMergeRequirementStatus(
                    diffSet.getPullRequestId(),
                    diffSet.getDiffSetId(),
                    UpdateCheckStatusRequest.builder()
                            .requirementId(ArcanumMergeRequirementId.of("CI", "autocheck: Сборка в бранчах " + rev))
                            .status(ArcanumMergeRequirementDto.Status.PENDING)
                            .build(),
                    VerificationTimes.exactly(0)
            );
        }
    }

    private void verifyAutocheckLaunchStatusInArcanum(PullRequestDiffSet diffSet,
                                                      String rev,
                                                      ArcanumMergeRequirementDto.Status status) {
        arcanumTestServer.verifySetMergeRequirementStatus(
                diffSet.getPullRequestId(),
                diffSet.getDiffSetId(),
                UpdateCheckStatusRequest.builder()
                        .requirementId(ArcanumMergeRequirementId.of("CI", "autocheck: Сборка в бранчах " + rev))
                        .status(status)
                        .build()
        );
    }

    private void verifyNoBranchAutocheckErrors(PullRequestDiffSet diffSet) {
        arcanumTestServer.verifyCreateReviewRequestComment(
                diffSet.getPullRequestId(),
                ".*Unable to start autocheck for this PR.*",
                BodyVerificationMode.REGEXP,
                VerificationTimes.exactly(0)
        );
    }

    private void verifyAutocheckError(PullRequestDiffSet diffSet, String expectError) {
        arcanumTestServer.verifyCreateReviewRequestComment(
                diffSet.getPullRequestId(),
                expectError
        );
    }

    private void verifyFailedValidationRequirementStatus(PullRequestDiffSet diffSet, String... paths) {
        for (var path : paths) {
            arcanumTestServer.verifySetMergeRequirementStatus(
                    diffSet.getPullRequestId(),
                    diffSet.getDiffSetId(),
                    UpdateCheckStatusRequest.builder()
                            .requirementId(ArcanumMergeRequirementId.of("CI", "[validation] " + path))
                            .status(ArcanumMergeRequirementDto.Status.FAILURE)
                            .description("See Comments")
                            .build()
            );
        }
    }

    private void verifyNoBazingaTasks(ArcCommit commit) {
        verifyBazingaTasks(commit, 0);
    }

    private void verifyScheduledBazingaTask(ArcCommit commit) {
        verifyBazingaTasks(commit, 1);
    }

    private void verifyBazingaTasks(ArcCommit commit, int taskCount) {
        // start delay launches and then check bazinga manager
        launchService.startDelayedLaunches(AutocheckConstants.AUTOCHECK_A_YAML_PATH,
                commit.getRevision());
        verify(bazingaTaskManagerStub, times(taskCount)).schedule(any(LaunchStartTask.class));
    }

    @CanIgnoreReturnValue
    private LaunchId verifyPrecommitAutocheckLaunchCreated(List<LaunchId> launchIds) {
        assertThat(launchIds)
                .extracting(LaunchId::getProcessId)
                .contains(AutocheckBootstrapServicePullRequests.TRUNK_PRECOMMIT_PROCESS_ID);

        return launchIds.stream()
                .filter(id -> id.getProcessId().equals(
                        AutocheckBootstrapServicePullRequests.TRUNK_PRECOMMIT_PROCESS_ID))
                .findFirst()
                .orElseThrow();
    }

    private void verifyNoPrecommitAutocheckLaunchCreated(List<LaunchId> launchIds) {
        assertThat(launchIds)
                .extracting(LaunchId::getProcessId)
                .doesNotContain(AutocheckBootstrapServicePullRequests.TRUNK_PRECOMMIT_PROCESS_ID);
    }

    private void verifyDefaultArcanumChecksSkipped(PullRequestDiffSet diffSet) {
        verify(pullRequestService).skipArcanumDefaultChecks(
                eq(diffSet.getPullRequestId()), eq(diffSet.getDiffSetId())
        );
    }

    private void verifyNoDefaultArcanumChecksSkipped(PullRequestDiffSet diffSet) {
        verify(pullRequestService, times(0)).skipArcanumDefaultChecks(
                eq(diffSet.getPullRequestId()), eq(diffSet.getDiffSetId())
        );
    }

    private void verifyConfigStateRegistered(String... configs) {
        var list = db.currentOrReadOnly(() -> db.configStates().findDraftPaths());
        assertThat(list)
                .isEqualTo(List.of(configs));
    }


}
