package ru.yandex.ci.observer.reader.message;

import ru.yandex.ci.core.logbroker.LogbrokerConfiguration;
import ru.yandex.ci.logbroker.LogbrokerStreamConsumer;
import ru.yandex.ci.observer.core.cache.ObserverSettingsCache;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;

public class ObserverStreamConsumer extends LogbrokerStreamConsumer {
    private final ObserverSettingsCache settingsCache;

    public ObserverStreamConsumer(
            LogbrokerConfiguration configuration,
            StreamListener listener,
            ObserverSettingsCache settingsCache,
            boolean onlyNewData,
            int maxUncommittedReads
    ) throws InterruptedException {
        super(configuration, listener, onlyNewData, maxUncommittedReads);

        this.settingsCache = settingsCache;
    }

    @Override
    protected boolean isReadStopped() {
        return this.settingsCache.get().isStopAllRead();
    }
}
