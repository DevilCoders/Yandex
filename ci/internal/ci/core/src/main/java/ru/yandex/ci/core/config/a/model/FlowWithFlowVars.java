package ru.yandex.ci.core.config.a.model;

import java.util.Set;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.google.gson.JsonObject;
import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.util.gson.GsonElementSerializer;
import ru.yandex.ci.util.gson.ToGsonElementDeserializer;

@Value
@AllArgsConstructor
public class FlowWithFlowVars {
    String flow;

    @Nullable
    @JsonProperty("flow-vars")
    @JsonDeserialize(converter = ToGsonElementDeserializer.class)
    @JsonSerialize(converter = GsonElementSerializer.class)
    JsonObject flowVars;

    @Nullable
    @JsonProperty("flow-vars-ui")
    FlowVarsUi flowVarsUi;

    @Nullable
    @JsonProperty("accept-flows")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    Set<String> acceptFlows;

    @JsonCreator
    public FlowWithFlowVars(String flow) {
        this.flow = flow;
        this.flowVars = null;
        this.acceptFlows = null;
        this.flowVarsUi = null;
    }

    public boolean acceptFlow(String flowId) {
        return acceptFlows == null || acceptFlows.isEmpty() || acceptFlows.contains(flowId);
    }
}
