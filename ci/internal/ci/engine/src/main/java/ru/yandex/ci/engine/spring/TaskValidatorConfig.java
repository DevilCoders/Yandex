package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.launch.FlowVarsService;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.engine.config.validation.InputOutputTaskValidator;
import ru.yandex.ci.engine.config.validation.ValidationResourceProvider;
import ru.yandex.ci.engine.launch.FlowFactory;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;

@Configuration
@Import({
        FlowFactoryConfig.class
})
public class TaskValidatorConfig {

    @Bean
    public ValidationResourceProvider validationResourceProvider() {
        return new ValidationResourceProvider();
    }

    @Bean
    public InputOutputTaskValidator inputOutputTaskValidator(
            FlowFactory flowFactory,
            SourceCodeService sourceCodeService,
            ValidationResourceProvider validationResourceProvider,
            TaskletMetadataService taskletMetadataService,
            FlowVarsService flowVarsService
    ) {
        return new InputOutputTaskValidator(
                flowFactory, sourceCodeService, validationResourceProvider, taskletMetadataService, flowVarsService
        );
    }
}
