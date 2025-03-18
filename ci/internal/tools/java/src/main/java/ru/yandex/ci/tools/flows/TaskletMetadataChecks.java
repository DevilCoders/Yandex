package ru.yandex.ci.tools.flows;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.TaskletMetadataConfig;
import ru.yandex.ci.core.tasklet.SchemaOptions;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import({
        YdbCiConfig.class,
        TaskletMetadataConfig.class
})
public class TaskletMetadataChecks extends AbstractSpringBasedApp {

    @Autowired
    TaskletMetadataService service;

    @Override
    protected void run() {
        var id = TaskletMetadata.Id.of("RunCommand", TaskletRuntime.SANDBOX, 2876764261L);
        var metadata = service.fetchMetadata(id);

        var options = SchemaOptions.defaultOptions();
        var schema = service.extractSchema(metadata, options);
        log.info("Schema: {}", schema);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
