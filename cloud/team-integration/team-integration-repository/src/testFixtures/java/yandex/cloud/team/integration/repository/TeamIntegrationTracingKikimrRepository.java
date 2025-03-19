package yandex.cloud.team.integration.repository;

import yandex.cloud.auth.SystemAccountServiceFactory;
import yandex.cloud.iam.repository.tracing.TracingRepository;
import yandex.cloud.iam.repository.tracing.TracingRepositoryTransaction;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.kikimr.KikimrConfig;

class TeamIntegrationTracingKikimrRepository extends TeamIntegrationKikimrRepository implements TracingRepository {

    TeamIntegrationTracingKikimrRepository() {
        super(
                KikimrConfig.create("127.0.0.1", 0, "/", "/"),
                SystemAccountServiceFactory.getDefaultFactory(),
                null
        );
    }

    @Override
    public TracingRepositoryTransaction startTransaction(TxOptions options) {
        return new TeamIntegrationTracingKikimrRepositoryTransaction(this, options);
    }

}
