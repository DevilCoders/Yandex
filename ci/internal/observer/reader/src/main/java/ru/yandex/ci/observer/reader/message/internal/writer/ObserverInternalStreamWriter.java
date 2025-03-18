package ru.yandex.ci.observer.reader.message.internal.writer;

import java.util.List;
import java.util.Map;
import java.util.Set;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.reader.message.FullTaskIdWithPartition;
import ru.yandex.ci.observer.reader.message.IterationFinishStatusWithTime;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;

public interface ObserverInternalStreamWriter {
    void onCheckTasksAggregation(Map<CheckIterationEntity.Id, Set<CheckTaskEntity.Id>> tasksToAggregation);

    void onIterationsCancel(List<CheckIterationEntity.Id> iterationsToCancel);

    void onIterationsTechnicalStats(Map<FullTaskIdWithPartition, TechnicalStatistics> taskPartitionStats);

    void onIterationsPessimize(List<CheckIterationEntity.Id> iterationsToPessimize);

    void onIterationFinish(Map<CheckIterationEntity.Id, IterationFinishStatusWithTime> iterationFinishStatuses);

    void onCheckTaskPartitionsFinish(
            Map<CheckIterationEntity.Id, List<FullTaskIdWithPartition>> taskPartitionsToFinish
    );

    void onRegistered(List<EventsStreamMessages.EventsStreamMessage> registrationMessages);

    void onFatalError(List<CheckTaskOuterClass.FullTaskId> fatalErrorTaskIds);
}
