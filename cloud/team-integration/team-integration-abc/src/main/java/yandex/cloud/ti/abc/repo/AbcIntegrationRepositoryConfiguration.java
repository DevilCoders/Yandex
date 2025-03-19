package yandex.cloud.ti.abc.repo;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;

public abstract class AbcIntegrationRepositoryConfiguration extends Configuration {

    @Override
    protected void configure() {
        put(AbcIntegrationRepository.class, this::abcIntegrationRepository);
    }

    protected abstract @NotNull AbcIntegrationRepository abcIntegrationRepository();

}
