package ru.yandex.ci.flow.engine.runtime;

import com.google.common.base.Preconditions;
import io.temporal.activity.ActivityExecutionContext;
import io.temporal.api.common.v1.WorkflowExecution;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.EqualsAndHashCode;

import ru.yandex.ci.ydb.Persisted;
import ru.yandex.commune.bazinga.impl.FullJobId;

@Persisted
@AllArgsConstructor(access = AccessLevel.PRIVATE)
@EqualsAndHashCode
public class TmsTaskId {
    private final TmsType type;
    private final String taskId;

    public static TmsTaskId fromBazingaId(FullJobId fullJobId) {
        return new TmsTaskId(TmsType.BAZINGA, fullJobId.toSerializedString());
    }

    public static TmsTaskId fromBazingaId(String fullJobId) {
        return new TmsTaskId(TmsType.BAZINGA, fullJobId);
    }

    public static TmsTaskId fromTemporal(WorkflowExecution execution) {
        return new TmsTaskId(TmsType.TEMPORAL, execution.getWorkflowId());
    }

    public static TmsTaskId fromTemporal(ActivityExecutionContext context) {
        return new TmsTaskId(TmsType.TEMPORAL, context.getInfo().getWorkflowId());
    }

    public boolean isBazinga() {
        return type == TmsType.BAZINGA;
    }

    public boolean isTemporal() {
        return type == TmsType.TEMPORAL;
    }

    public FullJobId getBazingaId() {
        Preconditions.checkState(isBazinga());
        return FullJobId.parse(taskId);
    }

    public String getTemporalWorkflowId() {
        Preconditions.checkState(isTemporal());
        return taskId;
    }

}
