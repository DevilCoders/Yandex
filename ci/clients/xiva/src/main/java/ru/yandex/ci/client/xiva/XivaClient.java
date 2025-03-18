package ru.yandex.ci.client.xiva;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

public interface XivaClient {
    /**
     * @param shortTitle short title, about what happend, useful for statistics
     */
    void send(@Nonnull String topic, @Nonnull SendRequest request, @Nullable String shortTitle);

    String getService();
}
