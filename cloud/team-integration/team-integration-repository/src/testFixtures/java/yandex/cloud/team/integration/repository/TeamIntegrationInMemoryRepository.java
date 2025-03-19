package yandex.cloud.team.integration.repository;

import yandex.cloud.repository.db.RepositoryTransaction;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.test.inmemory.InMemoryRepository;

class TeamIntegrationInMemoryRepository extends InMemoryRepository {

    @Override
    public RepositoryTransaction startTransaction(TxOptions options) {
        return new TeamIntegrationInMemoryRepositoryTransaction(options);
    }

    private class TeamIntegrationInMemoryRepositoryTransaction extends InMemoryRepositoryTransaction implements TeamIntegrationRepositoryTransaction {

        TeamIntegrationInMemoryRepositoryTransaction(TxOptions options) {
            super(options);
        }

    }

}
