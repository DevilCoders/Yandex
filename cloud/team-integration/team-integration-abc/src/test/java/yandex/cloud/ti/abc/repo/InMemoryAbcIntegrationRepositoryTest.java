package yandex.cloud.ti.abc.repo;

import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public class InMemoryAbcIntegrationRepositoryTest extends AbcIntegrationRepositoryContractTest {

    @Override
    protected @NotNull AbcIntegrationRepository getAbcIntegrationRepository() {
        return new InMemoryAbcIntegrationRepository();
    }

    @Override
    protected <T> @Nullable T tx(@NotNull Supplier<T> supplier) {
        return supplier.get();
    }

}
