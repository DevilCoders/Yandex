package ru.yandex.ci.core.autocheck;

import java.util.Optional;
import java.util.function.Supplier;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

public final class FlowVars {

    public static final String IS_PRECOMMIT = "is_precommit";
    public static final String IS_STRESS_TEST = "is_stress_test";
    public static final String CACHE_NAMESPACE = "cache_namespace";
    public static final String IS_SP_REGISTRATION_DISABLED = "is_sp_registration_disabled";

    private FlowVars() {
    }

    public static boolean getIsPrecommitOrThrow(JsonObject flowVars) {
        var name = FlowVars.IS_PRECOMMIT;
        return find(flowVars, name).map(JsonElement::getAsBoolean).orElseThrow(exceptionSupplier(name));
    }

    public static boolean getIsStressTest(JsonObject flowVars, boolean defaultValue) {
        return find(flowVars, FlowVars.IS_STRESS_TEST).map(JsonElement::getAsBoolean).orElse(defaultValue);
    }

    public static boolean getIsSpRegistrationDisabled(JsonObject flowVars, boolean defaultValue) {
        return find(flowVars, FlowVars.IS_SP_REGISTRATION_DISABLED).map(JsonElement::getAsBoolean).orElse(defaultValue);
    }

    private static Optional<JsonElement> find(JsonObject flowVars, String name) {
        return Optional.ofNullable(flowVars.get(name));
    }

    private static Supplier<IllegalStateException> exceptionSupplier(String fieldName) {
        return () -> new IllegalStateException("no %s value found among flow vars".formatted(fieldName));
    }

}
