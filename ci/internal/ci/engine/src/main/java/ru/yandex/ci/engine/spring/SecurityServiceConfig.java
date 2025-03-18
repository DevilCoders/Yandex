package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.security.AccessService;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.core.spring.AbcConfig;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.engine.flow.SecurityDelegationService;
import ru.yandex.ci.engine.flow.SecurityStateService;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.spring.clients.SecurityClientsConfig;

@Configuration
@Import({
        SecurityClientsConfig.class,
        AbcConfig.class,
        PullRequestConfig.class,
        PermissionsConfig.class
})
public class SecurityServiceConfig {

    @Bean
    public SecurityDelegationService securityService(
            YavClient yavClient,
            CiMainDb db,
            PullRequestService pullRequestService,
            SandboxClient securitySandboxClient,
            PermissionsService permissionsService
    ) {
        return new SecurityDelegationService(
                yavClient,
                db,
                securitySandboxClient,
                pullRequestService,
                permissionsService
        );
    }

    @Bean
    public SecurityStateService securityStateService(
            CiMainDb db,
            AccessService accessService,
            PermissionsService permissionsService
    ) {
        return new SecurityStateService(db, accessService, permissionsService);
    }

    @Bean
    public SecurityAccessService securityAccessService(
            YavClient yavClient,
            CiMainDb db) {
        return new SecurityAccessService(yavClient, db);
    }
}
