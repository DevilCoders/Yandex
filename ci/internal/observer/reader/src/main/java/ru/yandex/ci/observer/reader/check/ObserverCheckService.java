package ru.yandex.ci.observer.reader.check;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTraceEntity;
import ru.yandex.ci.observer.core.utils.StageAggregationUtils;
import ru.yandex.ci.storage.core.Common;

@Slf4j
public class ObserverCheckService {
    private final CiObserverDb db;

    public ObserverCheckService(CiObserverDb db) {
        this.db = db;
    }

    /**
     * @return Is checkTask aggregation needed
     */
    public boolean processPartitionTrace(CheckTaskEntity.Id taskId, int partition, Common.TraceStage traceStage) {
        processPartitionTraceInternal(taskId, partition, traceStage);

        return isCheckTaskAggregationNeeded(taskId, traceStage.getType());
    }

    public boolean processStoragePartitionTrace(CheckTaskEntity.Id taskId, Common.TraceStage traceStage) {
        return processPartitionTrace(taskId, CheckTaskPartitionTraceEntity.Id.ALL_PARTITIONS, traceStage);
    }

    private void processPartitionTraceInternal(CheckTaskEntity.Id taskId, int partition, Common.TraceStage traceStage) {
        var traceType = traceStage.getType();
        var traceTime = ProtoConverter.convert(traceStage.getTimestamp());

        log.info(
                "Processing trace {} from task {}, partition {} with trace time {}",
                traceType, taskId, partition, traceTime
        );

        var traceId = new CheckTaskPartitionTraceEntity.Id(taskId, partition, traceType);

        if (db.readOnly(() -> db.traces().find(traceId)).isPresent()) {
            log.warn("Trace {} from task {}, partition {} already received, skipping...", traceType, taskId, partition);
            return;
        }

        var trace = CheckTaskPartitionTraceEntity.builder()
                .id(traceId)
                .time(traceTime)
                .attributes(traceStage.getAttributesMap())
                .build();

        db.tx(() -> db.traces().save(trace));
        log.info("Trace {} for task {}, partition {} processed and commited", traceStage.getType(), taskId, partition);
    }

    private static boolean isCheckTaskAggregationNeeded(CheckTaskEntity.Id taskId, String traceType) {
        if (!StageAggregationUtils.isAggregatableStageTrace(traceType)) {
            log.warn("Trace {} for task {} not from stage list, skipping...", traceType, taskId);
            return false;
        }

        return true;
    }
}
