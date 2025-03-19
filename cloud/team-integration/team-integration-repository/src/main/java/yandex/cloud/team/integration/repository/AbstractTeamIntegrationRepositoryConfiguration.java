package yandex.cloud.team.integration.repository;

import java.time.Clock;
import java.util.Collection;
import java.util.List;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.config.AbstractRepositoryConfiguration;
import yandex.cloud.iam.repository.EntityIdGenerator;
import yandex.cloud.iam.repository.EntityIdGeneratorImpl;
import yandex.cloud.iam.repository.TransactionTime;
import yandex.cloud.iam.repository.TransactionTimeImpl;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.id.ClusterConfig;

public abstract class AbstractTeamIntegrationRepositoryConfiguration extends AbstractRepositoryConfiguration {

    @SuppressWarnings("rawtypes")
    private final @NotNull List<Class<? extends Entity>> entities;


    protected AbstractTeamIntegrationRepositoryConfiguration(
            @SuppressWarnings("rawtypes") @NotNull Collection<? extends Collection<Class<? extends Entity>>> entities
    ) {
        this.entities = TeamIntegrationEntitiesHelper.collectEntities(entities);
    }


    @Override
    protected void configure() {
        super.configure();
        put(TransactionTime.class, this::transactionTime);
        put(EntityIdGenerator.class, this::entityIdGenerator);
    }

    @Override
    @SuppressWarnings("rawtypes")
    protected @NotNull List<Class<? extends Entity>> getEntities() {
        return entities;
    }

    private @NotNull TransactionTime transactionTime() {
        return new TransactionTimeImpl(get(Clock.class));
    }

    private @NotNull EntityIdGenerator entityIdGenerator() {
        return new EntityIdGeneratorImpl(get(ClusterConfig.class));
    }

}
