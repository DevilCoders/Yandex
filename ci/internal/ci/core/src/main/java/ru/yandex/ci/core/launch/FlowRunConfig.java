package ru.yandex.ci.core.launch;

import javax.annotation.Nullable;

import com.google.gson.JsonObject;
import lombok.RequiredArgsConstructor;
import lombok.Value;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.FlowVarsUi;
import ru.yandex.ci.core.config.a.model.FlowWithFlowVars;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.util.ObjectUtils;

/**
 * Вместе с указателем на флоу в конфиге могут быть заданы дополнительные свойства, как то flow-vars.
 * Этот класс собирает возможные переопределения по его указателю.
 */
@Value(staticConstructor = "of")
public class FlowRunConfig {
    @Nullable
    JsonObject flowVars;
    @Nullable
    FlowVarsUi flowVarsUi;

    public static FlowRunConfig lookup(CiProcessId ciProcessId, CiConfig config, FlowReference flowReference) {
        return new Lookup<>(new IdentityMapper()).lookup(ciProcessId, config, flowReference);
    }

    public static <T> T lookup(
            CiProcessId ciProcessId,
            CiConfig config,
            FlowReference flowReference,
            Mapper<T> mapper
    ) {
        return new Lookup<>(mapper).lookup(ciProcessId, config, flowReference);
    }

    @RequiredArgsConstructor
    private static class Lookup<T> {
        private final Mapper<T> mapper;

        public T lookup(CiProcessId ciProcessId, CiConfig config, FlowReference flowReference) {
            return switch (ciProcessId.getType()) {
                case FLOW -> config.findAction(ciProcessId.getSubId())
                        .map(this::fromAction)
                        .orElseGet(() -> fromFlow(config.getFlow(ciProcessId.getSubId())));
                case RELEASE -> lookupInRelease(config.getRelease(ciProcessId.getSubId()), flowReference);
                case SYSTEM -> throw new IllegalArgumentException("unexpected ci process type " + ciProcessId);
            };
        }

        @Nullable
        private T lookupInRelease(ReleaseConfig release, FlowReference flowReference) {
            if (flowReference.getFlowType() == Common.FlowType.FT_DEFAULT) {
                return fromReleaseConfig(release);
            }

            return fromReleaseFlowReference(release, flowReference);
        }

        private T fromFlow(FlowConfig flowConfig) {
            var runConfig = of(null, null);
            return mapper.fromFlow(runConfig, flowConfig);
        }

        private T fromAction(ActionConfig actionConfig) {
            var runConfig = of(actionConfig.getFlowVars(), actionConfig.getFlowVarsUi());
            return mapper.fromAction(runConfig, actionConfig);
        }

        private T fromReleaseConfig(ReleaseConfig releaseConfig) {
            var runConfig = of(releaseConfig.getFlowVars(), releaseConfig.getFlowVarsUi());
            return mapper.fromReleaseConfig(runConfig, releaseConfig);
        }

        private T fromReleaseFlowReference(ReleaseConfig releaseConfig, FlowReference reference) {
            var flowWithFlowVars = releaseConfig.getFlowWithFlowVars(reference);
            var runConfig = of(
                    ObjectUtils.firstNonNullOrNull(flowWithFlowVars.getFlowVars(), releaseConfig.getFlowVars()),
                    ObjectUtils.firstNonNullOrNull(flowWithFlowVars.getFlowVarsUi(), releaseConfig.getFlowVarsUi())
            );
            return mapper.fromReleaseFlowReference(runConfig, flowWithFlowVars);
        }
    }

    public interface Mapper<T> {
        T fromAction(FlowRunConfig runConfig, ActionConfig actionConfig);

        T fromFlow(FlowRunConfig runConfig, FlowConfig flowConfig);

        T fromReleaseConfig(FlowRunConfig runConfig, ReleaseConfig releaseConfig);

        T fromReleaseFlowReference(FlowRunConfig runConfig, FlowWithFlowVars flowWithFlowVars);
    }

    private static class IdentityMapper implements Mapper<FlowRunConfig> {

        @Override
        public FlowRunConfig fromAction(FlowRunConfig runConfig, ActionConfig actionConfig) {
            return runConfig;
        }

        @Override
        public FlowRunConfig fromFlow(FlowRunConfig runConfig, FlowConfig flowConfig) {
            return runConfig;
        }

        @Override
        public FlowRunConfig fromReleaseConfig(FlowRunConfig runConfig, ReleaseConfig releaseConfig) {
            return runConfig;
        }

        @Override
        public FlowRunConfig fromReleaseFlowReference(FlowRunConfig runConfig, FlowWithFlowVars flowWithFlowVars) {
            return runConfig;
        }
    }
}
