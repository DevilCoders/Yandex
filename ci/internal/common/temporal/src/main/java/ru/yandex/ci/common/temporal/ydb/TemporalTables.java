package ru.yandex.ci.common.temporal.ydb;

import ru.yandex.ci.common.temporal.monitoring.TemporalFailingWorkflowTable;

public interface TemporalTables {
    TemporalLaunchQueueTable temporalLaunchQueue();

    TemporalFailingWorkflowTable temporalFailingWorkflow();
}
