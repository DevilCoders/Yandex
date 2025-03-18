package ru.yandex.ci.observer.reader.spring;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.spring.ObserverCacheConfig;
import ru.yandex.ci.observer.reader.check.ObserverCheckService;
import ru.yandex.ci.observer.reader.check.ObserverInternalCheckService;
import ru.yandex.ci.observer.reader.message.internal.InternalStreamStatistics;
import ru.yandex.ci.storage.core.message.main.MainStreamStatistics;

@Configuration
@Import({
        CommonConfig.class,
        ObserverCacheConfig.class
})
public class ObserverReaderCoreConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Autowired
    private CollectorRegistry collectorRegistry;

    @Bean
    public ObserverCheckService observerCheckService(
            CiObserverDb db
    ) {
        return new ObserverCheckService(db);
    }

    @Bean
    public ObserverInternalCheckService observerInternalCheckService(
            CiObserverDb ciObserverDb,
            ObserverCache observerCache
    ) {
        return new ObserverInternalCheckService(observerCache, ciObserverDb);
    }

    @Bean
    public MainStreamStatistics observerMainStreamStatistics() {
        return new MainStreamStatistics(meterRegistry, collectorRegistry);
    }

    @Bean
    public InternalStreamStatistics observerInternalStreamStatistics() {
        return new InternalStreamStatistics(meterRegistry, collectorRegistry);
    }
}
