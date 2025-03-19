package yandex.cloud.ti.abcd.adapter;

import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;

public class AbcdAdapterPropertiesConfiguration extends Configuration {

    private final @NotNull Supplier<AbcdAdapterProperties> abcdAdapterPropertiesSupplier;


    public AbcdAdapterPropertiesConfiguration(
            @NotNull Supplier<AbcdAdapterProperties> abcdAdapterPropertiesSupplier
    ) {
        this.abcdAdapterPropertiesSupplier = abcdAdapterPropertiesSupplier;
    }


    @Override
    protected void configure() {
        put(AbcdAdapterProperties.class, abcdAdapterPropertiesSupplier);
    }

}
