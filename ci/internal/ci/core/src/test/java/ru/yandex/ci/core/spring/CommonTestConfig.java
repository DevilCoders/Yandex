package ru.yandex.ci.core.spring;

import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.simple.SimpleMeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.common.application.ConversionConfig;
import ru.yandex.ci.test.clock.OverridableClock;

@Configuration
@PropertySource("classpath:ci-core/ci-common.properties")
@PropertySource("classpath:ci-core/ci-common-unit-test.properties")
@Import(ConversionConfig.class)
public class CommonTestConfig {

    @Bean
    public MeterRegistry meterRegistry() {
        return new SimpleMeterRegistry();
    }

    @Bean
    public CollectorRegistry collectorRegistry() {
        return new CollectorRegistry(true);
    }

    @Bean
    public OverridableClock clock() {
        return new OverridableClock();
    }

    @Bean
    public CallsMonitorSource callsMonitorSource() {
        return CallsMonitorSource.simple();
    }
}
