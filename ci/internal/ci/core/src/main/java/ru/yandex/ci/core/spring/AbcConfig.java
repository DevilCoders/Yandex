package ru.yandex.ci.core.spring;

import java.time.Clock;
import java.time.Duration;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.abc.AbcClient;
import ru.yandex.ci.client.abc.AbcTableClient;
import ru.yandex.ci.core.abc.AbcFavoriteProjectsService;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.abc.AbcServiceImpl;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.spring.clients.AbcClientConfig;

@Configuration
@Import({
        AbcClientConfig.class
})
public class AbcConfig {

    @Bean
    public AbcService abcService(
            Clock clock,
            AbcClient abcClient,
            CiMainDb db,
            MeterRegistry meterRegistry
    ) {
        return new AbcServiceImpl(clock, abcClient, Duration.ofMinutes(10), db, meterRegistry);
    }

    @Bean
    public AbcFavoriteProjectsService abcFavoriteProjectsService(
            AbcTableClient abcTableClient,
            CiMainDb db
    ) {
        return new AbcFavoriteProjectsService(abcTableClient, db);
    }
}
