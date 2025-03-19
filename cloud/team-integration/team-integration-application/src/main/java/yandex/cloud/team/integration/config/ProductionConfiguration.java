package yandex.cloud.team.integration.config;

import java.util.List;

import lombok.AllArgsConstructor;
import yandex.cloud.config.ConfigLoader;
import yandex.cloud.di.Configuration;
import yandex.cloud.iam.config.DefaultSystemAccountServiceConfiguration;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.team.integration.repository.TeamIntegrationKikimrRepositoryConfiguration;
import yandex.cloud.ti.abc.repo.ydb.AbcIntegrationEntities;

@AllArgsConstructor
public class ProductionConfiguration extends AbstractTeamIntegrationConfiguration {

    @Override
    protected YamlConfiguration configConfiguration() {
        return new YamlConfiguration(ConfigLoader.getConfigFromEnv());
    }

    @Override
    protected Configuration repositoryConfiguration() {
        @SuppressWarnings("rawtypes")
        List<List<Class<? extends Entity>>> entities = List.of(
                AbcIntegrationEntities.entities
        );
        return new TeamIntegrationKikimrRepositoryConfiguration(entities);
    }

    @Override
    protected Configuration systemAccountServiceConfiguration() {
        return new DefaultSystemAccountServiceConfiguration();
    }

}
