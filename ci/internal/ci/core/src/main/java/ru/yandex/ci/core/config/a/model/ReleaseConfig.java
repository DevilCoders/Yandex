package ru.yandex.ci.core.config.a.model;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.BaseJsonNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.google.gson.JsonObject;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.NonNull;
import lombok.Setter;
import lombok.Singular;
import lombok.ToString;
import lombok.Value;
import lombok.With;
import lombok.experimental.NonFinal;
import one.util.streamex.StreamEx;

import ru.yandex.ci.core.config.ConfigIdEntry;
import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.config.a.model.auto.AutoReleaseConfig;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.launch.FlowReference;
import ru.yandex.ci.util.gson.GsonElementSerializer;
import ru.yandex.ci.util.gson.ToGsonElementDeserializer;
import ru.yandex.ci.util.jackson.parse.HasParseInfo;
import ru.yandex.ci.util.jackson.parse.ParseInfo;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(buildMethodName = "buildInternal")
@JsonDeserialize(builder = ReleaseConfig.Builder.class)
public class ReleaseConfig implements ConfigIdEntry<ReleaseConfig>, HasParseInfo, HasFlowRef {

    @Getter(onMethod_ = @Override)
    @With(onMethod_ = @Override)
    @JsonIgnore
    String id;

    @JsonProperty
    String title;

    @JsonProperty
    String description;

    @Getter(onMethod_ = @Override)
    @JsonProperty
    String flow;

    @Singular
    @JsonProperty("hotfix-flows")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<FlowWithFlowVars> hotfixFlows;

    @With
    @Singular
    @JsonProperty("rollback-flows")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<FlowWithFlowVars> rollbackFlows;

    @JsonProperty
    @Singular
    List<FilterConfig> filters;

    @Nullable
    @JsonProperty
    AutoReleaseConfig auto;

    @Nullable
    @Singular
    List<StageConfig> stages;

    @Nullable
    @JsonProperty
    ReleaseBranchesConfig branches;

    @Nullable
    @JsonProperty("start-version")
    Integer startVersion;

    @Nullable
    @JsonProperty("flow-vars")
    @JsonDeserialize(converter = ToGsonElementDeserializer.class)
    @JsonSerialize(converter = GsonElementSerializer.class)
    JsonObject flowVars;

    @Nullable
    @JsonProperty("flow-vars-ui")
    FlowVarsUi flowVarsUi;

    @JsonProperty
    RequirementsConfig requirements;

    @Nullable
    @JsonProperty("runtime")
    RuntimeConfig runtimeConfig;

    @Nullable
    @JsonProperty("displacement-on-manual-start")
    DisplacementOnStart displacementOnStart;

    @Singular
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> tags;

    @JsonProperty
    Permissions permissions;

    @ToString.Exclude
    @EqualsAndHashCode.Exclude
    @Getter(onMethod_ = @Override)
    @Setter(onMethod_ = @Override)
    @NonFinal
    @JsonIgnore
    transient ParseInfo parseInfo;

    public String getTitle() {
        return Objects.requireNonNullElse(title, id);
    }

    public boolean hasDisplacement() {
        for (var stage : getStages()) {
            if (stage.getDisplace() != null) {
                if (!stage.getDisplace().getOnStatus().isEmpty()) {
                    return true;
                }
            }
        }
        return false;
    }

    public DisplacementOnStart getDisplacementOnStart() {
        return Objects.requireNonNullElse(displacementOnStart, DisplacementOnStart.AUTO);
    }

    @JsonIgnore
    public boolean isIndependentStages() {
        return getBranches().isIndependentStages();
    }

    public List<String> getTags() {
        return Objects.requireNonNullElse(tags, List.of());
    }

    public FlowWithFlowVars getFlowWithFlowVars(FlowReference flowReference) {
        var flowsWithFlowVar = switch (flowReference.getFlowType()) {
            case FT_HOTFIX -> getHotfixFlows();
            case FT_ROLLBACK -> getRollbackFlows();
            case FT_DEFAULT, UNRECOGNIZED -> throw new IllegalArgumentException("Internal error. " +
                    "Unable to validate flow type for " + flowReference.getFlowType());
        };

        return flowsWithFlowVar.stream()
                .filter(f -> Objects.equals(flowReference.getFlowId(), f.getFlow()))
                .findFirst()
                .orElseThrow(() -> {
                    var knownFlowIds = flowsWithFlowVar.stream().map(FlowWithFlowVars::getFlow).toList();
                    return new IllegalStateException(
                            "Invalid flow %s of %s, known flows of this type: %s".formatted(
                                    flowReference.getFlowId(), flowReference.getFlowType(), knownFlowIds
                            ));
                });
    }

    @NonNull
    public AutoReleaseConfig getAuto() {
        return Objects.requireNonNullElse(auto, AutoReleaseConfig.EMPTY);
    }

    @NonNull
    public ReleaseBranchesConfig getBranches() {
        return Objects.requireNonNullElse(branches, ReleaseBranchesConfig.EMPTY);
    }

    public enum DisplacementOnStart {
        @JsonProperty("auto")
        AUTO,
        @JsonProperty("enabled")
        ENABLED,
        @JsonProperty("disabled")
        DISABLED
    }

    @JsonIgnoreProperties(ignoreUnknown = true) // rollback-config
    public static class Builder {

        @JsonProperty("stages")
        public void setStagesMap(BaseJsonNode node) {
            // Больше костылей богу костылей, поддерживаем два формата стейджей.
            var value = new ArrayList<StageConfig>();
            if (node instanceof ObjectNode) {
                StreamEx.of(node.fieldNames())
                        .forEach(key -> {
                            try {
                                var stage = AYamlParser.getMapper().treeToValue(node.get(key), StageConfig.class);
                                value.add(stage.withId(key));
                            } catch (JsonProcessingException e) {
                                throw new RuntimeException("Failed to parse", e);
                            }

                        });
            } else if (node instanceof ArrayNode) {
                node.forEach(element -> {
                    try {
                        value.add(AYamlParser.getMapper().treeToValue(element, StageConfig.class));
                    } catch (JsonProcessingException e) {
                        throw new RuntimeException("Failed to parse", e);
                    }
                });
            } else {
                throw new RuntimeException("unknown node");
            }

            this.stages = value;
        }

        public ReleaseConfig build() {
            if (stages == null || stages.isEmpty()) {
                stage(StageConfig.IMPLICIT_STAGE);
            }
            return buildInternal();
        }

    }
}
