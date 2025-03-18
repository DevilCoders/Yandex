package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.UUID;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.autocheck.Autocheck;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.engine.autocheck.testenv.TestenvStartService;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;

@Slf4j
@RequiredArgsConstructor
@ExecutorInfo(
        title = "Start testenv",
        description = "Internal job for starting TestEnv jobs"
)
@Consume(name = "launch", proto = Autocheck.AutocheckLaunch.class)
public class StartTestenvJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("0b681388-b8b6-4967-84c0-c2b8baa060ea");

    private final TestenvStartService testenvStartService;
    private final StorageApiClient storageApiClient;
    private final StartTestenvJobSemaphore jobSemaphore;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        try (var semaphore = jobSemaphore.acquire()) {
            doExecute(context);
        }
    }

    private void doExecute(JobContext context) throws Exception {
        var launch = context.resources().consume(Autocheck.AutocheckLaunch.class);
        var vcsInfo = context.getFlowLaunch().getVcsInfo();

        var reviewRequestId = getPullRequestId(vcsInfo);
        var diffSetId = getDiffSetId(vcsInfo);
        if (reviewRequestId == null || diffSetId == null) {
            return;
        }

        var reviewRequestData = testenvStartService.getReviewRequestData(reviewRequestId);
        var diffSet = testenvStartService.getDiffSet(reviewRequestId, diffSetId, reviewRequestData);

        if (diffSet.getGsid() == null || diffSet.getZipatch() == null || diffSet.getPatchUrl() == null) {
            throw new RuntimeException(
                    String.format(
                            "Unable to start pre-commit check for reviewRequest %d, diffSet %d," +
                                    " gsid %s, zipatch %s, patchUrl %s",
                            reviewRequestId, diffSetId,
                            diffSet.getGsid(), diffSet.getZipatch(), diffSet.getPatchUrl()
                    )
            );
        }

        var testenvResponse = testenvStartService.startTestenvCheck(
                reviewRequestId, diffSetId, reviewRequestData, diffSet, launch.getCheckInfo().getCheckId()
        );

        log.info("TestEnv response: {}", testenvResponse);

        var testenvId = testenvResponse.getCheckId();
        if (testenvId != null) {
            storageApiClient.setTestenvId(launch.getCheckInfo().getCheckId(), testenvId);
        }
    }

    @Nullable
    private Long getPullRequestId(LaunchVcsInfo vcsInfo) {
        var pullRequestInfo = vcsInfo.getPullRequestInfo();
        return pullRequestInfo != null ? pullRequestInfo.getPullRequestId() : null;
    }

    @Nullable
    private Long getDiffSetId(LaunchVcsInfo vcsInfo) {
        var pullRequestInfo = vcsInfo.getPullRequestInfo();
        return pullRequestInfo != null ? pullRequestInfo.getDiffSetId() : null;
    }
}
