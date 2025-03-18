package ru.yandex.ci.observer.api.statistics.model;

import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import javax.annotation.Nonnull;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.storage.StorageUtils;
import ru.yandex.ci.observer.api.ObserverApiYdbTestBase;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;

import static org.assertj.core.api.Assertions.assertThat;

class CheckTaskMaxDelayTest extends ObserverApiYdbTestBase {

    private static final String LINUX_JOB_NAME = "BUILD_TRUNK_META_LINUX_DISTBUILD";
    private static final String MANDATORY_JOB_NAME = "AUTOCHECK_MANDATORY_PLATFORMS";

    private final AtomicInteger iterationIdSequence = new AtomicInteger(0);
    private final AtomicInteger taskIdSequence = new AtomicInteger(0);

    private static final CheckTaskEntity POSTCOMMIT_TASK = SAMPLE_TASK.toBuilder()
            .checkType(CheckType.TRUNK_POST_COMMIT)
            .jobName(LINUX_JOB_NAME)
            .build();

    private static final Instant FROM = TIME;
    private static final Instant TO = FROM.plus(200, ChronoUnit.MINUTES);
    private static final Duration DELTA = Duration.between(FROM, TO);

    @Test
    void computeForPostCommits_whenNoTasksFound() {
        assertThat(computeForPostCommits(FROM, TO, db)).isEmpty();
    }

    @Test
    void computeForPostCommits_whenThereAreLeftAndRightTasks() {
        generateLinuxLeftTask(FROM.plus(50, ChronoUnit.MINUTES));
        assertThat(computeForPostCommits(FROM, TO, db)).containsExactly(
                new CheckTaskMaxDelay(CheckType.TRUNK_POST_COMMIT, LINUX_JOB_NAME, DELTA.minusMinutes(50))
        );

        generateLinuxLeftTask(FROM.plus(30, ChronoUnit.MINUTES));
        assertThat(computeForPostCommits(FROM, TO, db)).containsExactly(
                new CheckTaskMaxDelay(CheckType.TRUNK_POST_COMMIT, LINUX_JOB_NAME, DELTA.minusMinutes(30))
        );

        generateLinuxRightTask(FROM.plus(40, ChronoUnit.MINUTES));
        assertThat(computeForPostCommits(FROM, TO, db)).containsExactly(
                new CheckTaskMaxDelay(CheckType.TRUNK_POST_COMMIT, LINUX_JOB_NAME, DELTA.minusMinutes(30))
        );

        generateLinuxRightTask(FROM.plus(20, ChronoUnit.MINUTES));
        assertThat(computeForPostCommits(FROM, TO, db)).containsExactly(
                new CheckTaskMaxDelay(CheckType.TRUNK_POST_COMMIT, LINUX_JOB_NAME, DELTA.minusMinutes(20))
        );
    }

    @Test
    void computeForPostCommits_whenThereAreDifferentPlatforms() {
        generateLinuxLeftTask(FROM.plus(50, ChronoUnit.MINUTES));
        generateMandatoryLeftTask(FROM.plus(10, ChronoUnit.MINUTES));
        generateMandatoryLeftTask(FROM.plus(60, ChronoUnit.MINUTES));
        assertThat(computeForPostCommits(FROM, TO, db)).containsExactlyInAnyOrder(
                new CheckTaskMaxDelay(CheckType.TRUNK_POST_COMMIT, LINUX_JOB_NAME, DELTA.minusMinutes(50)),
                new CheckTaskMaxDelay(CheckType.TRUNK_POST_COMMIT, MANDATORY_JOB_NAME, DELTA.minusMinutes(10))
        );
    }

    @Test
    void computeForPostCommits_whenThereIsRecheckJob() {
        generateLinuxLeftTask(FROM.plus(40, ChronoUnit.MINUTES));
        generateLinuxLeftTaskRecheck(FROM.plus(50, ChronoUnit.MINUTES));

        assertThat(computeForPostCommits(FROM, TO, db)).containsExactlyInAnyOrder(
                new CheckTaskMaxDelay(CheckType.TRUNK_POST_COMMIT, LINUX_JOB_NAME, DELTA.minusMinutes(40))
        );

        generateLinuxLeftTaskRecheck(FROM.plus(30, ChronoUnit.MINUTES));
        assertThat(computeForPostCommits(FROM, TO, db)).containsExactlyInAnyOrder(
                new CheckTaskMaxDelay(CheckType.TRUNK_POST_COMMIT, LINUX_JOB_NAME, DELTA.minusMinutes(30))
        );
    }

    public static List<CheckTaskMaxDelay> computeForPostCommits(@Nonnull Instant from,
                                                                @Nonnull Instant to,
                                                                @Nonnull CiObserverDb db) {
        return CheckTaskMaxDelay.computeForPostCommits(from, to, db);
    }

    private CheckTaskEntity generateLinuxRightTask(@Nonnull Instant created) {
        return generateTask(created, LINUX_JOB_NAME, true);
    }

    private CheckTaskEntity generateLinuxLeftTask(@Nonnull Instant created) {
        return generateTask(created, LINUX_JOB_NAME, false);
    }

    private CheckTaskEntity generateLinuxLeftTaskRecheck(@Nonnull Instant created) {
        return generateTask(created, StorageUtils.toRestartJobName(LINUX_JOB_NAME), false);
    }

    private CheckTaskEntity generateMandatoryLeftTask(@Nonnull Instant created) {
        return generateTask(created, MANDATORY_JOB_NAME, false);
    }

    private CheckTaskEntity generateTask(@Nonnull Instant created, @Nonnull String jobName, boolean right) {
        var iterationId = generateIterationId();
        db.currentOrTx(() -> {
            // save new iteration with create time, cause max delay uses iteration.right for computation
            var iteration = SAMPLE_ITERATION.toBuilder()
                    .id(iterationId)
                    .right(storageRevision(created))
                    .build();
            db.iterations().save(iteration);
        });

        var task = POSTCOMMIT_TASK.toBuilder()
                .id(generateTaskId(iterationId))
                .created(created)
                .rightRevisionTimestamp(created)
                .jobName(jobName)
                .right(right)
                .build();
        return db.currentOrTx(() -> db.tasks().save(task));
    }

    private CheckIterationEntity.Id generateIterationId() {
        return new CheckIterationEntity.Id(
                SAMPLE_ITERATION_ID.getCheckId(),
                SAMPLE_ITERATION_ID.getIterType(),
                iterationIdSequence.incrementAndGet()
        );
    }

    private CheckTaskEntity.Id generateTaskId(CheckIterationEntity.Id iterationId) {
        var taskId = "testTaskId-" + taskIdSequence.incrementAndGet();
        return new CheckTaskEntity.Id(iterationId, taskId);
    }

    private StorageRevision storageRevision(Instant createTime) {
        return StorageRevision.from(
                "pr:1234",
                ArcCommit.builder().createTime(createTime).id(ArcCommit.Id.of("commitId")).svnRevision(123).build()
        );
    }

}
