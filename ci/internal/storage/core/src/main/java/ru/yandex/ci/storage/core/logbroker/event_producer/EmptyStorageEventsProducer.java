package ru.yandex.ci.storage.core.logbroker.event_producer;

import java.util.List;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.CheckIteration.Iteration;
import ru.yandex.ci.storage.core.CheckOuterClass.Check;
import ru.yandex.ci.storage.core.CheckTaskOuterClass.CheckTask;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;

@Slf4j
public class EmptyStorageEventsProducer implements StorageEventsProducer {

    @Override
    public void onIterationRegistered(Check check, Iteration iteration) {
        log.info("Iteration registered {}", iteration.getId());
    }

    @Override
    public void onTasksRegistered(Check check, Iteration iteration, List<CheckTask> tasks) {
        for (var task : tasks) {
            log.info("Task registered {}", task.getId());
        }
    }

    @Override
    public void onCancelRequested(CheckIterationEntity.Id iterationId) {
        log.info("Cancel requested {}", iterationId);
    }

    @Override
    public void onTaskFinished(CheckTaskEntity.Id taskId) {
        log.info("Task {} finished", taskId);
    }

    @Override
    public void onIterationFinished(CheckIterationEntity.Id iterationId, Common.CheckStatus iterationStatus) {
        log.info("Iteration {} finished with status {}", iterationId, iterationStatus);
    }

    @Override
    public void onCheckFatalError(
            CheckEntity.Id checkId,
            List<CheckIterationEntity> runningIterations,
            CheckIterationEntity brokenIteration
    ) {
        log.info("Iteration {} finished with fatal error in status {}", brokenIteration.getId(),
                brokenIteration.getStatus());
    }
}
