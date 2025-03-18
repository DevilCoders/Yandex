package ru.yandex.ci.event.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.event.arcanum.ArcanumEventService;
import ru.yandex.ci.event.arcanum.ArcanumReadProcessor;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        YdbCiConfig.class,
        BazingaCoreConfig.class
})
public class ArcanumServiceConfig {
    @Bean
    public ArcanumEventService arcanumEventService(
            BazingaTaskManager bazingaTaskManager,
            @Value("${ci.arcanumEventService.authorFilter}") String authorFilter,
            CiMainDb db
    ) {
        return new ArcanumEventService(bazingaTaskManager, authorFilter, db);
    }

    @Bean
    public ArcanumReadProcessor arcanumReadProcessor(ArcanumEventService arcanumEventService) {
        return new ArcanumReadProcessor(arcanumEventService);
    }
}
