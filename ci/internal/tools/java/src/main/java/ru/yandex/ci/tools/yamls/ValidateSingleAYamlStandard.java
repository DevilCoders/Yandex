package ru.yandex.ci.tools.yamls;

import java.nio.file.Path;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.engine.config.ConfigParseService;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(ConfigurationServiceConfig.class)
@Configuration
public class ValidateSingleAYamlStandard extends AbstractSpringBasedApp {

    @Autowired
    ConfigParseService configParseService;

    @Override
    protected void run() {
        var config = configParseService.parseAndValidate(
                Path.of("alice/alice4business/alice-in-business-api/a.yaml"),
                ArcRevision.of("7588b95b8161b6e8e4450fa1fc2e564af66ebc9a"),
                null);
        log.info("Status: {}", config.getStatus());
        for (var problem : config.getProblems()) {
            log.info("{}", problem);
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
