package ru.yandex.ci.api.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.api.controllers.admin.GraphDiscoveryAdminController;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.security.AccessService;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;

@Configuration
@Import(FrontendApiConfig.class)
public class AdminApiConfig {

    @Bean
    public GraphDiscoveryAdminController graphDiscoveryAdminController(
            AccessService accessService,
            GraphDiscoveryService graphDiscoveryService,
            ArcService arcService
    ) {
        return new GraphDiscoveryAdminController(accessService, graphDiscoveryService, arcService);
    }

}
