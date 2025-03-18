package ru.yandex.ci.client.sandbox.api;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.RequiredArgsConstructor;
import lombok.Value;

@Value
@RequiredArgsConstructor
public class SandboxTaskPriority {

    @Nullable
    @JsonProperty("class")
    PriorityClass clazz;

    @Nullable
    PrioritySubclass subclass;

    public SandboxTaskPriority() {
        this(null, null);
    }

    public enum PriorityClass {
        USER, SERVICE, BACKGROUND
    }

    public enum PrioritySubclass {
        LOW, NORMAL, HIGH
    }
}
