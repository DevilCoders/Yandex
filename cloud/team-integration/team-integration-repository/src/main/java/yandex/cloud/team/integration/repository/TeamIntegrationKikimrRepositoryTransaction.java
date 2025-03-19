package yandex.cloud.team.integration.repository;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.kikimr.KikimrRepositoryTransaction;

class TeamIntegrationKikimrRepositoryTransaction extends KikimrRepositoryTransaction<TeamIntegrationKikimrRepository> implements TeamIntegrationRepositoryTransaction {

    TeamIntegrationKikimrRepositoryTransaction(
            @NotNull TeamIntegrationKikimrRepository kikimrRepository,
            @NotNull TxOptions options
    ) {
        super(kikimrRepository, options);
    }

}
