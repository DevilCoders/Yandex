package ru.yandex.ci.core.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.common.ydb.spring.YdbConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.spring.clients.SandboxClientConfig;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.tasklet.TaskletMetadataServiceImpl;

/**
 * Сервисы получения информации о тасклетах.
 */
@Configuration
@Import({
        YdbConfig.class,
        SandboxClientConfig.class
})
public class TaskletMetadataConfig {

    @Bean
    public SchemaService schemaService() {
        return new SchemaService();
    }

    @Bean
    public TaskletMetadataService taskletMetadataService(
            CiMainDb db,
            SandboxClient sandboxClient,
            SchemaService schemaService,
            MeterRegistry meterRegistry
    ) {
        return new TaskletMetadataServiceImpl(
                sandboxClient,
                db,
                schemaService,
                meterRegistry
        );
    }
}
