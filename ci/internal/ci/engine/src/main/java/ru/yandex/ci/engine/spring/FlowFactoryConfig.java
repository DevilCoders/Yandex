package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.launch.FlowVarsService;
import ru.yandex.ci.core.spring.TaskletMetadataConfig;
import ru.yandex.ci.core.spring.TaskletV2MetadataConfig;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.engine.launch.FlowFactory;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.spring.SourceCodeServiceConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;

@Configuration
@Import({
        YdbCiConfig.class,
        TaskletMetadataConfig.class,
        TaskletV2MetadataConfig.class,
        SourceCodeServiceConfig.class
})
public class FlowFactoryConfig {

    @Bean
    public FlowFactory flowFactory(
            TaskletMetadataService taskletMetadataService,
            TaskletV2MetadataService taskletV2MetadataService,
            SchemaService schemaService,
            SourceCodeService sourceCodeService) {
        return new FlowFactory(taskletMetadataService, taskletV2MetadataService, schemaService, sourceCodeService);
    }

    @Bean
    public FlowVarsService flowVarsService(SchemaService schemaService) {
        return new FlowVarsService(schemaService);
    }
}
