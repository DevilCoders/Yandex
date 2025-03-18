package ru.yandex.ci.engine.notification.xiva;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;

import yandex.cloud.repository.db.Tx;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.launch.Launch;

@RequiredArgsConstructor
public class XivaNotifierDeferredImpl implements XivaNotifier {

    @Nonnull
    private final XivaNotifier xivaNotifier;

    @Override
    public void onLaunchStateChanged(@Nonnull Launch updatedLaunch, @Nullable Launch oldLaunch) {
        defer(() -> xivaNotifier.onLaunchStateChanged(updatedLaunch, oldLaunch));
    }

    @Override
    public Common.XivaSubscription toXivaSubscription(@Nonnull XivaBaseEvent event) {
        return xivaNotifier.toXivaSubscription(event);
    }

    @Override
    public void close() throws Exception {
        xivaNotifier.close();
    }

    private static void defer(Runnable runnable) {
        if (Tx.Current.exists()) {
            Tx.Current.get().defer(runnable);
        } else {
            runnable.run();
        }
    }

}
