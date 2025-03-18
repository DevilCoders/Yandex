package ru.yandex.ci.storage.core.logbroker.event_producer;

import java.util.List;

import ru.yandex.ci.storage.core.CheckIteration.Iteration;
import ru.yandex.ci.storage.core.CheckOuterClass.Check;
import ru.yandex.ci.storage.core.CheckTaskOuterClass.CheckTask;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;

public interface StorageEventsProducer {
    void onIterationRegistered(Check check, Iteration iteration);

    void onTasksRegistered(Check check, Iteration iteration, List<CheckTask> tasks);

    void onCancelRequested(CheckIterationEntity.Id iterationId);

    void onTaskFinished(CheckTaskEntity.Id taskId);

    void onIterationFinished(CheckIterationEntity.Id iterationId, Common.CheckStatus iterationStatus);

    void onCheckFatalError(
            CheckEntity.Id checkId,
            List<CheckIterationEntity> runningIterations,
            CheckIterationEntity brokenIteration
    );
}
