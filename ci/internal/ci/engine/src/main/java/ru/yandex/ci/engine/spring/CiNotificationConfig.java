package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.engine.event.EventPublisher;
import ru.yandex.ci.flow.spring.temporal.CiTemporalServiceConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        BazingaCoreConfig.class,
        CiTemporalServiceConfig.class
})
public class CiNotificationConfig {

    @Bean
    public EventPublisher launchEventPublisher(
            BazingaTaskManager bazingaTaskManager,
            TemporalService temporalService
    ) {
        return new EventPublisher(bazingaTaskManager, temporalService);
    }

}
