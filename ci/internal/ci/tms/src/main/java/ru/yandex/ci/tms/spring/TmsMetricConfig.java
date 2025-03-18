package ru.yandex.ci.tms.spring;

import java.time.Duration;

import com.google.common.util.concurrent.MoreExecutors;
import io.micrometer.core.instrument.MeterRegistry;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.guava.ShutdownServiceListener;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tms.metric.ci.CiSystemsUsageMetrics;
import ru.yandex.ci.ydb.service.metric.YdbSolomonMetricService;

@Configuration
@Import({
        CommonConfig.class,
        YdbCiConfig.class
})
public class TmsMetricConfig {

    @Bean(initMethod = "startAsync")
    public YdbSolomonMetricService ciSystemsUsageYdbSolomonMetricService(
            MeterRegistry meterRegistry,
            CiMainDb db) {
        YdbSolomonMetricService service = new YdbSolomonMetricService(
                meterRegistry, db, Duration.ofMinutes(10), Duration.ofDays(3), CiSystemsUsageMetrics.allMetrics()
        );
        service.addListener(new ShutdownServiceListener("ShardService"), MoreExecutors.directExecutor());
        return service;
    }
}
