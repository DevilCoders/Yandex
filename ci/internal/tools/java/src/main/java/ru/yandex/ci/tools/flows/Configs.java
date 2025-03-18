package ru.yandex.ci.tools.flows;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.spring.SourceCodeServiceConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import({
        SourceCodeServiceConfig.class,
        YdbCiConfig.class
})
@Configuration
public class Configs extends AbstractSpringBasedApp {

    @Autowired
    SourceCodeService sourceCodeService;

    @Autowired
    CiDb db;

    @Override
    protected void run() {
        var configs = db.scan().run(() -> db.configStates().findAllVisible(true));
        for (var config : configs) {
            log.info("Config: {}", config);
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
