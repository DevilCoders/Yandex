package ru.yandex.ci.storage.core.check;

import java.util.HashSet;
import java.util.Set;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.ExpectedTask;

public class IterationUtils {
    private IterationUtils() {

    }

    public static CheckIterationEntity createMetaIteration(
            CheckIterationEntity firstIteration,
            Set<ExpectedTask> newExpectedTasks
    ) {
        var metaExpectedTasks = new HashSet<>(firstIteration.getExpectedTasks());
        metaExpectedTasks.addAll(newExpectedTasks);

        return firstIteration.toBuilder()
                .id(firstIteration.getId().toMetaId())
                .status(Common.CheckStatus.RUNNING)
                .expectedTasks(metaExpectedTasks)
                .testTypeStatistics(firstIteration.getTestTypeStatistics().resetCompleted())
                .finish(null)
                .tasksType(Common.CheckTaskType.CTT_AUTOCHECK)
                .build();
    }

    public static CheckIterationEntity restartMetaIteration(
            Set<ExpectedTask> newExpectedTasks, CheckIterationEntity iteration
    ) {
        var metaExpectedTasks = new HashSet<>(iteration.getExpectedTasks());
        metaExpectedTasks.addAll(newExpectedTasks);

        return iteration.toBuilder()
                .status(Common.CheckStatus.RUNNING)
                .finish(null)
                .expectedTasks(metaExpectedTasks)
                .testTypeStatistics(iteration.getTestTypeStatistics().resetCompleted())
                .build();
    }
}
