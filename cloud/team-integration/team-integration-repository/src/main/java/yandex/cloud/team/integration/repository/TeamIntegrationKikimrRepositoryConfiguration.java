package yandex.cloud.team.integration.repository;

import java.util.Collection;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.auth.SystemAccountServiceFactory;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.Repository;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.ts.BaseTokenServiceClient;

public class TeamIntegrationKikimrRepositoryConfiguration extends AbstractTeamIntegrationRepositoryConfiguration {

    public TeamIntegrationKikimrRepositoryConfiguration(
            @SuppressWarnings("rawtypes") @NotNull Collection<? extends Collection<Class<? extends Entity>>> entities
    ) {
        super(entities);
    }


    @Override
    protected Repository repository() {
        return new TeamIntegrationKikimrRepository(
                get(KikimrConfig.class),
                get(SystemAccountServiceFactory.class),
                get(BaseTokenServiceClient.class)
        );
    }

}
