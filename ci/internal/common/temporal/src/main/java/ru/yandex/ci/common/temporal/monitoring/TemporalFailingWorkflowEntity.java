package ru.yandex.ci.common.temporal.monitoring;

import javax.annotation.Nullable;

import io.temporal.activity.ActivityInfo;
import io.temporal.workflow.WorkflowInfo;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;

@Value
@AllArgsConstructor
@Builder
@Table(name = "temporal/FailingWorkflow")
public class TemporalFailingWorkflowEntity implements Entity<TemporalFailingWorkflowEntity> {

    Id id;

    String workflowType;

    String runId;

    @Nullable
    String activityId;

    int attemptNumber;

    @Nullable
    String exception;

    public boolean hasFailingActivity() {
        return activityId != null;
    }

    @Override
    public Entity.Id<TemporalFailingWorkflowEntity> getId() {
        return id;
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<TemporalFailingWorkflowEntity> {
        String id;

        public static Id of(WorkflowInfo info) {
            return of(info.getWorkflowId());
        }

        public static Id of(ActivityInfo info) {
            return of(info.getWorkflowId());
        }
    }

}
