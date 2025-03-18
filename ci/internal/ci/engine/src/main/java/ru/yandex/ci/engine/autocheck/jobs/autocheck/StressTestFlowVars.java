package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.Objects;
import java.util.Optional;
import java.util.function.Supplier;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

import ru.yandex.ci.flow.engine.definition.context.JobContext;

public class StressTestFlowVars {

    private static final String DURATION_HOURS = "duration_hours";
    private static final String REVISIONS_PER_HOURS = "revisions_per_hours";
    private static final String NAMESPACE = "namespace";

    private StressTestFlowVars() {
    }

    public static JsonObject getFlowVars(JobContext context) {
        return Objects.requireNonNull(context.getFlowLaunch().getFlowInfo().getFlowVars()).getData();
    }

    public static int getDurationHours(JsonObject flowVars) {
        var name = DURATION_HOURS;
        return find(flowVars, name).map(JsonElement::getAsInt).orElseThrow(exceptionSupplier(name));
    }

    public static int getRevisionsPerHours(JsonObject flowVars) {
        var name = REVISIONS_PER_HOURS;
        return find(flowVars, name).map(JsonElement::getAsInt).orElseThrow(exceptionSupplier(name));
    }

    public static String getNamespace(JsonObject flowVars) {
        var name = NAMESPACE;
        return find(flowVars, name).map(JsonElement::getAsString).orElseThrow(exceptionSupplier(name));
    }

    private static Optional<JsonElement> find(JsonObject flowVars, String name) {
        return Optional.ofNullable(flowVars.get(name));
    }

    private static Supplier<IllegalStateException> exceptionSupplier(String fieldName) {
        return () -> new IllegalStateException("no %s value found among flow vars".formatted(fieldName));
    }

}
