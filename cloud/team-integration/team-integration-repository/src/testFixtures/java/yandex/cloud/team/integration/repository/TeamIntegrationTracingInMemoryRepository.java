package yandex.cloud.team.integration.repository;

import yandex.cloud.iam.repository.tracing.TracingInMemoryRepository;
import yandex.cloud.iam.repository.tracing.TracingRepositoryTransaction;
import yandex.cloud.repository.db.TxOptions;

public class TeamIntegrationTracingInMemoryRepository extends TracingInMemoryRepository {

    public TeamIntegrationTracingInMemoryRepository() {
        super(new TeamIntegrationTracingKikimrRepository());
    }

    @Override
    public TracingRepositoryTransaction startPrimaryTransaction(TxOptions options) {
        return new TeamIntegrationTracingInMemoryRepositoryTransaction(options);
    }

    private class TeamIntegrationTracingInMemoryRepositoryTransaction extends TracingInMemoryRepositoryTransaction implements TeamIntegrationRepositoryTransaction {

        TeamIntegrationTracingInMemoryRepositoryTransaction(TxOptions options) {
            super(options);
        }

    }

}
