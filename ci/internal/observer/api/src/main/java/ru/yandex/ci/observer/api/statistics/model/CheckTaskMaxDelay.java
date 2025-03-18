package ru.yandex.ci.observer.api.statistics.model;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import yandex.cloud.repository.db.IsolationLevel;
import ru.yandex.ci.core.storage.StorageUtils;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTasksTable;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;

import static java.util.Comparator.comparing;
import static java.util.stream.Collectors.minBy;

public class CheckTaskMaxDelay extends StatisticsItem {

    private static final String SENSOR = "check_task_max_delay_minutes";

    public CheckTaskMaxDelay(@Nonnull CheckType checkType, @Nonnull String jobName, @Nonnull Duration value) {
        super(
                SENSOR,
                Map.of(
                        SensorLabels.CHECK_TYPE, checkType.toString(),
                        SensorLabels.JOB_NAME, jobName
                ),
                (double) value.toMinutes()
        );
    }

    public static List<CheckTaskMaxDelay> computeForPostCommits(
            @Nonnull Instant from,
            @Nonnull Instant to,
            @Nonnull CiObserverDb db
    ) {
        var checkType = CheckType.TRUNK_POST_COMMIT;
        return getMinTimestampForRunningTasksGroupedByJobName(from, to, checkType, db)
                .stream()
                .collect(Collectors.groupingBy(
                        it -> StorageUtils.removeRestartJobPrefix(it.getJobName()),
                        minBy(comparing(CheckTasksTable.MinRightRevisionTimestamp::getRightRevisionTimestamp))
                )).values().stream()
                .filter(Optional::isPresent)
                .map(Optional::get)
                .map(it -> new CheckTaskMaxDelay(
                        checkType,
                        StorageUtils.removeRestartJobPrefix(it.getJobName()),
                        Duration.between(it.getRightRevisionTimestamp(), to)
                ))
                .toList();
    }

    private static List<CheckTasksTable.MinRightRevisionTimestamp> getMinTimestampForRunningTasksGroupedByJobName(
            @Nonnull Instant from, @Nonnull Instant to, @Nonnull CheckType checkType, @Nonnull CiObserverDb db
    ) {
        return db.readOnly()
                .withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> db.tasks()
                        .getMinRightRevisionTimestampForRunningTasksGroupedByJobName(
                                from, to,
                                checkType,
                                List.of(IterationType.FAST, IterationType.FULL)
                        )
                );
    }

}
