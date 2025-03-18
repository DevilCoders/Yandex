package ru.yandex.ci.client.testenv.model;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Value;

@Value
public class TestResultDto {
    int id;
    int revision;
    Status status;
    @JsonProperty("task_id")
    String taskId;

    public enum Status {
        OK,
        ERROR,
        FILTERED,
        EXECUTING,
        PARENT_ERROR,
        NOT_CHECKED,
        WAIT_PARENT,
        @JsonEnumDefaultValue
        UNKNOWN,
        INTERNAL,
        CANCELLED,
    }
}
