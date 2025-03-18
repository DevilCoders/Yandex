package ru.yandex.ci.core.config.a.model;

import java.util.HashMap;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonProperty;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum PermissionScope {
    @JsonProperty("modify")
    @JsonAlias("MODIFY")
    MODIFY(true, true),

    @JsonProperty("start-flow")
    @JsonAlias("START_FLOW")
    START_FLOW(true, false),

    @JsonProperty("start-hotfix")
    @JsonAlias("START_HOTFIX")
    START_HOTFIX(true, false),

    @JsonProperty("cancel-flow")
    @JsonAlias("CANCEL_FLOW")
    CANCEL_FLOW(false, true),

    @JsonProperty("rollback-flow")
    @JsonAlias("ROLLBACK_FLOW")
    ROLLBACK_FLOW(true, false),


    @JsonProperty("add-commit")
    @JsonAlias("ADD_COMMIT")
    ADD_COMMIT(true, false),

    @JsonProperty("create-branch")
    @JsonAlias("CREATE_BRANCH")
    CREATE_BRANCH(true, false),


    @JsonProperty("start-job")
    @JsonAlias("START_JOB")
    START_JOB(false, true),

    @JsonProperty("kill-job")
    @JsonAlias("KILL_JOB")
    KILL_JOB(false, true),

    @JsonProperty("skip-job")
    @JsonAlias("SKIP_JOB")
    SKIP_JOB(false, true),

    @JsonProperty("manual-trigger")
    @JsonAlias("MANUAL_TRIGGER")
    MANUAL_TRIGGER(false, true),

    @JsonProperty("toggle-autorun")
    @JsonAlias("TOGGLE_AUTORUN")
    TOGGLE_AUTORUN(true, false);

    private static final Map<String, PermissionScope> SCOPES;

    private final boolean checkWithoutRevision;
    private final boolean checkWithRevision;

    PermissionScope(boolean checkWithoutRevision, boolean checkWithRevision) {
        this.checkWithoutRevision = checkWithoutRevision;
        this.checkWithRevision = checkWithRevision;
    }

    public boolean isCheckWithoutRevision() {
        return checkWithoutRevision;
    }

    public boolean isCheckWithRevision() {
        return checkWithRevision;
    }

    public static PermissionScope ofJsonProperty(String property) {
        var value = SCOPES.get(property);
        return value != null
                ? value
                : valueOf(property);
    }

    static {
        var cache = new HashMap<String, PermissionScope>();
        for (var scope : PermissionScope.values()) {
            try {
                var property = PermissionScope.class.getField(scope.name()).getAnnotation(JsonProperty.class);
                cache.put(property.value(), scope);
            } catch (NoSuchFieldException e) {
                throw new RuntimeException("Unable to access to " + scope, e);
            }

        }

        SCOPES = Map.copyOf(cache);
    }
}
