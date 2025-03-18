package ru.yandex.ci.engine.launch.cleanup;

import java.time.Duration;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.joda.time.Instant;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.config.a.model.CleanupReason;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.engine.launch.LaunchCleanupTask;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;
import ru.yandex.misc.bender.annotation.BenderDefaultValue;

@Slf4j
public class PullRequestDiffSetCompleteTask extends AbstractOnetimeTask<PullRequestDiffSetCompleteTask.Params> {

    private BazingaTaskManager bazingaTaskManager;
    private CiMainDb db;
    private Duration rescheduleDelay;

    public PullRequestDiffSetCompleteTask(
            @Nonnull BazingaTaskManager bazingaTaskManager,
            @Nonnull CiMainDb db,
            @Nonnull Duration rescheduleDelay) {
        super(PullRequestDiffSetCompleteTask.Params.class);
        this.bazingaTaskManager = bazingaTaskManager;
        this.db = db;
        this.rescheduleDelay = rescheduleDelay;
    }

    public PullRequestDiffSetCompleteTask(long pullRequestId, long diffSetId, @Nullable CleanupReason cleanupReason) {
        super(new PullRequestDiffSetCompleteTask.Params(pullRequestId, diffSetId, cleanupReason));
    }

    @Override
    protected void execute(PullRequestDiffSetCompleteTask.Params params, ExecutionContext context) {
        log.info("Processing complete diff-set: {}", params);

        db.currentOrTx(() -> {
            var diffSetOpt = db.pullRequestDiffSetTable().findById(params.getPullRequestId(), params.getDiffSetId());
            if (diffSetOpt.isEmpty()) {
                // Just a temporary solution to close obsolete tasks
                log.info("Diff-set {} does not exists, no need for completion", params);
                return; // ---
            }

            var diffSet = diffSetOpt.get();
            if (!diffSet.getScheduleCompletion()) {
                log.info("Diff-set {} is not scheduled for completion", params);
                return; // ---
            }

            if (diffSet.getStatus() == PullRequestDiffSet.Status.NEW) {
                // In rare occasions...
                log.info("Diff-set {} is not discovered yet, reschedule", params);
                bazingaTaskManager.schedule(
                        new PullRequestDiffSetCompleteTask(params.getPullRequestId(), params.getDiffSetId(),
                                params.getCleanupReason()),
                        Instant.now().plus(org.joda.time.Duration.millis(rescheduleDelay.toMillis())));
                return; // ---
            } else if (diffSet.getStatus() != PullRequestDiffSet.Status.DISCOVERED) {
                log.warn("Diff-set {} has status {}, skip execution", params, diffSet.getStatus());
                return; // ---
            }

            // Ignore all virtual launches until CI-2833
            var launches = db.launches().getPullRequestLaunches(params.getPullRequestId(), params.getDiffSetId())
                    .stream()
                    .filter(id -> VirtualType.of(id.toProcessId()) == null)
                    .toList();
            if (launches.isEmpty()) {
                log.info("No launches found, skipping");
            } else {
                for (var id : launches) {
                    log.info("Scheduling cleanup for {}", id);
                    bazingaTaskManager.schedule(new LaunchCleanupTask(id, params.getCleanupReason()));
                }
            }

            db.pullRequestDiffSetTable().save(diffSet.withStatus(PullRequestDiffSet.Status.COMPLETE));
            log.info("Complete diff-set processed: {}", params);
        });
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(10);
    }

    @Value
    @BenderBindAllFields
    public static class Params {
        long pullRequestId;
        long diffSetId;
        @BenderDefaultValue("FINISH")
        @Nullable
        CleanupReason cleanupReason;
    }
}
