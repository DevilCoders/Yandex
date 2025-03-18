package ru.yandex.ci.storage.core.yt;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

@WorkflowInterface
public interface YtExportWorkflow extends BaseTemporalWorkflow<CheckIterationEntity.Id> {

    String QUEUE = "yt-export";

    @WorkflowMethod
    void exportIteration(CheckIterationEntity.Id id);

}
