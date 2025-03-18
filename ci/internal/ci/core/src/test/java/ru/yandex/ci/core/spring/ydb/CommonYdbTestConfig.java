package ru.yandex.ci.core.spring.ydb;

import java.util.concurrent.ExecutorService;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Lazy;

import yandex.cloud.repository.kikimr.KikimrConfig;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.common.ydb.spring.YdbTestConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.CiMainDbImpl;
import ru.yandex.ci.core.db.CiMainEntities;
import ru.yandex.ci.core.db.CiMainRepository;
import ru.yandex.ci.ydb.YdbCleanupProxy;

@Configuration
@Import(YdbTestConfig.class)
public class CommonYdbTestConfig {

    // For testing purposes only
    // Lazy because we have CiDbImpl in flow-engine

    @Lazy
    @Bean(destroyMethod = "shutdown")
    public CiMainRepository ciRepository(KikimrConfig ciKikimrConfig) {
        CiMainRepository repository = new CiMainRepository(ciKikimrConfig);
        YdbUtils.initDb(repository, CiMainEntities.ALL);
        return repository;
    }

    @Lazy
    @Bean
    public CiMainDb db(CiMainRepository ciRepository, ExecutorService testExecutor) {
        return YdbCleanupProxy.withCleanupProxy(new CiMainDbImpl(ciRepository), testExecutor);
    }
}
