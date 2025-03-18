package ru.yandex.ci.flow.spring;

import java.time.Clock;
import java.util.HashMap;

import com.fasterxml.jackson.core.type.TypeReference;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.Lazy;

import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.context.impl.UpstreamResourcesCollector;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.JobConditionalChecks;
import ru.yandex.ci.flow.engine.runtime.JobContextFactory;
import ru.yandex.ci.flow.engine.runtime.JobContextFactoryImpl;
import ru.yandex.ci.flow.engine.runtime.JobExecutorProvider;
import ru.yandex.ci.flow.engine.runtime.JobExecutorProviderImpl;
import ru.yandex.ci.flow.engine.runtime.JobLauncher;
import ru.yandex.ci.flow.engine.runtime.JobResourcesValidator;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.zookeeper.CuratorFactory;
import ru.yandex.ci.util.FileUtils;

// TODO: CI-2866
@Configuration
@Import({
        FlowControlConfig.class
})
public class FlowServicesConfig {

    @Bean
    public JobExecutorProvider jobExecutorProvider(
            SourceCodeService sourceCodeService,
            JobExecutor launchTaskletExecutor,
            JobExecutor launchTaskletV2Executor,
            JobExecutor launchSandboxTaskExecutor,
            TaskletContextProcessor taskletContextProcessor
    ) {
        return new JobExecutorProviderImpl(
                sourceCodeService,
                launchTaskletExecutor,
                launchTaskletV2Executor,
                launchSandboxTaskExecutor,
                taskletContextProcessor
        );
    }

    @Bean
    public JobConditionalChecks jobBypassChecks(
            TaskletContextProcessor taskletContextProcessor
    ) {
        return new JobConditionalChecks(taskletContextProcessor);
    }

    @Bean
    public JobResourcesValidator jobResourcesValidator(
            CiDb db,
            Clock clock,
            @Value("${ci.jobResourcesValidator.limitsResource}") String limitsResource
    ) {
        var typeRef = new TypeReference<HashMap<String, JobResourcesValidator.Limits>>() {

        };
        var limits = FileUtils.parseJson(limitsResource, typeRef);
        return new JobResourcesValidator(db, clock, limits);
    }

    @Bean
    public JobLauncher jobLauncher(
            CuratorFactory curatorFactory,
            FlowStateService flowStateService,
            JobExecutorProvider jobExecutorProvider,
            JobConditionalChecks jobConditionalChecks,
            JobResourcesValidator jobResourcesValidator,
            CiDb db,
            @Lazy JobContextFactory jobContextFactory
    ) {
        return new JobLauncher(
                curatorFactory,
                flowStateService,
                db,
                jobContextFactory,
                jobExecutorProvider,
                jobConditionalChecks,
                jobResourcesValidator);
    }

    @Bean
    public JobContextFactory jobContextFactory(
            JobProgressService jobProgressService,
            UpstreamResourcesCollector upstreamResourcesCollector,
            FlowStateService flowStateService,
            ResourceService resourceService,
            SourceCodeService sourceCodeService
    ) {
        return new JobContextFactoryImpl(
                jobProgressService,
                upstreamResourcesCollector,
                flowStateService,
                resourceService,
                sourceCodeService
        );
    }
}
