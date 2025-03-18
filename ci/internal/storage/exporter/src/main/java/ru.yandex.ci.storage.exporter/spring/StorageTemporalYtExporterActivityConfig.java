package ru.yandex.ci.storage.exporter.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;
import ru.yandex.ci.storage.core.yt.IterationToYtExporter;
import ru.yandex.ci.storage.core.yt.YtClientFactory;
import ru.yandex.ci.storage.core.yt.impl.IterationToYtExporterImpl;
import ru.yandex.ci.storage.core.yt.impl.YtClientFactoryImpl;
import ru.yandex.ci.storage.core.yt.impl.YtExportActivityImpl;
import ru.yandex.yt.ytclient.rpc.RpcCredentials;

@Configuration
@Import(StorageYdbConfig.class)
public class StorageTemporalYtExporterActivityConfig {

    @Bean
    @Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
    public YtClientFactory ytClientFactory(
            @Value("${storage.ytClientFactory.username}") String username,
            @Value("${storage.ytClientFactory.token}") String token
    ) {
        return new YtClientFactoryImpl(new RpcCredentials(username, token));
    }

    @Bean
    public IterationToYtExporter iterationToYtExporter(
            CiStorageDb db,
            YtClientFactory ytClientFactory,
            @Value("${storage.iterationToYtExporter.rootPath}") String rootPath,
            @Value("${storage.iterationToYtExporter.batchSize}") int batchSize
    ) {
        return new IterationToYtExporterImpl(db, rootPath, ytClientFactory, batchSize);
    }

    @Bean
    public YtExportActivityImpl ytExportActivityImpl(IterationToYtExporter exporter) {
        return new YtExportActivityImpl(exporter);
    }

}
