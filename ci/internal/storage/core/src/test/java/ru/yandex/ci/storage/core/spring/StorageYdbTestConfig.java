package ru.yandex.ci.storage.core.spring;

import java.util.concurrent.ExecutorService;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import yandex.cloud.repository.kikimr.KikimrConfig;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.common.ydb.spring.YdbTestConfig;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.CiStorageDbImpl;
import ru.yandex.ci.storage.core.db.CiStorageEntities;
import ru.yandex.ci.storage.core.db.CiStorageRepository;
import ru.yandex.ci.storage.core.ydb.HintRegistry;
import ru.yandex.ci.ydb.YdbCleanupProxy;

@Configuration
@Import({
        YdbTestConfig.class
})
public class StorageYdbTestConfig {

    @Bean(destroyMethod = "shutdown")
    public CiStorageRepository storageRepository(KikimrConfig kikimrConfig) {
        CiStorageRepository repository = new CiStorageRepository(kikimrConfig);

        resetNumberOfPartitions();

        YdbUtils.initDb(repository, CiStorageEntities.getAllEntities());
        return repository;
    }

    @Bean
    public CiStorageDb ciStorageDb(CiStorageRepository storageRepository, ExecutorService testExecutor) {
        return YdbCleanupProxy.withCleanupProxy(new CiStorageDbImpl(storageRepository), testExecutor);
    }

    public static void resetNumberOfPartitions() {
        // Initialize all hints
        CiStorageEntities.getAllEntities().forEach(c -> {
            try {
                Class.forName(c.getName());
            } catch (ClassNotFoundException e) {
                throw new RuntimeException(e);
            }
        });
        HintRegistry.getInstance().getHints().forEach(builder -> builder.apply(1));
    }
}
