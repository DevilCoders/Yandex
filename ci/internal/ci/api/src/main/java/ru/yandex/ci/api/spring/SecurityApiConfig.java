package ru.yandex.ci.api.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.api.controllers.internal.SecurityApiController;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.flow.SecurityDelegationService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.spring.LaunchConfig;

@Configuration
@Import({
        LaunchConfig.class
})
public class SecurityApiConfig {

    @Bean
    public SecurityApiController securityApiController(
            SecurityDelegationService securityDelegationService,
            ConfigurationService configurationService,
            LaunchService launchService
    ) {
        return new SecurityApiController(securityDelegationService, configurationService, launchService);
    }

}
