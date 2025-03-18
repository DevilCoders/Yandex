package ru.yandex.ci.flow.spring;

import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.ApplicationContext;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.TaskletMetadataConfig;
import ru.yandex.ci.core.spring.TaskletV2MetadataConfig;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.flow.engine.runtime.JobExecutorObjectProvider;
import ru.yandex.ci.flow.engine.runtime.JobExecutorObjectProviderImpl;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.engine.source_code.impl.ReflectionsSourceCodeProvider;
import ru.yandex.ci.flow.engine.source_code.impl.SourceCodeServiceImpl;

@Configuration
@Import({
        TaskletMetadataConfig.class,
        TaskletV2MetadataConfig.class
})
public class SourceCodeServiceConfig {

    @Bean
    public JobExecutorObjectProvider executorObjectProvider(
            TaskletMetadataService taskletMetadataService,
            TaskletV2MetadataService taskletV2MetadataService
    ) {
        return new JobExecutorObjectProviderImpl(taskletMetadataService, taskletV2MetadataService);
    }

    @Bean
    public SourceCodeService sourceCodeEntityService(
            JobExecutorObjectProvider jobExecutorObjectProvider,
            ApplicationContext applicationContext,
            @Value("${ci.sourceCodeEntityService.resolveBeansOnRefresh}") boolean resolveBeansOnRefresh,
            MeterRegistry meterRegistry) {
        return new SourceCodeServiceImpl(
                new ReflectionsSourceCodeProvider(ReflectionsSourceCodeProvider.SOURCE_CODE_PACKAGE),
                jobExecutorObjectProvider,
                applicationContext,
                resolveBeansOnRefresh,
                meterRegistry
        );
    }
}
