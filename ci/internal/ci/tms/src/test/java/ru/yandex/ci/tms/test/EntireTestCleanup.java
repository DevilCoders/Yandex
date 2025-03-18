package ru.yandex.ci.tms.test;

import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.arcanum.UpdateCheckStatusRequest;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.a.model.CleanupReason;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.engine.launch.LaunchCleanupTask;
import ru.yandex.ci.engine.launch.cleanup.PullRequestDiffSetCompleteTask;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

public class EntireTestCleanup extends AbstractEntireTest {

    @Test
    void onCommitWithCleanupNormal() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, TestData.TRUNK_R2);

        var job1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job1.getTitle()).isEqualTo("Задача 1, переменная из action");
        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT,
                Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        disableManualSwitch(waitLaunch, "job-2");

        var job2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-2", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job2.getTitle()).isEqualTo("Задача 2, переменная из action");
        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_CLEANUP);

        intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();


        // Запустим cleanup
        flowStateService.cleanupLaunch(FlowLaunchId.of(launch.getLaunchId()));


        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT);
        assertThat(launch.getStatus()).isEqualTo(Status.SUCCESS);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        // Проверим, что выполнились cleanup задачи
        var cJob1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob1.getTitle()).isEqualTo("Очистка 1, переменная из action");
        var cJob2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-2", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob2.getTitle()).isEqualTo("Очистка 2, переменная из action");
    }

    @Test
    void onCommitWithCleanupNoCleanupJobs() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_WITHOUT_JOBS_ID;
        engineTester.delegateToken(processId.getPath());

        long pullRequestId = 12;
        long diffSetId = 14;

        var vcsInfo = vcsInfo();
        LaunchPullRequestInfo pullRequestInfo = pullRequestInfo(pullRequestId, diffSetId, vcsInfo);
        Launch launch = launch(processId, TestData.TRUNK_R2, pullRequestInfo);

        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        db.currentOrTx(() ->
                db.pullRequestDiffSetTable().save(
                        new PullRequestDiffSet(
                                PullRequestDiffSet.Id.of(pullRequestId, diffSetId),
                                "username",
                                "Some summary2", // different info, for testing purposes
                                "Some description2", // different info, for testing purposes
                                vcsInfo,
                                new PullRequestDiffSet.State(0),
                                List.of("CI-2"), // different info, for testing purposes
                                PullRequestDiffSet.Status.DISCOVERED,
                                true,
                                List.of("label2"),
                                null,
                                null
                        )
                )
        );

        // Запустится в отдельной задаче
        bazingaTaskManager.schedule(new PullRequestDiffSetCompleteTask(pullRequestId, diffSetId,
                CleanupReason.NEW_DIFF_SET));

        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.CANCELED);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.isDisabled()).isTrue();
    }

    @Test
    void onCommitWithCleanupNormalDependent() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_DEPENDENT_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, TestData.TRUNK_R2);

        engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL);
        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT,
                Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        disableManualSwitch(waitLaunch, "job-2");

        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_CLEANUP);

        intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();


        // Запустим cleanup
        flowStateService.cleanupLaunch(FlowLaunchId.of(launch.getLaunchId()));

        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT);
        assertThat(launch.getStatus()).isEqualTo(Status.SUCCESS);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        // Проверим, что выполнились cleanup задачи
        engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1", StatusChangeType.SUCCESSFUL);
        engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-2", StatusChangeType.SUCCESSFUL);
        engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1-1", StatusChangeType.SUCCESSFUL);
    }

    @Test
    void onCommitWithCleanupNormalWithDelay() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_AUTO_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, TestData.TRUNK_R2);

        var job1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job1.getTitle()).isEqualTo("Задача 1, переменная из action, delay");
        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT,
                Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        disableManualSwitch(waitLaunch, "job-2");

        var job2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-2", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job2.getTitle()).isEqualTo("Задача 2, переменная из action, delay");

        // Cleanup отработает в отдельной задаче
        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        // Проверим, что выполнились cleanup задачи
        var cJob1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob1.getTitle()).isEqualTo("Очистка 1, переменная из action, delay");
        var cJob2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-2", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob2.getTitle()).isEqualTo("Очистка 2, переменная из action, delay");
    }

    @Test
    void onCommitWithCleanupAfterFailNoAction() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (task, resourceCollector) -> {
            throw new RuntimeException("JOB FAILED");
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_FAIL_SUCCESS_ONLY_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, TestData.TRUNK_R2);

        var job1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job1.getTitle()).isEqualTo("Задача 1, переменная из action, failure");
        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT,
                Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        disableManualSwitch(waitLaunch, "job-2");

        var job2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-2", StatusChangeType.FAILED).getJobState();
        assertThat(job2.getTitle()).isEqualTo("Задача 2, переменная из action, failure");

        // Cleanup-а не будет - нет настроек для запуска (разрешаем только SUCCESS)
        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.FAILURE);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        assertThat(flowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(flowLaunch.getJobs().get("cleanup-job-2")).isNull();
    }


    @Test
    void onCommitWithCleanupAfterFailAuto() throws YavDelegationException, InterruptedException {
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (task, resourceCollector) -> {
            throw new RuntimeException("JOB FAILED");
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_FAIL_AUTO_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, TestData.TRUNK_R2);

        var job1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job1.getTitle()).isEqualTo("Задача 1, переменная из action, failure auto");
        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT,
                Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        disableManualSwitch(waitLaunch, "job-2");

        var job2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-2", StatusChangeType.FAILED).getJobState();
        assertThat(job2.getTitle()).isEqualTo("Задача 2, переменная из action, failure auto");

        // Cleanup отработает в отдельной задаче, сработав на FAILURE
        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.FAILURE);

        // Проверим, что выполнились cleanup задачи
        var cJob1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob1.getTitle()).isEqualTo("Очистка 1, переменная из action, failure auto");
        var cJob2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-2", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob2.getTitle()).isEqualTo("Очистка 2, переменная из action, failure auto");
    }


    @Test
    void onCommitWithCleanupPullRequestClose() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_ID;
        engineTester.delegateToken(processId.getPath());

        long pullRequestId = 12;
        long diffSetId = 14;
        // vcsInfo здесь не особо важен и ни на что не влияет
        var vcsInfo = vcsInfo();
        var pullRequestInfo = pullRequestInfo(pullRequestId, diffSetId, vcsInfo);
        Launch launch = launch(processId, TestData.TRUNK_R2, pullRequestInfo);

        var job1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job1.getTitle()).isEqualTo("Задача 1, переменная из action");
        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT,
                Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        var prInfo = intFlowLaunch.getVcsInfo().getPullRequestInfo();
        assertThat(prInfo).isNotNull();
        assertThat(prInfo.getSummary())
                .isEqualTo("Some summary");
        assertThat(prInfo.getDescription())
                .isEqualTo("Some description");
        assertThat(prInfo.getPullRequestIssues())
                .isEqualTo(List.of("CI-1"));
        assertThat(prInfo.getPullRequestLabels())
                .isEqualTo(List.of("label1"));

        disableManualSwitch(waitLaunch, "job-2");

        var job2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-2", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job2.getTitle()).isEqualTo("Задача 2, переменная из action");
        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_CLEANUP);

        intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();


        db.currentOrTx(() ->
                db.pullRequestDiffSetTable().save(
                        new PullRequestDiffSet(
                                PullRequestDiffSet.Id.of(pullRequestId, diffSetId),
                                "username",
                                "Some summary2", // different info, for testing purposes
                                "Some description2", // different info, for testing purposes
                                vcsInfo,
                                new PullRequestDiffSet.State(0),
                                List.of("CI-2"), // different info, for testing purposes
                                PullRequestDiffSet.Status.DISCOVERED,
                                true,
                                List.of("label2"),
                                null,
                                null
                        )
                )
        );

        // Запустится в отдельной задаче
        bazingaTaskManager.schedule(new PullRequestDiffSetCompleteTask(pullRequestId, diffSetId,
                CleanupReason.PR_MERGED));

        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);

        arcanumTestServer.verifySetMergeRequirementStatus(
                pullRequestId,
                diffSetId,
                UpdateCheckStatusRequest.builder()
                        .requirementId(ArcanumMergeRequirementId.of("x", "y"))
                        .status(ArcanumMergeRequirementDto.Status.SUCCESS)
                        .systemCheckUri(
                                "https://arcanum-test-url/projects/ci/ci/actions/flow?dir=release%2Fsawmill&id" +
                                        "=simplest-action-with-cleanup&number=2")
                        .build()
        );

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        // Проверим, что выполнились cleanup задачи
        var cJob1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob1.getTitle()).isEqualTo("Очистка 1, переменная из action");

        var cJob2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-2", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob2.getTitle()).isEqualTo("Очистка 2, переменная из action");

        var lastPrInfo = flowLaunch.getVcsInfo().getPullRequestInfo();
        assertThat(lastPrInfo).isNotNull();
        assertThat(lastPrInfo.getSummary())
                .isEqualTo("Some summary");
        assertThat(lastPrInfo.getDescription())
                .isEqualTo("Some description");
        assertThat(lastPrInfo.getPullRequestIssues())
                .isEqualTo(List.of("CI-1"));
    }


    @SuppressWarnings("unchecked")
    @Test
    void onCommitWithCleanupAndFailPullRequestClose() throws YavDelegationException, InterruptedException {
        var contextMapRef = new AtomicReference<Map<String, Object>>();
        sandboxClientStub.uploadSandboxTask("SAWMILL_SANDBOX_SOURCE", (task, resourceCollector) -> {
            contextMapRef.set(task.getContext());
            throw new RuntimeException("JOB FAILED");
        });

        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_FAIL_ID;
        engineTester.delegateToken(processId.getPath());

        long pullRequestId = 12;
        long diffSetId = 14;
        // vcsInfo здесь не особо важен и ни на что не влияет
        var vcsInfo = vcsInfo();
        var pullRequestInfo = new LaunchPullRequestInfo(
                pullRequestId,
                diffSetId,
                TestData.CI_USER,
                "Some summary",
                "Some description",
                ArcanumMergeRequirementId.of("x", "y"),
                vcsInfo,
                List.of("CI-1"),
                List.of("label1"),
                null);
        Launch launch = launch(processId, TestData.TRUNK_R2, pullRequestInfo);

        var job1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job1.getTitle()).isEqualTo("Задача 1, переменная из action, failure");
        var waitLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT,
                Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        var prInfo = intFlowLaunch.getVcsInfo().getPullRequestInfo();
        assertThat(prInfo).isNotNull();
        assertThat(prInfo.getSummary())
                .isEqualTo("Some summary");
        assertThat(prInfo.getDescription())
                .isEqualTo("Some description");
        assertThat(prInfo.getPullRequestIssues())
                .isEqualTo(List.of("CI-1"));

        disableManualSwitch(waitLaunch, "job-2");

        var job2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-2", StatusChangeType.FAILED).getJobState();
        assertThat(job2.getTitle()).isEqualTo("Задача 2, переменная из action, failure");
        assertThat(job2.getManualTriggerPrompt().getQuestion())
                .isEqualTo("Подтверждение переменная из action, failure...");
        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.FAILURE);

        arcanumTestServer.verifySetMergeRequirementStatus(
                pullRequestId,
                diffSetId,
                UpdateCheckStatusRequest.builder()
                        .requirementId(ArcanumMergeRequirementId.of("x", "y"))
                        .status(ArcanumMergeRequirementDto.Status.FAILURE)
                        .systemCheckUri(
                                "https://arcanum-test-url/projects/ci/ci/actions/flow?dir=release%2Fsawmill&id" +
                                        "=simplest-action-with-cleanup-fail&number=2")
                        .build()
        );

        intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        db.currentOrTx(() ->
                db.pullRequestDiffSetTable().save(
                        new PullRequestDiffSet(
                                PullRequestDiffSet.Id.of(pullRequestId, diffSetId),
                                "username",
                                "Some summary2", // different info, for testing purposes
                                "Some description2", // different info, for testing purposes
                                vcsInfo,
                                new PullRequestDiffSet.State(0),
                                List.of(), // different info, for testing purposes
                                PullRequestDiffSet.Status.DISCOVERED,
                                true,
                                List.of(),
                                null,
                                null
                        )
                )
        );
        // Запустится в отдельной задаче
        bazingaTaskManager.schedule(new PullRequestDiffSetCompleteTask(pullRequestId, diffSetId,
                CleanupReason.PR_DISCARDED));

        launch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.FAILURE);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(flowLaunch.getStateVersion()).isGreaterThan(1);

        // Проверим, что выполнились cleanup задачи
        var cJob1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob1.getTitle()).isEqualTo("Очистка 1, переменная из action, failure");

        var cJob2 = engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-2", StatusChangeType.SUCCESSFUL)
                .getJobState();
        assertThat(cJob2.getTitle()).isEqualTo("Очистка 2, переменная из action, failure");

        var lastPrInfo = flowLaunch.getVcsInfo().getPullRequestInfo();
        assertThat(lastPrInfo).isNotNull();
        assertThat(lastPrInfo.getSummary())
                .isEqualTo("Some summary");
        assertThat(lastPrInfo.getDescription())
                .isEqualTo("Some description");
        assertThat(lastPrInfo.getPullRequestIssues())
                .isEqualTo(List.of("CI-1"));

        var map = contextMapRef.get();
        assertThat(map).isNotEmpty();
        var pullRequestMap = ((Map<String, Object>) ((Map<String, Object>) map.get("__CI_CONTEXT"))
                .get("launch_pull_request_info"))
                .get("pull_request");
        assertThat(pullRequestMap).isNotNull();
        assertThat(pullRequestMap)
                .isEqualTo(Map.of(
                        "id", pullRequestId,
                        "author", TestData.CI_USER,
                        "summary", "Some summary",
                        "description", "Some description"));
    }


    @Test
    void onCommitWithCleanupCanceling() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, TestData.TRUNK_R2);

        var job1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job1.getTitle()).isEqualTo("Задача 1, переменная из action");
        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        bazingaTaskManager.schedule(new LaunchCleanupTask(launch.getId(), CleanupReason.NEW_DIFF_SET));

        // Проверим, что выполнились cleanup задачи
        engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1", StatusChangeType.SUCCESSFUL);
        engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-2", StatusChangeType.SUCCESSFUL);
    }

    @Test
    void onCommitWithCleanupCancelingIgnored() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var processId = TestData.SIMPLEST_ACTION_WITH_CLEANUP_ID;

        engineTester.delegateToken(processId.getPath());

        Launch launch = launch(processId, TestData.TRUNK_R2);

        var job1 = engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL).getJobState();
        assertThat(job1.getTitle()).isEqualTo("Задача 1, переменная из action");
        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        bazingaTaskManager.schedule(new LaunchCleanupTask(launch.getId(), CleanupReason.FINISH));

        // По умолчанию cleanup не сработает, т.к. в режиме NORMAL не настроен interrupt
        assertThatThrownBy(() -> engineTester.waitJob(launch.getLaunchId(), Duration.ofSeconds(10),
                "cleanup-job-1", StatusChangeType.SUCCESSFUL))
                .hasMessage("Job cleanup-job-1 of launch %s don't became in state %s in 10sec"
                        .formatted(launch.getLaunchId(), StatusChangeType.SUCCESSFUL));
    }

    @Test
    void onCommitWithCleanupCancelingWithDependent() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2);

        var action = TestData.SIMPLEST_ACTION_WITH_CLEANUP_DEPENDENT_ID;
        engineTester.delegateToken(action.getPath());

        Launch launch = launch(action, TestData.TRUNK_R2);

        engineTester.waitJob(launch.getLaunchId(), WAIT, "job-1", StatusChangeType.SUCCESSFUL);
        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // cleanup задач еще нет
        FlowLaunchEntity intFlowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launch.getLaunchId()));
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-1")).isNull();
        assertThat(intFlowLaunch.getJobs().get("cleanup-job-2")).isNull();

        bazingaTaskManager.schedule(new LaunchCleanupTask(launch.getId(), CleanupReason.NEW_DIFF_SET));

        // Проверим, что выполнились cleanup задачи
        engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1", StatusChangeType.SUCCESSFUL);
        engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-2", StatusChangeType.SUCCESSFUL);
        engineTester.waitJob(launch.getLaunchId(), WAIT, "cleanup-job-1-1", StatusChangeType.SUCCESSFUL);
    }

    private static PullRequestVcsInfo vcsInfo() {
        return new PullRequestVcsInfo(TestData.DS2_REVISION,
                TestData.TRUNK_R2.toRevision(),
                ArcBranch.trunk(),
                TestData.REVISION,
                TestData.USER_BRANCH);
    }

    private static LaunchPullRequestInfo pullRequestInfo(long pullRequestId,
                                                         long diffSetId,
                                                         PullRequestVcsInfo vcsInfo) {
        return new LaunchPullRequestInfo(
                pullRequestId,
                diffSetId,
                TestData.CI_USER,
                "Some summary",
                "Some description",
                ArcanumMergeRequirementId.of("x", "y"),
                vcsInfo,
                List.of("CI-1"),
                List.of("label1"),
                null);
    }
}
