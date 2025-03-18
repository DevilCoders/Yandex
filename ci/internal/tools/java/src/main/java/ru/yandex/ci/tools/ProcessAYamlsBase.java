package ru.yandex.ci.tools;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.engine.config.ConfigParseService;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.flow.db.CiDb;

@Import(ConfigurationServiceConfig.class)
@Configuration
public abstract class ProcessAYamlsBase extends AbstractSpringBasedApp {

    @Autowired
    protected CiDb db;

    @Autowired
    protected ConfigParseService configParseService;

    @Autowired
    protected ArcService arcService;

    protected class YamlScanner extends ru.yandex.ci.engine.config.process.YamlScanner {
        public YamlScanner() {
            super(db, configParseService, arcService);
        }
    }
}
