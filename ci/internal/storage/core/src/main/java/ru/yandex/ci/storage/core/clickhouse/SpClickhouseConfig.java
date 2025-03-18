package ru.yandex.ci.storage.core.clickhouse;

import java.util.concurrent.TimeUnit;

import com.google.common.base.Preconditions;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.storage.core.db.clickhouse.change_run.ChangeRunTable;
import ru.yandex.ci.storage.core.db.clickhouse.last_run.LastRunTable;
import ru.yandex.ci.storage.core.db.clickhouse.old_test.OldTestTable;
import ru.yandex.ci.storage.core.db.clickhouse.run_link.RunLinkTable;
import ru.yandex.ci.storage.core.db.clickhouse.runs.TestRunTable;
import ru.yandex.ci.storage.core.db.clickhouse.runs.TestRunV2Table;
import ru.yandex.ci.storage.core.db.clickhouse.test_event.TestEventTable;
import ru.yandex.clickhouse.BalancedClickhouseDataSource;
import ru.yandex.clickhouse.settings.ClickHouseProperties;

@Configuration
@Profile(value = CiProfile.STABLE_OR_TESTING_PROFILE)
public class SpClickhouseConfig {
    public static final String DATABASE_NAME = "testenv";

    @Bean
    public ClickHouseProperties spClickhouseProperties(
            @Value("${storage.spClickhouseProperties.user}") String user,
            @Value("${storage.spClickhouseProperties.password}") String password
    ) {
        return createClickHouseProperties(user, password);
    }

    @Bean
    public ClickHouseProperties spRunsClickhouseProperties(
            @Value("${storage.spRunsClickhouseProperties.user}") String user,
            @Value("${storage.spRunsClickhouseProperties.password}") String password
    ) {
        return createClickHouseProperties(user, password);
    }

    @Bean
    public BalancedClickhouseDataSource spClickhouse(
            @Value("${storage.spClickhouse.url}") String url,
            ClickHouseProperties spClickhouseProperties
    ) {
        var dataSource = new BalancedClickhouseDataSource(url, spClickhouseProperties);
        dataSource.scheduleActualization(10, TimeUnit.SECONDS);
        dataSource.actualize();
        Preconditions.checkState(dataSource.getEnabledClickHouseUrls().size() > 0, "No enabled clickhouse urls");
        return dataSource;
    }

    @Bean
    public BalancedClickhouseDataSource spRunsClickhouse(
            @Value("${storage.spRunsClickhouse.url}") String url,
            ClickHouseProperties spRunsClickhouseProperties
    ) {
        var dataSource = new BalancedClickhouseDataSource(url, spRunsClickhouseProperties);
        dataSource.scheduleActualization(10, TimeUnit.SECONDS);
        dataSource.actualize();
        Preconditions.checkState(dataSource.getEnabledClickHouseUrls().size() > 0, "No enabled clickhouse urls");
        return dataSource;
    }

    @Bean
    public ChangeRunTable changeRunTable(BalancedClickhouseDataSource spClickhouse) {
        return new ChangeRunTable(DATABASE_NAME, spClickhouse);
    }

    @Bean
    public LastRunTable lastRunTable(BalancedClickhouseDataSource spClickhouse) {
        return new LastRunTable(DATABASE_NAME, spClickhouse);
    }

    @Bean
    public OldTestTable oldTestTable(BalancedClickhouseDataSource spClickhouse) {
        return new OldTestTable(DATABASE_NAME, spClickhouse);
    }

    @Bean
    public RunLinkTable runLinkTable(BalancedClickhouseDataSource spClickhouse) {
        return new RunLinkTable(DATABASE_NAME, spClickhouse);
    }

    @Bean
    public TestEventTable testEventTable(BalancedClickhouseDataSource spClickhouse) {
        return new TestEventTable(DATABASE_NAME, spClickhouse);
    }

    @Bean
    public TestRunTable testRunTable(BalancedClickhouseDataSource spRunsClickhouse) {
        return new TestRunTable(DATABASE_NAME, spRunsClickhouse);
    }

    @Bean
    public TestRunV2Table testRunV2Table(BalancedClickhouseDataSource spClickhouse) {
        return new TestRunV2Table(DATABASE_NAME, spClickhouse);
    }

    @Bean
    public AutocheckClickhouse autocheckClickhouse(
            OldTestTable oldTestTable,
            ChangeRunTable changeRunTable,
            LastRunTable lastRunTable,
            TestRunTable testRunTable,
            TestEventTable testEventTable,
            RunLinkTable runLinkTable
    ) {
        return new AutocheckClickhouse(
                oldTestTable, changeRunTable, lastRunTable, testRunTable, testEventTable, runLinkTable
        );
    }

    private ClickHouseProperties createClickHouseProperties(String user, String password) {
        var properties = new ClickHouseProperties();
        properties.setUser(user);
        properties.setPassword(password);
        return properties;
    }
}
