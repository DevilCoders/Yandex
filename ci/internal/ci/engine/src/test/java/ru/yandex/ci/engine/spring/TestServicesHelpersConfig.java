package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.EngineTester;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.flow.SecurityDelegationService;
import ru.yandex.ci.engine.spring.clients.SecurityClientsTestConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestQueries;

@Configuration
@Import({
        ConfigurationServiceConfig.class,
        SecurityClientsTestConfig.class
})
public class TestServicesHelpersConfig {

    @Bean
    public FlowTestQueries flowTestQueries(CiDb db) {
        return new FlowTestQueries(db);
    }

    @Bean
    public EngineTester engineTester(
            YavClient yavClient,
            SecurityDelegationService securityDelegationService,
            SandboxClient securitySandboxClient,
            ConfigurationService configurationService,
            CiMainDb db,
            FlowTestQueries flowTestQueries
    ) {
        return new EngineTester(
                yavClient,
                securityDelegationService,
                securitySandboxClient,
                configurationService,
                db,
                flowTestQueries);
    }

}
