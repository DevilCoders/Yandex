package ru.yandex.ci.storage.core.test_metrics;

import java.time.Duration;
import java.util.concurrent.TimeUnit;

import com.google.common.base.Preconditions;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricTable;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;
import ru.yandex.clickhouse.settings.ClickHouseProperties;

@Configuration
@Profile(value = CiProfile.NOT_UNIT_TEST_PROFILE)
@Import({StorageYdbConfig.class})
public class MetricsClickhouseConfig {

    @Bean
    public ClickHouseProperties metricsClickhouseProperties(
            @Value("${storage.metricsClickhouseProperties.user}") String user,
            @Value("${storage.metricsClickhouseProperties.password}") String password
    ) {
        var properties = new ClickHouseProperties();
        properties.setUser(user);
        properties.setPassword(password);
        properties.setSsl(true);
        properties.setSslMode("none");
        return properties;
    }

    @Bean
    public BalancedClickhouseDataSource metricsClickhouse(
            @Value("${storage.metricsClickhouse.url}") String url,
            @Value("${storage.metricsClickhouse.actualization}") Duration actualization,
            ClickHouseProperties metricsClickhouseProperties
    ) {
        var dataSource = new BalancedClickhouseDataSource(url, metricsClickhouseProperties);
        dataSource.scheduleActualization((int) actualization.toSeconds(), TimeUnit.SECONDS);
        dataSource.actualize();
        Preconditions.checkState(dataSource.getEnabledClickHouseUrls().size() > 0, "No enabled clickhouse urls");
        return dataSource;
    }

    @Bean
    public TestMetricTable testMetricTable(
            BalancedClickhouseDataSource metricsClickhouse,
            @Value("${storage.testMetricTable.database}") String database
    ) {
        return new TestMetricTable(database, metricsClickhouse);
    }

    @Bean
    public TestMetricsService testMetricsService(TestMetricTable metricTable, CiStorageDb db) {
        return new TestMetricsServiceImpl(metricTable, db);
    }
}
