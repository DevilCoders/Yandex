package ru.yandex.ci.storage.tests;

import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.util.concurrent.ExecutionException;

import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import yandex.cloud.repository.kikimr.KikimrConfig;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.CiStorageDbImpl;
import ru.yandex.ci.storage.core.db.CiStorageRepository;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.yt.impl.IterationToYtExporterImpl;
import ru.yandex.ci.storage.core.yt.impl.YtClientFactoryImpl;
import ru.yandex.yt.ytclient.rpc.RpcCredentials;

@ExtendWith(SpringExtension.class)
@ContextConfiguration(classes = YtProductionExportTest.Config.class)
public class YtProductionExportTest {
    @Value("${storage.ytClientFactory.username}")
    String username;

    @Value("${storage.ytClientFactory.token}")
    String token;

    @Autowired
    CiStorageDb db;

    @Value("${storage.yt.path://home/ci/storage/local/}")
    String rootPath;

    @Test
    @Disabled("Works only on local machine, connects to PROD YDB")
    public void export() throws ExecutionException, InterruptedException {
        var exporter = new IterationToYtExporterImpl(
                this.db, rootPath, new YtClientFactoryImpl(new RpcCredentials(username, token)), 10
        );

        exporter.export(
                new CheckIterationEntity.Id(
                        CheckEntity.Id.of(97700000000931L),
                        CheckIteration.IterationType.FULL.getNumber(),
                        1
                )
        );
    }

    @Configuration
    @PropertySource(value = "file:${user.home}/.ci/ci-test.properties", ignoreResourceNotFound = true)
    public static class Config {

        @Bean
        public KikimrConfig kikimrConfig(@Value("${ydb.kikimrConfig.ydbToken}") String ydbToken) {
            return YdbUtils.createConfig(
                    "ydb-ru.yandex.net:2135",
                    "/ru/ci/stable/ci-storage",
                    ydbToken,
                    null
            ).withSessionCreationTimeout(Duration.of(1, ChronoUnit.MINUTES));
        }

        @Bean
        public CiStorageRepository rep(KikimrConfig kikimrConfig) {
            return new CiStorageRepository(kikimrConfig);
        }

        @Bean
        public CiStorageDb db(CiStorageRepository repository) {
            return new CiStorageDbImpl(repository, true);
        }
    }
}
