package ru.yandex.ci.storage.tests;

import java.sql.SQLException;
import java.time.Instant;
import java.util.HashSet;
import java.util.List;

import com.google.common.collect.Lists;
import com.google.common.primitives.UnsignedLongs;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;
import org.springframework.test.context.ActiveProfiles;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.clickhouse.AutocheckClickhouse;
import ru.yandex.ci.storage.core.clickhouse.ClickHouseExportServiceImpl;
import ru.yandex.ci.storage.core.clickhouse.SpClickhouseConfig;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricEntity;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricTable;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.test_metrics.MetricsClickhouseConfig;
import ru.yandex.ci.storage.core.test_metrics.TestMetricsServiceImpl;
import ru.yandex.ci.storage.core.ydb.SequenceService;

import static org.assertj.core.api.Assertions.assertThat;

@SuppressWarnings("unused")
@ExtendWith(SpringExtension.class)
@ContextConfiguration(classes = {
        ClickHouseTest.Config.class
})
@ActiveProfiles(profiles = CiProfile.STABLE_PROFILE)
@Slf4j
@Disabled
public class ClickHouseTest {
    @Autowired
    private AutocheckClickhouse clickhouse;

    @Autowired
    private TestMetricTable testMetricTable;

    @Autowired
    private CiStorageDb db;

    @Test
    @Disabled
    public void deleteOldTests() {
        var tests = db.scan().withMaxSize(Integer.MAX_VALUE).run(
                () -> db.tests().find(
                        YqlPredicate.where("id.branch").eq("trunk")
                                .and("revisionNumber").lt(9367817L)
                )
        ).stream().map(TestStatusEntity::getId).toList();

        for (var batch : Lists.partition(tests, 1000)) {
            db.currentOrTx(() -> db.tests().delete(new HashSet<>(batch)));
        }
    }

    @Test
    @Disabled
    public void test() throws SQLException, InterruptedException {
        var service = new ClickHouseExportServiceImpl(db, clickhouse, new SequenceService(db), 500);

        service.process(new CheckIterationEntity.Id(new CheckEntity.Id(42800000001873L), 1, 1));
    }

    @Test
    @Disabled
    public void metricsInsertTest() {
        var service = new TestMetricsServiceImpl(testMetricTable, db);

        service.insert(
                List.of(
                        new TestMetricEntity(
                                Trunk.name(),
                                "path",
                                "testName",
                                "subTestName",
                                "toolchain",
                                "metricName",
                                Instant.now(),
                                1L,
                                Common.ResultType.RT_TEST_MEDIUM,
                                1.0,
                                Common.TestStatus.TS_OK,
                                0L,
                                333L,
                                444L
                        )
                )
        );
    }

    @Test
    @Disabled
    public void metricsReadTest() {
        var service = new TestMetricsServiceImpl(testMetricTable, db);

        var id = UnsignedLongs.parseUnsignedLong("12755404088032698000");
        var metrics = service.getTestMetrics(new TestStatusEntity.Id(id, id, Trunk.name(), TestEntity.ALL_TOOLCHAINS));
        assertThat(metrics).hasSize(11);
    }

    @Configuration
    @PropertySource("classpath:ci-storage-core/ci-storage.properties")
    @PropertySource("classpath:ci-storage-core/ci-storage-stable.properties")
    @PropertySource(value = "file:${user.home}/.ci/ci-local.properties", ignoreResourceNotFound = true)
    @Import(value = {SpClickhouseConfig.class, MetricsClickhouseConfig.class})
    public static class Config {
    }
}
