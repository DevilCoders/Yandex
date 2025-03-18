package ru.yandex.ci.storage.core.spring;

import java.time.Duration;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import yandex.cloud.repository.kikimr.KikimrConfig;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.CiStorageDbImpl;
import ru.yandex.ci.storage.core.db.CiStorageEntities;
import ru.yandex.ci.storage.core.db.CiStorageRepository;

@Configuration
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class StorageYdbConfig {
    @Bean
    public KikimrConfig storageKikimrConfig(
            @Value("${storage.storageKikimrConfig.ydbEndpoint}") String ydbEndpoint,
            @Value("${storage.storageKikimrConfig.ydbDatabase}") String ydbDatabase,
            @Value("${storage.storageKikimrConfig.ydbToken}") String ydbToken,
            @Value("${storage.storageKikimrConfig.maxPoolSize}") int maxPoolSize
    ) {
        var config = YdbUtils.createConfig(ydbEndpoint, ydbDatabase, ydbToken, null);
        return config
                .withSessionPoolMax(maxPoolSize)
                .withSessionCreationTimeout(Duration.ofSeconds(10));
    }

    @Bean(destroyMethod = "shutdown")
    public CiStorageRepository storageRepository(KikimrConfig storageKikimrConfig) {
        return YdbUtils.initDb(new CiStorageRepository(storageKikimrConfig), CiStorageEntities.getAllEntities());
    }

    @Bean
    public CiStorageDb ciStorageDb(CiStorageRepository repository) {
        return new CiStorageDbImpl(repository);
    }


}
