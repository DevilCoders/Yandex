package ru.yandex.ci.engine.discovery.task;

import java.time.Duration;
import java.util.Objects;

import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Slf4j
public class PullRequestDiffSetProcessedByCiTask
        extends AbstractOnetimeTask<PullRequestDiffSetProcessedByCiTask.Params> {

    private PullRequestService pullRequestService;

    public PullRequestDiffSetProcessedByCiTask(PullRequestService pullRequestService) {
        super(Params.class);
        this.pullRequestService = pullRequestService;
    }

    public PullRequestDiffSetProcessedByCiTask(PullRequestDiffSet.Id id, boolean skipDefaultCiChecks) {
        super(new Params(
                id.getPullRequestId(), id.getDiffSetId(), skipDefaultCiChecks
        ));
    }

    @Override
    protected void execute(Params params, ExecutionContext context) throws Exception {
        log.info("Processing task: {}", params);

        var pullRequestId = params.getPullRequestId();
        var diffSetId = params.getDiffSetId();
        log.info("Sending processed_by_ci check status");
        pullRequestService.sendProcessedByCiMergeRequirementStatus(pullRequestId, diffSetId);
        if (params.isSkipDefaultCiChecks()) {
            log.info("Skipping default ci checks");
            pullRequestService.skipArcanumDefaultChecks(pullRequestId, diffSetId);
        }
        log.info("Processed task: {}", params);
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(1);
    }

    @Value
    @BenderBindAllFields
    public static class Params {
        long pullRequestId;
        long diffSetId;
        Boolean skipDefaultCiChecks;

        public boolean isSkipDefaultCiChecks() {
            return Objects.requireNonNullElse(skipDefaultCiChecks, false);
        }
    }
}
