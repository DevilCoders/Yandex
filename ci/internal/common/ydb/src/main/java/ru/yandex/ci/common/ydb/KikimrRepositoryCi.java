package ru.yandex.ci.common.ydb;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.yandex.ydb.core.grpc.GrpcTransport;

import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.KikimrRepository;
import yandex.cloud.repository.kikimr.KikimrRepositoryTransaction;
import yandex.cloud.repository.kikimr.client.SessionManager;
import yandex.cloud.repository.kikimr.client.YdbSchemaOperations;
import yandex.cloud.repository.kikimr.yql.YqlPrimitiveType;

public class KikimrRepositoryCi extends KikimrRepository {

    @Nullable
    private SessionManagerCI sessionManager;

    public KikimrRepositoryCi(@Nonnull KikimrConfig config) {
        super(config);
        YqlPrimitiveType.changeStringDefaultTypeToUtf8();
    }

    @Nonnull
    @Override
    protected YdbSchemaOperations buildSchemaOperations(@Nonnull KikimrConfig config, GrpcTransport transport) {
        if (sessionManager == null) {
            sessionManager = new SessionManagerCI(config, transport);
        }
        return super.buildSchemaOperations(config, transport);
    }

    @Override
    public SessionManager getSessionManager() {
        Preconditions.checkState(sessionManager != null, "sessionManager must be initialized");
        return sessionManager;
    }

    public static class KikimrRepositoryTransactionCi<R extends KikimrRepository>
            extends KikimrRepositoryTransaction<R> {

        public KikimrRepositoryTransactionCi(R kikimrRepository, @Nonnull TxOptions options) {
            super(kikimrRepository, options);
        }

        public TxOptions getOptions() {
            return this.options;
        }

    }
}
