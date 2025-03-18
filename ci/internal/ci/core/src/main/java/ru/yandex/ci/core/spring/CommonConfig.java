package ru.yandex.ci.core.spring;

import java.time.Clock;
import java.time.Duration;

import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.prometheus.PrometheusConfig;
import io.micrometer.prometheus.PrometheusMeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Profile;

import ru.yandex.ci.client.base.http.CallsMonitorSource;
import ru.yandex.ci.common.application.profiles.CiProfile;
import ru.yandex.ci.metrics.PrometheusCallsMonitorSource;

@Configuration
@Profile(CiProfile.NOT_UNIT_TEST_PROFILE)
public class CommonConfig {

    @Bean
    public Clock clock() {
        return defaultClock();
    }

    @Bean
    public MeterRegistry meterRegistry() {
        return new PrometheusMeterRegistry(PrometheusConfig.DEFAULT);
    }

    @Bean
    public CollectorRegistry collectorRegistry() {
        return CollectorRegistry.defaultRegistry;
    }

    @Bean
    public CallsMonitorSource callsMonitorSource(
            CollectorRegistry registry,
            @Value("#{'${ci.clients.histogram.buckets.seconds:" +
                    "0.001,0.005,0.01,0.05,0.075,0.1,0.25,0.5,1,2,5,10,20,30}'.split(',')}") double[] buckets) {
        return new PrometheusCallsMonitorSource(registry, buckets);
    }

    public static Clock defaultClock() {
        return Clock.tick(Clock.systemUTC(), Duration.ofNanos(1000));
    }
}
