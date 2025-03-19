package yandex.cloud.team.integration.repository;

import java.util.Collection;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.Repository;

public class TeamIntegrationInMemoryRepositoryConfiguration extends AbstractTeamIntegrationRepositoryConfiguration {

    public TeamIntegrationInMemoryRepositoryConfiguration(
            @SuppressWarnings("rawtypes") @NotNull Collection<? extends Collection<Class<? extends Entity>>> entities
    ) {
        super(entities);
    }

    @Override
    protected Repository repository() {
        return new TeamIntegrationInMemoryRepository();
    }

}
