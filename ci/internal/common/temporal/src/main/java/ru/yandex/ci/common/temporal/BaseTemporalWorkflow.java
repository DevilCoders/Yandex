package ru.yandex.ci.common.temporal;

import com.fasterxml.jackson.annotation.JsonIgnore;
import lombok.Value;

/**
 * Basic interface for all flows. Note that all due to Workflow Interface Inheritance
 * <a href="https://docs.temporal.io/docs/java/workflows#workflow-interface-inheritance">limitations</a>
 * you should create method with @WorkflowMethod annotation and T input in your workflow interface.
 * Extended interface should also have @WorkflowInterface annotation.
 */
public interface BaseTemporalWorkflow<T extends BaseTemporalWorkflow.Id> {

    Id EMPTY_ID = new EmptyId();

    interface Id {
        /**
         * Used for deduplication
         */
        @JsonIgnore
        String getTemporalWorkflowId();
    }

    @Value
    class EmptyId implements Id {

        @Override
        public String getTemporalWorkflowId() {
            throw new UnsupportedOperationException();
        }
    }
}
