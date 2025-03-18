package ru.yandex.ci.common.temporal.workflow.simple;

import lombok.Value;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;

@Value(staticConstructor = "of")
public class SimpleTestLongId implements BaseTemporalWorkflow.Id {
    long id;

    public static SimpleTestLongId ofCurrentDate() {
        return of(System.currentTimeMillis());
    }

    @Override
    public String getTemporalWorkflowId() {
        return String.valueOf(id);
    }

}
