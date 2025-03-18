package ru.yandex.ci.observer.core.spring;

import java.util.concurrent.ExecutorService;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import yandex.cloud.repository.kikimr.KikimrConfig;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.common.ydb.spring.YdbTestConfig;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.CiObserverDbImpl;
import ru.yandex.ci.observer.core.db.CiObserverEntities;
import ru.yandex.ci.observer.core.db.CiObserverRepository;
import ru.yandex.ci.storage.core.spring.StorageYdbTestConfig;
import ru.yandex.ci.ydb.YdbCleanupProxy;

@Configuration
@Import(YdbTestConfig.class)
public class ObserverYdbTestConfig {
    @Bean(destroyMethod = "shutdown")
    public CiObserverRepository observerRepository(KikimrConfig kikimrConfig) {
        CiObserverRepository repository = new CiObserverRepository(kikimrConfig);

        StorageYdbTestConfig.resetNumberOfPartitions();

        YdbUtils.initDb(repository, CiObserverEntities.getEntities());
        return repository;
    }

    @Bean
    public CiObserverDb ciObserverDb(CiObserverRepository observerRepository, ExecutorService testExecutor) {
        return YdbCleanupProxy.withCleanupProxy(new CiObserverDbImpl(observerRepository), testExecutor);
    }
}
