package yandex.cloud.ti.rm.client;

import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;

public class ResourceManagerClientPropertiesConfiguration extends Configuration {

    private final @NotNull Supplier<ResourceManagerClientProperties> resourceManagerClientPropertiesSupplier;


    public ResourceManagerClientPropertiesConfiguration(
            @NotNull Supplier<ResourceManagerClientProperties> resourceManagerClientPropertiesSupplier
    ) {
        this.resourceManagerClientPropertiesSupplier = resourceManagerClientPropertiesSupplier;
    }


    @Override
    public void configure() {
        put(ResourceManagerClientProperties.class, resourceManagerClientPropertiesSupplier);
    }

}
