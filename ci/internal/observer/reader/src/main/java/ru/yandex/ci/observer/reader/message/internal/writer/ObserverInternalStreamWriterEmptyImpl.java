package ru.yandex.ci.observer.reader.message.internal.writer;

import java.util.List;
import java.util.Map;
import java.util.Set;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.reader.message.FullTaskIdWithPartition;
import ru.yandex.ci.observer.reader.message.IterationFinishStatusWithTime;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;

@Slf4j
public class ObserverInternalStreamWriterEmptyImpl implements ObserverInternalStreamWriter {
    @Override
    public void onCheckTasksAggregation(Map<CheckIterationEntity.Id, Set<CheckTaskEntity.Id>> tasksToAggregation) {
        log.info("Writing {} aggregation messages to observer internal stream", tasksToAggregation.size());
    }

    @Override
    public void onIterationsCancel(List<CheckIterationEntity.Id> iterationsToCancel) {
        log.info("Writing {} cancel messages to observer internal stream", iterationsToCancel.size());
    }

    @Override
    public void onIterationsTechnicalStats(Map<FullTaskIdWithPartition, TechnicalStatistics> taskPartitionStats) {
        log.info("Writing {} technical statistics messages to observer internal stream", taskPartitionStats.size());
    }

    @Override
    public void onIterationsPessimize(List<CheckIterationEntity.Id> iterationsToPessimize) {
        log.info("Writing {} pessimize messages to observer internal stream", iterationsToPessimize.size());
    }

    @Override
    public void onIterationFinish(Map<CheckIterationEntity.Id, IterationFinishStatusWithTime> iterationFinishStatuses) {
        log.info("Writing {} iteration finish messages to observer internal stream", iterationFinishStatuses.size());
    }

    @Override
    public void onCheckTaskPartitionsFinish(
            Map<CheckIterationEntity.Id, List<FullTaskIdWithPartition>> taskPartitionsToFinish
    ) {
        log.info(
                "Writing {} finish task partition messages to observer internal stream",
                taskPartitionsToFinish.size()
        );
    }

    @Override
    public void onRegistered(List<EventsStreamMessages.EventsStreamMessage> registrationMessages) {
        log.info("Writing {} registered messages to observer internal stream", registrationMessages.size());
    }

    @Override
    public void onFatalError(List<CheckTaskOuterClass.FullTaskId> fatalErrorTaskIds) {
        log.info("Writing {} fatal error tasks to observer internal stream", fatalErrorTaskIds.size());
    }
}
