package ru.yandex.ci.storage.reader;

import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsProducer;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.spring.StorageReaderTestConfig;

@ContextConfiguration(classes = StorageReaderTestConfig.class)
public class StorageReaderYdbTestBase extends StorageYdbTestBase {

    @Autowired
    protected ReaderCache readerCache;

    @Autowired
    protected TimeTraceService timeTraceService;

    @Autowired
    protected BadgeEventsProducer badgeEventsProducer;

    @BeforeEach
    public void clearCache() {
        this.readerCache.modify(ReaderCache.Modifiable::invalidateAll);
    }
}
