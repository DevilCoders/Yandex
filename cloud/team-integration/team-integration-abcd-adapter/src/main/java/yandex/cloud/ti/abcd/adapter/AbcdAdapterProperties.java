package yandex.cloud.ti.abcd.adapter;

import java.util.List;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.ti.abcd.provider.AbcdProviderProperties;

public record AbcdAdapterProperties(
        @NotNull List<AbcdProviderProperties> providers
) {

    public AbcdAdapterProperties(
            @Nullable List<AbcdProviderProperties> providers
    ) {
        this.providers = providers == null ? List.of() : List.copyOf(providers);
    }

}
