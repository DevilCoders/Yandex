package ru.yandex.ci.tools.yamls;

import java.nio.file.Path;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(YdbCiConfig.class)
public class LoadConfigStates extends AbstractSpringBasedApp {

    @Autowired
    private CiDb db;

    @Override
    protected void run() throws JsonProcessingException, ProcessingException {
        var configStates = db.scan().run(() -> db.configStates().findAll());
        for (var state : configStates) {
            log.info("State: {}", state);
        }

        var path = Path.of("taxi/lavka/frontend/a.yaml");
        for (var state : configStates) {
            if (state.getConfigPath().equals(path)) {
                log.info("Matched state: {}", state);
            }
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }
}
