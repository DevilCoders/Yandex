package ru.yandex.ci.engine.discovery.util;

import com.google.common.collect.Multimaps;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.engine.config.ConfigBundle;

public class ConfigStates {

    private ConfigStates() {
        //
    }

    public static ConfigState.Builder prepareConfigState(
            ConfigBundle configBundle,
            ArcCommit commit
    ) {

        var configStateBuilder = ConfigState.builder()
                .configPath(configBundle.getConfigPath())
                .created(commit.getCreateTime())
                .updated(commit.getCreateTime())
                .lastRevision(configBundle.getRevision());

        configBundle.getOptionalAYamlConfig()
                .map(AYamlConfig::getService)
                .ifPresent(configStateBuilder::project);

        configBundle.getOptionalAYamlConfig()
                .map(AYamlConfig::getTitle)
                .ifPresent(configStateBuilder::title);

        if (configBundle.getStatus().isValidCiConfig()) {
            configStateBuilder.lastValidRevision(configBundle.getRevision());
            CiConfig ciConfig = configBundle.getValidAYamlConfig().getCi();

            var flowToActions = Multimaps.index(ciConfig.getMergedActions().values(), ActionConfig::getFlow);

            for (var flow : ciConfig.getFlows().values()) {
                configStateBuilder.actions(ActionConfigState.of(flow, flowToActions));
            }
            for (var release : ciConfig.getReleases().values()) {
                configStateBuilder.release(ReleaseConfigState.of(release));
            }
        }

        return configStateBuilder;
    }

}
