package ru.yandex.ci.storage.core;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.storage.core.spring.StorageCoreTestConfig;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.ci.util.Retryable;

@ContextConfiguration(classes = {
        StorageCoreTestConfig.class
})
public class StorageTestBase extends CommonTestBase {

    @Autowired
    protected MeterRegistry meterRegistry;

    @Autowired
    protected CollectorRegistry collectorRegistry;

    @Autowired
    protected OverridableClock clock;

    public StorageTestBase() {
        Retryable.disable();
    }
}

