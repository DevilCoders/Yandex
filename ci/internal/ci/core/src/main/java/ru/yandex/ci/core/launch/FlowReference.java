package ru.yandex.ci.core.launch;

import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;

/**
 * Указатель на флоу из релиза.
 * Релиз может быть запущен с кастомным флоу, например, хотфиксовым.
 */
@Value
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class FlowReference {
    @Nonnull
    String flowId;

    @Nonnull
    Common.FlowType flowType;

    public static FlowReference from(Common.FlowProcessId flowProcessId, Common.FlowType flowType) {
        return new FlowReference(flowProcessId.getId(), Objects.requireNonNullElse(flowType,
                Common.FlowType.FT_DEFAULT));
    }

    public static FlowReference defaultIfNull(
            CiConfig config,
            CiProcessId processId,
            @Nullable FlowReference flowReference
    ) {
        if (flowReference == null) {
            switch (processId.getType()) {
                case FLOW -> {
                    var flowId = config.findAction(processId.getSubId())
                            .map(ActionConfig::getFlow)
                            .orElse(processId.getSubId());
                    return new FlowReference(flowId, Common.FlowType.FT_DEFAULT);
                }
                case RELEASE -> {
                    var flowId = config.getRelease(processId.getSubId()).getFlow();
                    return new FlowReference(flowId, Common.FlowType.FT_DEFAULT);
                }
                default -> {
                    return new FlowReference(processId.getSubId(), Common.FlowType.FT_DEFAULT);
                }
            }
        }

        if (flowReference.getFlowType() == Common.FlowType.FT_DEFAULT
                && processId.getType().isRelease()
                && !flowReference.getFlowId().equals(config.getRelease(processId.getSubId()).getFlow())) {
            // TODO: поменять после того как из фронта начнет всегда приходить flow_type
            return new FlowReference(flowReference.getFlowId(), Common.FlowType.FT_HOTFIX);
        }

        return flowReference;
    }

    public static FlowReference of(String flowId, Common.FlowType flowType) {
        return new FlowReference(flowId, flowType);
    }
}
