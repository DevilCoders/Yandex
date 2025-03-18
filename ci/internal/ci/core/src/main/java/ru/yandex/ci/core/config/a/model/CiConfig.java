package ru.yandex.ci.core.config.a.model;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.google.common.base.Preconditions;
import lombok.Builder;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.Setter;
import lombok.ToString;
import lombok.Value;
import lombok.With;
import lombok.experimental.NonFinal;

import ru.yandex.ci.core.config.ConfigUtils;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.util.jackson.parse.HasParseInfo;
import ru.yandex.ci.util.jackson.parse.ParseInfo;

@SuppressWarnings("ReferenceEquality")
@Value
@Builder(buildMethodName = "buildInternal", toBuilder = true)
@JsonDeserialize(builder = CiConfig.Builder.class)
public class CiConfig implements HasParseInfo {

    @JsonProperty
    String secret;

    @JsonProperty("additional-secrets")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> additionalSecrets;

    @JsonProperty
    RequirementsConfig requirements;

    @JsonProperty
    RuntimeConfig runtime;

    @JsonProperty
    AutocheckConfig autocheck;

    @JsonProperty
    List<TriggerConfig> triggers;

    @With
    @JsonProperty
    LinkedHashMap<String, ReleaseConfig> releases;

    @JsonProperty
    LinkedHashMap<String, ActionConfig> actions;

    @With
    @JsonProperty
    LinkedHashMap<String, FlowConfig> flows;

    @JsonProperty("release-title-source")
    ReleaseTitleSource releaseTitleSource;

    @JsonProperty
    Permissions permissions;

    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    @JsonProperty("config-edit-approvals")
    List<PermissionRule> approvals;

    //

    @ToString.Exclude
    @EqualsAndHashCode.Exclude
    @JsonIgnore
    Map<String, ActionConfig> mergedActions;

    @ToString.Exclude
    @EqualsAndHashCode.Exclude
    @Getter(onMethod_ = @Override)
    @Setter(onMethod_ = @Override)
    @JsonIgnore
    @NonFinal
    transient ParseInfo parseInfo;

    // TODO: Use only for validation!
    public LinkedHashMap<String, ActionConfig> getActions() {
        return actions;
    }

    public Optional<ActionConfig> findAction(String actionId) {
        return Optional.ofNullable(mergedActions.get(actionId));
    }

    public ActionConfig getAction(String actionId) {
        return findAction(actionId).orElseThrow(() ->
                new IllegalArgumentException("Action not found " + actionId));
    }

    public Optional<ReleaseConfig> findRelease(String releaseId) {
        return Optional.ofNullable(releases.get(releaseId));
    }

    public ReleaseConfig getRelease(String releaseId) {
        return findRelease(releaseId).orElseThrow(() ->
                new IllegalArgumentException("Release not found " + releaseId));
    }

    public Optional<FlowConfig> findFlow(String flowId) {
        return Optional.ofNullable(flows.get(flowId));
    }

    public FlowConfig getFlow(String flowId) {
        return findFlow(flowId).orElseThrow(() ->
                new IllegalArgumentException("Flow not found " + flowId));
    }

    public static class Builder {
        private boolean firstAction = true;
        private boolean firstRelease = true;
        private boolean firstFlow = true;

        {
            triggers = new ArrayList<>();
            releases = new LinkedHashMap<>();
            flows = new LinkedHashMap<>();
            actions = new LinkedHashMap<>();
            additionalSecrets = new ArrayList<>();
            releaseTitleSource = ReleaseTitleSource.RELEASE;
        }

        public Builder action(ActionConfig... actions) {
            // The problem is - operation `toBuilder` will not copy the map without @Singular annotation,
            // only get it's reference

            // This is a solution to make sure we don't corrupt original map
            if (firstAction) {
                this.actions = new LinkedHashMap<>(this.actions);
                this.firstAction = false;
            }
            for (var action : actions) {
                this.actions.put(action.getId(), action);
            }
            return this;
        }

        public Builder release(ReleaseConfig... releases) {
            if (firstRelease) {
                this.releases = new LinkedHashMap<>(this.releases);
                this.firstRelease = false;
            }
            for (var release : releases) {
                this.releases.put(release.getId(), release);
            }
            return this;
        }

        public Builder flow(FlowConfig... flows) {
            if (firstFlow) {
                this.flows = new LinkedHashMap<>(this.flows);
                this.firstFlow = false;
            }
            for (var flow : flows) {
                this.flows.put(flow.getId(), flow);
            }
            return this;
        }

        public CiConfig build() {
            releases(ConfigUtils.update(this.releases));
            flows(ConfigUtils.update(this.flows));
            actions(ConfigUtils.update(this.actions));
            mergedActions(mergeActions(this.actions, this.triggers));
            return buildInternal();
        }
    }

    static Map<String, ActionConfig> mergeActions(LinkedHashMap<String, ActionConfig> actions,
                                                  List<TriggerConfig> triggers) {
        // We need to merge legacy triggers and new actions into single map
        // This code must be removed after complete migration to actions
        var mergedActions = new LinkedHashMap<>(actions);

        Map<String, List<TriggerConfig>> rawTriggers = new LinkedHashMap<>();
        for (var trigger : triggers) {
            var list = rawTriggers.computeIfAbsent(trigger.getFlow(), f -> new ArrayList<>());
            list.add(trigger.withFlow(null));
        }
        rawTriggers.forEach((flowId, value) -> {
            var action = ActionConfig.builder()
                    .id(flowId)
                    .flow(flowId)
                    .triggers(value)
                    .virtual(true)
                    .build();
            var prevAction = mergedActions.put(flowId, action);
            Preconditions.checkState(prevAction == null,
                    "Internal error, action with id %s and trigger for flow %s " +
                            "cannot be defined simultaneously. Consider migration to Actions");
        });

        return mergedActions;
    }
}
