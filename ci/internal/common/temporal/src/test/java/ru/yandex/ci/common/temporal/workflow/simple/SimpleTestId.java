package ru.yandex.ci.common.temporal.workflow.simple;

import java.time.LocalDateTime;
import java.time.ZoneId;

import lombok.Value;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;

@Value(staticConstructor = "of")
public class SimpleTestId implements BaseTemporalWorkflow.Id {
    String id;

    public static SimpleTestId ofCurrentDate() {
        return of(LocalDateTime.now(ZoneId.systemDefault()).toString());
    }

    @Override
    public String getTemporalWorkflowId() {
        return id;
    }

}
