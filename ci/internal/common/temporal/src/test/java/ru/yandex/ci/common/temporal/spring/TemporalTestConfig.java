package ru.yandex.ci.common.temporal.spring;

import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.simple.SimpleMeterRegistry;
import io.temporal.client.WorkflowClientOptions;
import io.temporal.testing.TestEnvironmentOptions;
import io.temporal.testing.TestWorkflowEnvironment;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;

import ru.yandex.ci.common.application.ConversionConfig;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.config.TemporalConfigurationUtil;
import ru.yandex.ci.common.temporal.config.TemporalSystemConfig;
import ru.yandex.ci.common.temporal.logging.TemporalLogService;
import ru.yandex.ci.common.temporal.ydb.TemporalDb;

@Configuration
@PropertySource("classpath:temporal/temporal-default.properties")
@Import({
        TemporalYdbTestConfig.class,
        TemporalSystemConfig.class,
        ConversionConfig.class
})
public class TemporalTestConfig {

    @MockBean
    private TemporalLogService temporalLogService;

    @Bean
    public TemporalLogService temporalLogService() {
        return temporalLogService;
    }

    @Bean
    public MeterRegistry meterRegistry() {
        return new SimpleMeterRegistry();
    }

    @Bean(initMethod = "start")
    public TestWorkflowEnvironment testWorkflowEnvironment() {
        return TestWorkflowEnvironment.newInstance(
                TestEnvironmentOptions.newBuilder()
                        .setWorkflowClientOptions(
                                WorkflowClientOptions.newBuilder()
                                        .setDataConverter(TemporalConfigurationUtil.dataConverter())
                                        .validateAndBuildWithDefaults()
                        )
                        .validateAndBuildWithDefaults()
        );
    }

    @Bean
    public TemporalService temporalService(
            TemporalDb temporalDb,
            TestWorkflowEnvironment testWorkflowEnvironment
    ) {
        return new TemporalService(
                testWorkflowEnvironment.getWorkflowClient(),
                temporalDb,
                TemporalConfigurationUtil.dataConverter(),
                "http://ci-temporal-testing.yandex.net/"
        );
    }

}
