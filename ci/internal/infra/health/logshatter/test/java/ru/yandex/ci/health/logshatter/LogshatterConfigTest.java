package ru.yandex.ci.health.logshatter;


import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import ru.yandex.devtools.test.Paths;
import ru.yandex.market.clickhouse.ClickHouseSource;
import ru.yandex.market.clickhouse.ddl.ClickHouseDdlServiceOld;
import ru.yandex.market.logshatter.config.ConfigurationService;
import ru.yandex.market.logshatter.config.ddl.UpdateDDLService;

public class LogshatterConfigTest {

    @Test
    void validateConfigs() {
        var configurationService = createConfigurationService();
        var configs = configurationService.readConfigurationFromFiles();
        var report = configurationService.validateConfigs(configs);
        Assertions.assertThat(report.allConfigsAreValid()).isTrue();
    }

    private ConfigurationService createConfigurationService() {
        ClickHouseSource source = new ClickHouseSource();
        ClickHouseDdlServiceOld clickHouseDdlService = new ClickHouseDdlServiceOld();
        clickHouseDdlService.setClickHouseSource(source);

        UpdateDDLService updateDDLService = Mockito.mock(UpdateDDLService.class);

        Mockito.when(updateDDLService.getClickhouseDdlService()).thenReturn(clickHouseDdlService);

        ConfigurationService configurationService = new ConfigurationService();
        configurationService.setConfigDir(Paths.getSourcePath("ci/internal/infra/health/logshatter/conf.d"));
        configurationService.setUpdateDDLService(updateDDLService);
        return configurationService;
    }
}
