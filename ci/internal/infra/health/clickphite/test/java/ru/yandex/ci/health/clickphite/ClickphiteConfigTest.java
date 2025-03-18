package ru.yandex.ci.health;

import java.util.function.Function;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.test.context.ContextConfiguration;
import org.springframework.test.context.junit4.SpringJUnit4ClassRunner;

import ru.yandex.devtools.test.Paths;
import ru.yandex.market.clickphite.config.ClickphiteConfigsTest;
import ru.yandex.market.clickphite.config.ConfigurationService;
import ru.yandex.market.clickphite.config.TestConfiguration;
import ru.yandex.market.clickphite.config.validation.context.ConfigValidator;


@RunWith(SpringJUnit4ClassRunner.class)
@ContextConfiguration(classes = {TestConfiguration.class, ClickphiteConfigTest.Config.class})
public class ClickphiteConfigTest {

    @Autowired
    private Function<String, ConfigurationService> configurationServiceFactory;

    @Test
    public void validate() {
        var configPath = "ci/internal/infra/health/clickphite/conf.d";
        var configurationService = configurationServiceFactory.apply(Paths.getSourcePath(configPath));
        configurationService.setUseNewConfigLoading(false);
        ClickphiteConfigsTest.doValidateConfigs(configurationService);
    }

    public static class Config {
        @Bean
        public ConfigValidator configValidator() {
            return new ConfigValidator("", "");
        }
    }
}
