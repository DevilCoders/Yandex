package ru.yandex.ci.flow.spring.ydb;

import java.util.Collection;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import yandex.cloud.repository.kikimr.KikimrConfig;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.temporal.ydb.TemporalEntities;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.common.ydb.spring.YdbConfig;
import ru.yandex.ci.core.db.CiMainEntities;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.db.CiDbImpl;
import ru.yandex.ci.flow.db.CiDbRepository;
import ru.yandex.ci.flow.db.CiFlowEntities;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageEntities;

@Configuration
@Import(YdbConfig.class)
public class YdbCiConfig {

    @Bean(destroyMethod = "shutdown")
    public CiDbRepository ciRepository(KikimrConfig ciKikimrConfig) {
        CiDbRepository repository = new CiDbRepository(ciKikimrConfig);

        var allEntities = Stream.of(
                CiMainEntities.ALL,
                CiFlowEntities.ALL,
                BazingaStorageEntities.ALL,
                TemporalEntities.ALL
        )
                .flatMap(Collection::stream)
                .collect(Collectors.toList());
        YdbUtils.initDb(repository, allEntities);
        return repository;
    }

    @Bean
    @Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
    public CiDb db(CiDbRepository ciDbRepository) {
        return new CiDbImpl(ciDbRepository);
    }

}
