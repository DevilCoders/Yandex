package ru.yandex.ci.storage.shard.message;

import ru.yandex.ci.core.logbroker.LogbrokerConfiguration;
import ru.yandex.ci.logbroker.LogbrokerStreamConsumer;
import ru.yandex.ci.storage.core.cache.SettingsCache;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;

public class ShardInStreamConsumer extends LogbrokerStreamConsumer {
    private final SettingsCache settingsCache;

    public ShardInStreamConsumer(
            LogbrokerConfiguration configuration,
            StreamListener listener,
            SettingsCache settingsCache,
            boolean onlyNewData,
            int maxUncommittedReads
    ) throws InterruptedException {
        super(configuration, listener, onlyNewData, maxUncommittedReads);

        this.settingsCache = settingsCache;
    }

    @Override
    protected boolean isReadStopped() {
        return this.settingsCache.get().getShard().isStopAllRead();
    }
}
