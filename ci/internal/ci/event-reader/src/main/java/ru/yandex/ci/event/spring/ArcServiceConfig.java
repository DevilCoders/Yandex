package ru.yandex.ci.event.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.event.arc.ArcEventService;
import ru.yandex.ci.event.arc.ArcReadProcessor;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import(BazingaCoreConfig.class)
public class ArcServiceConfig {

    @Bean
    public ArcEventService arcEventService(BazingaTaskManager taskManager) {
        return new ArcEventService(taskManager);
    }

    @Bean
    public ArcReadProcessor arcReadProcessor(ArcEventService arcEventService) {
        return new ArcReadProcessor(arcEventService);
    }
}
