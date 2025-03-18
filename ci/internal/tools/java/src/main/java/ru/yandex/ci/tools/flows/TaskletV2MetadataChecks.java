package ru.yandex.ci.tools.flows;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.TaskletV2MetadataConfig;
import ru.yandex.ci.core.tasklet.SchemaOptions;
import ru.yandex.ci.core.taskletv2.TaskletV2Metadata;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import({
        YdbCiConfig.class,
        TaskletV2MetadataConfig.class
})
public class TaskletV2MetadataChecks extends AbstractSpringBasedApp {

    @Autowired
    TaskletV2MetadataService service;

    @Override
    protected void run() {
        var id = TaskletV2Metadata.Description.of("test-tasklets", "dummy_java_tasklet", "version1");
        var metadata = service.getLocalCache().fetchMetadata(id);

        var options = SchemaOptions.builder().singleInput(true).singleOutput(true).build();
        var schema = service.extractSchema(metadata, options);
        log.info("Schema: {}", schema);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
