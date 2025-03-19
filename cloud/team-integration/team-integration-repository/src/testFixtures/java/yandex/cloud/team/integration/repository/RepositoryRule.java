package yandex.cloud.team.integration.repository;

import java.util.List;
import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;
import yandex.cloud.iam.repository.tracing.QueryTraceContext;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.Repository;
import yandex.cloud.repository.db.TxManager;
import yandex.cloud.repository.db.TxManagerImpl;

public class RepositoryRule implements TestRule {

    @SuppressWarnings("rawtypes")
    private final @NotNull List<Class<? extends Entity>> entities;

    private Repository repository;
    private TxManager txManager;


    public RepositoryRule(
            @SuppressWarnings("rawtypes") @NotNull List<Class<? extends Entity>> entities
    ) {
        this.entities = List.copyOf(entities);
    }


    @Override
    public Statement apply(Statement base, Description description) {
        return new RepositoryStatement(base);
    }

    public <T> @Nullable T tx(@NotNull Supplier<T> supplier) {
        return txManager.tx(supplier);
    }

    @SuppressWarnings("unchecked")
    private void beforeEach() {
        repository = new TeamIntegrationTracingInMemoryRepository();
        repository.createTablespace();
        entities.forEach(entityClass -> repository.schema(entityClass).create());

        txManager = new TxManagerImpl(repository);
    }

    private void afterEach() {
        QueryTraceContext.withoutQueryTrace(() -> {
            txManager.tx(() -> {
                repository.dropDb();
            });
        });
    }

    private class RepositoryStatement extends Statement {

        private final Statement next;

        RepositoryStatement(Statement next) {
            this.next = next;
        }

        @Override
        public void evaluate() throws Throwable {
            beforeEach();
            try {
                next.evaluate();
            } finally {
                afterEach();
            }
        }

    }

}
