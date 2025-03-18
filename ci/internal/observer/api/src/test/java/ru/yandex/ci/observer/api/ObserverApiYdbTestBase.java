package ru.yandex.ci.observer.api;

import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.client.arcanum.ArcanumClient;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.observer.api.spring.ObserverApiConfig;
import ru.yandex.ci.observer.api.spring.ObserverApiPropertiesTestConfig;
import ru.yandex.ci.observer.core.ObserverYdbTestBase;

@ContextConfiguration(classes = {
        ObserverApiConfig.class,
        ObserverApiPropertiesTestConfig.class,
        ArcClientTestConfig.class,
})
public class ObserverApiYdbTestBase extends ObserverYdbTestBase {

    @MockBean
    protected ArcanumClient arcanumClient;
}
