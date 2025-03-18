package ru.yandex.ci.core.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.taskletv2.TaskletV2Client;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.spring.clients.TaskletV2ClientConfig;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataServiceImpl;

/**
 * Сервисы получения информации о тасклетах.
 */
@Configuration
@Import({
        TaskletMetadataConfig.class,
        TaskletV2ClientConfig.class
})
public class TaskletV2MetadataConfig {

    @Bean
    public TaskletV2MetadataService taskletV2MetadataService(
            TaskletV2Client taskletV2Client,
            SchemaService schemaService,
            CiMainDb db,
            MeterRegistry meterRegistry
    ) {
        return new TaskletV2MetadataServiceImpl(taskletV2Client, schemaService, db, meterRegistry);
    }
}
