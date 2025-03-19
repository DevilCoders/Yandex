package yandex.cloud.team.integration.abc;

import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;

public class AbcIntegrationPropertiesConfiguration extends Configuration {

    private final @NotNull Supplier<AbcIntegrationProperties> abcIntegrationPropertiesSupplier;


    public AbcIntegrationPropertiesConfiguration(
            @NotNull Supplier<AbcIntegrationProperties> abcIntegrationPropertiesSupplier
    ) {
        this.abcIntegrationPropertiesSupplier = abcIntegrationPropertiesSupplier;
    }


    @Override
    public void configure() {
        put(AbcIntegrationProperties.class, abcIntegrationPropertiesSupplier);
    }

}
