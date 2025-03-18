package ru.yandex.ci.tools.yamls;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.engine.registry.TaskRegistry;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(ConfigurationServiceConfig.class)
public class ValidateTasklet extends AbstractSpringBasedApp {

    @Autowired
    private TaskRegistry taskRegistry;

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

    @Override
    protected void run() throws Exception {
        var configs = taskRegistry.loadRegistry();
        log.info("Loaded {} configs", configs.size());
    }
}
