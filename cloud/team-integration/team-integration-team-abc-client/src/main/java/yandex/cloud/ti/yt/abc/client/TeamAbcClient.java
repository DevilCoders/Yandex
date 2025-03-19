package yandex.cloud.ti.yt.abc.client;

import org.jetbrains.annotations.NotNull;

public interface TeamAbcClient {

    @NotNull TeamAbcService getAbcServiceById(
            long id
    );

    @NotNull TeamAbcService getAbcServiceBySlug(
            @NotNull String slug
    );

}
