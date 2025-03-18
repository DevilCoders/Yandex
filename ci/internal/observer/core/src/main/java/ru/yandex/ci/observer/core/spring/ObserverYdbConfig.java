package ru.yandex.ci.observer.core.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import yandex.cloud.repository.kikimr.KikimrConfig;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.CiObserverDbImpl;
import ru.yandex.ci.observer.core.db.CiObserverEntities;
import ru.yandex.ci.observer.core.db.CiObserverRepository;

@Configuration
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class ObserverYdbConfig {
    @Bean
    public KikimrConfig observerYdbConfig(
            @Value("${observer.observerYdbConfig.ydbEndpoint}") String ydbEndpoint,
            @Value("${observer.observerYdbConfig.ydbDatabase}") String ydbDatabase,
            @Value("${observer.observerYdbConfig.ydbToken}") String ydbToken,
            @Value("${observer.observerYdbConfig.maxPoolSize}") int maxPoolSize
    ) {
        return YdbUtils.createConfig(ydbEndpoint, ydbDatabase, ydbToken, null)
                .withSessionPoolMax(maxPoolSize);
    }

    @Bean(destroyMethod = "shutdown")
    public CiObserverRepository observerRepository(KikimrConfig observerYdbConfig) {
        return YdbUtils.initDb(new CiObserverRepository(observerYdbConfig), CiObserverEntities.getEntities());
    }

    @Bean
    public CiObserverDb ciObserverDb(CiObserverRepository observerRepository) {
        return new CiObserverDbImpl(observerRepository);
    }
}
