package yandex.cloud.ti.yt.abc.client;

import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.di.Configuration;

public class TeamAbcClientPropertiesConfiguration extends Configuration {


    private final @NotNull Supplier<TeamAbcClientProperties> supplier;


    public TeamAbcClientPropertiesConfiguration(
            @NotNull Supplier<TeamAbcClientProperties> supplier
    ) {
        this.supplier = supplier;
    }


    @Override
    public void configure() {
        put(TeamAbcClientProperties.class, supplier);
    }

}
