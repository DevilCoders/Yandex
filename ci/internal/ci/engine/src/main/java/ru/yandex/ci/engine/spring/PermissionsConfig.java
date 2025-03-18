package ru.yandex.ci.engine.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.security.AccessService;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.core.spring.AbcConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;

@Configuration
@Import({
        AbcConfig.class,
        YdbCiConfig.class
})
public class PermissionsConfig {
    @Bean
    public AccessService accessService(
            AbcService abcService,
            @Value("${ci.accessService.adminAbcGroup}") String adminAbcGroup,
            @Value("${ci.accessService.adminAbcScope}") String adminAbcScope
    ) {
        return new AccessService(abcService, adminAbcGroup, adminAbcScope);
    }

    @Bean
    public PermissionsService frontendPermissionsService(
            AccessService accessService,
            CiMainDb db) {
        return new PermissionsService(accessService, db);
    }
}
