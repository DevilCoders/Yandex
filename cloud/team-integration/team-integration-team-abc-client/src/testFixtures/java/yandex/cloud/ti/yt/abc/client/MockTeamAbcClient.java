package yandex.cloud.ti.yt.abc.client;

import java.util.ArrayList;
import java.util.List;

import org.jetbrains.annotations.NotNull;

public class MockTeamAbcClient implements TeamAbcClient {

    private final @NotNull List<TeamAbcService> services = new ArrayList<>();


    @Override
    public @NotNull TeamAbcService getAbcServiceById(long id) {
        return services.stream()
                .filter(it -> it.id() == id)
                .findFirst()
                .orElseThrow(() -> AbcServiceNotFoundException.forAbcId(id));
    }

    @Override
    public @NotNull TeamAbcService getAbcServiceBySlug(@NotNull String slug) {
        return services.stream()
                .filter(it -> it.slug().equals(slug))
                .findFirst()
                .orElseThrow(() -> AbcServiceNotFoundException.forAbcSlug(slug));
    }


    public void addService(@NotNull TeamAbcService service) {
        services.add(service);
    }

    public void clearServices() {
        services.clear();
    }

}
