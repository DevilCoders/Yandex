package yandex.cloud.ti.yt.abcd.client;

import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;

public class TeamAbcdClientPropertiesConfiguration extends Configuration {

    private final @NotNull Supplier<TeamAbcdClientProperties> abcdPropertiesSupplier;


    public TeamAbcdClientPropertiesConfiguration(
            @NotNull Supplier<TeamAbcdClientProperties> abcdPropertiesSupplier
    ) {
        this.abcdPropertiesSupplier = abcdPropertiesSupplier;
    }


    @Override
    public void configure() {
        put(TeamAbcdClientProperties.class, abcdPropertiesSupplier);
    }

}
