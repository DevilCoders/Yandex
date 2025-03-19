package yandex.cloud.team.integration.repository;

import java.util.List;
import java.util.stream.Stream;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.repository.tracing.TracingAbstractTable;
import yandex.cloud.iam.repository.tracing.TracingQueryExecutorPartialImpl;
import yandex.cloud.iam.repository.tracing.TracingRepositoryTransaction;
import yandex.cloud.iam.repository.tracing.TracingRepositoryTransactionPartialImpl;
import yandex.cloud.repository.db.AbstractTable;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.TxOptions;
import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.readtable.ReadTableMapper;
import yandex.cloud.repository.kikimr.statement.Statement;

class TeamIntegrationTracingKikimrRepositoryTransaction extends TeamIntegrationKikimrRepositoryTransaction implements TracingRepositoryTransaction {

    private final @NotNull TracingRepositoryTransactionPartialImpl tracingRepositoryTransaction;
    private final @NotNull TracingQueryExecutorPartialImpl tracingQueryExecutor;


    TeamIntegrationTracingKikimrRepositoryTransaction(
            @NotNull TeamIntegrationKikimrRepository kikimrRepository,
            @NotNull TxOptions options
    ) {
        super(kikimrRepository, options);
        tracingRepositoryTransaction = new TracingRepositoryTransactionPartialImpl();
        tracingQueryExecutor = new TracingQueryExecutorPartialImpl();
    }


    @Override
    public void setOuterTransaction(TracingRepositoryTransaction outerTransaction) {
        tracingRepositoryTransaction.setOuterTransaction(outerTransaction);
    }

    @Override
    public <T extends Entity<T>> TracingAbstractTable<T> table(Class<T> c) {
        return tracingRepositoryTransaction.table(c);
    }

    @Override
    public <T extends Entity<T>> AbstractTable<T> tableImpl(Class<T> c) {
        return super.table(c);
    }

    @Override
    public <PARAMS, RESULT> List<RESULT> execute(Statement<PARAMS, RESULT> statement, PARAMS params) {
        return tracingQueryExecutor.execute(statement, params);
    }

    @Override
    public <IN> void pendingExecute(Statement<IN, ?> statement, IN value) {
        tracingQueryExecutor.pendingExecute(statement, value);
    }

    @Override
    public <IN, OUT> Stream<OUT> readTable(ReadTableMapper<IN, OUT> mapper, ReadTableParams<IN> params) {
        return tracingQueryExecutor.readTable(mapper, params);
    }

}
