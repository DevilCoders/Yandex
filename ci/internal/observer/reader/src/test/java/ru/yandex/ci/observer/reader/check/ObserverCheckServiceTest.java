package ru.yandex.ci.observer.reader.check;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.observer.core.db.model.traces.CheckTaskPartitionTraceEntity;
import ru.yandex.ci.observer.reader.ObserverReaderYdbTestBase;
import ru.yandex.ci.storage.core.Common;

public class ObserverCheckServiceTest extends ObserverReaderYdbTestBase {
    private static final Common.TraceStage STAGE_TRACE = Common.TraceStage.newBuilder()
            .setType("distbuild/started")
            .setTimestamp(ProtoConverter.convert(TIME))
            .build();
    private static final Common.TraceStage NOT_STAGE_TRACE = Common.TraceStage.newBuilder()
            .setType("distbuild/other_trace")
            .setTimestamp(ProtoConverter.convert(TIME))
            .build();

    private ObserverCheckService checkService;

    @BeforeEach
    public void setup() {
        this.checkService = new ObserverCheckService(db);
    }

    @Test
    void processPartitionTrace_WhenTraceFromAggregationList() {
        var traceId = new CheckTaskPartitionTraceEntity.Id(SAMPLE_TASK_ID, 0, STAGE_TRACE.getType());
        var expectedTrace = CheckTaskPartitionTraceEntity.builder().id(traceId).time(TIME).build();

        var isAggregationNeeded = checkService.processPartitionTrace(SAMPLE_TASK_ID, 0, STAGE_TRACE);

        var trace = cache.traces().getFreshOrThrow(traceId);

        Assertions.assertTrue(isAggregationNeeded);
        Assertions.assertEquals(expectedTrace, trace);
    }

    @Test
    void processPartitionTrace_WhenTraceNotFromAggregationList() {
        var traceId = new CheckTaskPartitionTraceEntity.Id(SAMPLE_TASK_ID, 0, NOT_STAGE_TRACE.getType());
        var expectedTrace = CheckTaskPartitionTraceEntity.builder().id(traceId).time(TIME).build();

        var isAggregationNeeded = checkService.processPartitionTrace(SAMPLE_TASK_ID, 0, NOT_STAGE_TRACE);

        var trace = cache.traces().getFreshOrThrow(traceId);

        Assertions.assertFalse(isAggregationNeeded);
        Assertions.assertEquals(expectedTrace, trace);
    }

    @Test
    void processStoragePartitionTrace_WhenTraceFromAggregationList() {
        var traceId = new CheckTaskPartitionTraceEntity.Id(SAMPLE_TASK_ID, -1, STAGE_TRACE.getType());
        var expectedTrace = CheckTaskPartitionTraceEntity.builder().id(traceId).time(TIME).build();

        var isAggregationNeeded = checkService.processStoragePartitionTrace(SAMPLE_TASK_ID,  STAGE_TRACE);

        var trace = cache.traces().getFreshOrThrow(traceId);

        Assertions.assertTrue(isAggregationNeeded);
        Assertions.assertEquals(expectedTrace, trace);
    }

    @Test
    void processStoragePartitionTrace_WhenTraceNotFromAggregationList() {
        var traceId = new CheckTaskPartitionTraceEntity.Id(SAMPLE_TASK_ID, -1, NOT_STAGE_TRACE.getType());
        var expectedTrace = CheckTaskPartitionTraceEntity.builder().id(traceId).time(TIME).build();

        var isAggregationNeeded = checkService.processStoragePartitionTrace(SAMPLE_TASK_ID,  NOT_STAGE_TRACE);

        var trace = cache.traces().getFreshOrThrow(traceId);

        Assertions.assertFalse(isAggregationNeeded);
        Assertions.assertEquals(expectedTrace, trace);
    }
}
