package ru.yandex.ci.common.temporal.monitoring;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;
import lombok.Value;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;

@WorkflowInterface
public interface DeserializationExecptionWorkflow extends BaseTemporalWorkflow<DeserializationExecptionWorkflow.BadId> {
    @WorkflowMethod
    void run(BadId id);

    class Impl implements DeserializationExecptionWorkflow {
        @Override
        public void run(BadId id) {

        }
    }

    @Value
    class BadId implements BaseTemporalWorkflow.Id {

        String id;
        boolean bool;

        public BadId(String id) {
            this.id = id;
            bool = false;
        }

        public BadId(String id, boolean bool) {
            this.id = id;
            this.bool = bool;
        }

        @Override
        public String getTemporalWorkflowId() {
            return id;
        }
    }
}
