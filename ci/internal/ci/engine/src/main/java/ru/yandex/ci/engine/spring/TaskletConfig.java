package ru.yandex.ci.engine.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.engine.flow.SandboxClientFactory;
import ru.yandex.ci.engine.flow.SandboxTaskLauncher;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.engine.flow.TaskBadgeService;
import ru.yandex.ci.engine.spring.clients.SandboxClientsConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.spring.FlowControlConfig;
import ru.yandex.ci.flow.utils.UrlService;

/**
 * Сервисы для непосредственной работы с тасклетами.
 * Запуск, остановка, статус
 */
@Configuration
@Import({
        SecurityServiceConfig.class,
        FlowControlConfig.class,
        SandboxClientsConfig.class
})
public class TaskletConfig {

    @Bean
    public TaskBadgeService taskBadgeService(JobProgressService jobProgressService) {
        return new TaskBadgeService(jobProgressService);
    }

    @Bean
    public SandboxTaskLauncher taskletService(
            SchemaService schemaService,
            CiDb db,
            SecurityAccessService securityAccessService,
            @Value("${ci.taskletService.sandboxBaseUrl}") String sandboxBaseUrl,
            SandboxClientFactory sandboxClientFactory,
            UrlService urlService,
            TaskBadgeService taskBadgeService,
            TaskletContextProcessor taskletContextProcessor
    ) {
        return new SandboxTaskLauncher(
                schemaService,
                db,
                securityAccessService,
                sandboxClientFactory,
                urlService,
                sandboxBaseUrl,
                taskBadgeService,
                taskletContextProcessor
        );
    }
}
