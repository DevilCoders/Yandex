package yandex.cloud.team.integration.repository;

import yandex.cloud.auth.SystemAccountServiceFactory;
import yandex.cloud.repository.db.RepositoryTransaction;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.KikimrRepository;
import yandex.cloud.ts.BaseTokenServiceClient;

public class TeamIntegrationKikimrRepository extends KikimrRepository {

    public TeamIntegrationKikimrRepository(
            KikimrConfig config,
            SystemAccountServiceFactory systemAccountServiceFactory,
            BaseTokenServiceClient tokenServiceClient
    ) {
        super(config, systemAccountServiceFactory, tokenServiceClient);
    }


    @Override
    public RepositoryTransaction startTransaction(TxOptions options) {
        return new TeamIntegrationKikimrRepositoryTransaction(this, options);
    }

}
