package yandex.cloud.ti.abc.repo;

import java.util.List;

import lombok.NonNull;
import yandex.cloud.iam.config.RepositoryConfig;
import yandex.cloud.team.integration.repository.TeamIntegrationInMemoryRepositoryConfiguration;

public class InMemoryAbcIntegrationRepositoryConfiguration extends AbcIntegrationRepositoryConfiguration {

    @Override
    protected void configure() {
        super.configure();
        put(InMemoryAbcIntegrationRepository.class, this::memoryAbcIntegrationRepository);
        // todo RepositoryConfig used in AbstractRepositoryConfiguration
        //  forTesting should be moved into TeamIntegrationInMemoryRepositoryConfiguration
        //  AbstractRepositoryConfiguration is required for in-memory OperationDb and TaskDb support
        put(RepositoryConfig.class, RepositoryConfig::forTesting);
        merge(new TeamIntegrationInMemoryRepositoryConfiguration(List.of()));
    }

    protected @NonNull InMemoryAbcIntegrationRepository memoryAbcIntegrationRepository() {
        return new InMemoryAbcIntegrationRepository();
    }

    @Override
    protected @NonNull AbcIntegrationRepository abcIntegrationRepository() {
        return get(InMemoryAbcIntegrationRepository.class);
    }

}
