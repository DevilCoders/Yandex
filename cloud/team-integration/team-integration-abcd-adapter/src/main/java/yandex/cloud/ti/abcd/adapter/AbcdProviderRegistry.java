package yandex.cloud.ti.abcd.adapter;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.ti.abcd.provider.AbcdProvider;

public class AbcdProviderRegistry {
    // todo move near AbcdAdapter api

    private final @NotNull Map<String, AbcdProvider> providers;


    public AbcdProviderRegistry(@NotNull List<AbcdProvider> providers) {
        this.providers = providers.stream()
                .collect(Collectors.toMap(
                        AbcdProvider::getId,
                        it -> it
                ));
    }


    public @NotNull AbcdProvider getProvider(@NotNull String id) {
        AbcdProvider provider = providers.get(id);
        if (provider == null) {
            throw new AbcdProviderNotFoundException(String.format("abcd provider %s not found", id));
        }
        return provider;
    }

}
