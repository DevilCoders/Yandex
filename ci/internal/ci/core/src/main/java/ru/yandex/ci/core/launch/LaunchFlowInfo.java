package ru.yandex.ci.core.launch;

import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.model.CleanupConfig;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.util.gson.GsonJacksonDeserializer;
import ru.yandex.ci.util.gson.GsonJacksonSerializer;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder(toBuilder = true)
@JsonSerialize(using = GsonJacksonSerializer.class)
@JsonDeserialize(using = GsonJacksonDeserializer.class)
@JsonIgnoreProperties(ignoreUnknown = true) // cleanupTrigger, manualResources, skipStagesAllowed
public class LaunchFlowInfo {
    @Nonnull
    OrderedArcRevision configRevision;

    @Nonnull
    FlowFullId flowId;

    @Nullable
    String stageGroupId;

    @Nonnull
    LaunchRuntimeInfo runtimeInfo;

    //TODO проставлять осознано
    boolean skipStagesAllowed = false;

    @Nullable
    LaunchFlowDescription flowDescription;

    @Nullable
    JobResource flowVars;

    @Nullable
    CleanupConfig cleanupConfig;

    @Nullable
    List<String> rollbackFlows;

    @Nullable
    Version rollbackToVersion;

    public List<String> getRollbackFlows() {
        return Objects.requireNonNullElse(rollbackFlows, List.of());
    }
}
