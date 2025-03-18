package ru.yandex.ci.observer.core;

import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.observer.core.spring.ObserverCorePropertiesTestConfig;
import ru.yandex.ci.util.Retryable;

@ContextConfiguration(classes = {
        CommonTestConfig.class,
        ObserverCorePropertiesTestConfig.class
})
public class ObserverTestBase extends CommonTestBase {

    public ObserverTestBase() {
        Retryable.disable();
    }
}
