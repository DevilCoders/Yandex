package ru.yandex.ci.engine.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.xiva.XivaClient;
import ru.yandex.ci.engine.notification.xiva.XivaNotifier;
import ru.yandex.ci.engine.notification.xiva.XivaNotifierDeferredImpl;
import ru.yandex.ci.engine.notification.xiva.XivaNotifierImpl;
import ru.yandex.ci.engine.spring.clients.XivaClientConfig;

@Configuration
@Import(XivaClientConfig.class)
public class XivaNotificationConfig {

    @Bean
    public XivaNotifier xivaNotifier(XivaClient xivaClient, MeterRegistry meterRegistry) {
        return new XivaNotifierDeferredImpl(
                new XivaNotifierImpl(xivaClient, meterRegistry)
        );
    }

}
