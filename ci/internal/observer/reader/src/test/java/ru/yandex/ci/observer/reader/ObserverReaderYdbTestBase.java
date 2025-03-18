package ru.yandex.ci.observer.reader;

import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.observer.core.ObserverYdbTestBase;
import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.reader.spring.ObserverLoadedEntitiesConfig;
import ru.yandex.ci.observer.reader.spring.ObserverReaderPropertiesTestConfig;

@ContextConfiguration(classes = {
        ObserverLoadedEntitiesConfig.class,
        ObserverReaderPropertiesTestConfig.class
})
public class ObserverReaderYdbTestBase extends ObserverYdbTestBase {

    @Autowired
    protected ObserverCache cache;

    @BeforeEach
    public void clearCache() {
        this.cache.modify(ObserverCache.Modifiable::invalidateAll);
    }
}
