package ru.yandex.ci.tools.flows;

import java.util.List;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.api.controllers.storage.ConfigBundleCollectionService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(ConfigurationServiceConfig.class)
public class GetLargeTestsPrefixes extends AbstractSpringBasedApp {

    @Autowired
    ConfigurationService configurationService;

    @Autowired
    CiDb db;

    @Override
    protected void run() {
        var service = new ConfigBundleCollectionService(configurationService, db);
        var prefixes = service.getLastValidConfigs(9611738, List.of("maps/automotive/remote_access/autotests"));
        log.info("Loaded prefixes: {}", prefixes.size());
        for (var prefix : prefixes) {
            log.info("{}", prefix);
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
