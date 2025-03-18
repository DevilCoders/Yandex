package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;

@Configuration
@Import(YdbCiConfig.class)
public class DiscoveryProgressConfig {

    @Bean
    public DiscoveryProgressService discoveryProgressService(CiMainDb db) {
        return new DiscoveryProgressService(db);
    }
}
