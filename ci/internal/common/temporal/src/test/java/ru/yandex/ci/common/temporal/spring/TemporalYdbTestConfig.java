package ru.yandex.ci.common.temporal.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import yandex.cloud.repository.kikimr.KikimrConfig;
import yandex.cloud.repository.kikimr.yql.YqlPrimitiveType;
import ru.yandex.ci.common.temporal.ydb.TemporalDb;
import ru.yandex.ci.common.temporal.ydb.TemporalDbImpl;
import ru.yandex.ci.common.temporal.ydb.TemporalEntities;
import ru.yandex.ci.common.temporal.ydb.TemporalRepository;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.common.ydb.spring.YdbTestConfig;

@Configuration
@Import(YdbTestConfig.class)
public class TemporalYdbTestConfig {

    static {
        YqlPrimitiveType.changeStringDefaultTypeToUtf8();
    }

    @Bean
    public TemporalRepository temporalRepository(KikimrConfig storageKikimrConfig) {
        TemporalRepository repository = new TemporalRepository(storageKikimrConfig);
        YdbUtils.initDb(repository, TemporalEntities.ALL);
        return repository;
    }

    @Bean
    public TemporalDb temporalDb(TemporalRepository repository) {
        return new TemporalDbImpl(repository);
    }

}
