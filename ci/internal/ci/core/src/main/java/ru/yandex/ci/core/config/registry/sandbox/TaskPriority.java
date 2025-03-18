package ru.yandex.ci.core.config.registry.sandbox;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("ReferenceEquality")
@Persisted
@Value
@AllArgsConstructor
public class TaskPriority {

    @With
    @Nullable
    @JsonProperty
    String expression;

    @Nullable
    @JsonProperty("class")
    @JsonAlias("clazz")
    PriorityClass clazz;

    @Nullable
    PrioritySubclass subclass;

    @JsonCreator
    public TaskPriority(String expression) {
        this(expression, null, null);
    }

    public TaskPriority(
            @JsonProperty("class") @JsonAlias("clazz") PriorityClass clazz,
            @JsonProperty("subclass") PrioritySubclass subclass
    ) {
        this(null, clazz, subclass);
    }

    @Persisted
    public enum PriorityClass {
        USER, SERVICE, BACKGROUND
    }

    @Persisted
    public enum PrioritySubclass {
        LOW, NORMAL, HIGH
    }

}
