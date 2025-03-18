package ru.yandex.ci.engine.notification.xiva;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.launch.Launch;

public interface XivaNotifier extends AutoCloseable {
    void onLaunchStateChanged(@Nonnull Launch updatedLaunch, @Nullable Launch oldLaunch);

    Common.XivaSubscription toXivaSubscription(@Nonnull XivaBaseEvent event);
}
