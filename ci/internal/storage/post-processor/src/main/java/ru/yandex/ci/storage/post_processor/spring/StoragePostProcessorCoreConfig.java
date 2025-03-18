package ru.yandex.ci.storage.post_processor.spring;

import java.time.Clock;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.storage.core.db.model.test_revision.FragmentationSettings;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;

@Configuration
@Import({
        CommonConfig.class
})
public class StoragePostProcessorCoreConfig {
    @Autowired
    private MeterRegistry meterRegistry;

    @Autowired
    private CollectorRegistry collectorRegistry;

    @Bean
    public TimeTraceService timeTraceService(Clock clock) {
        return new TimeTraceService(clock, collectorRegistry);
    }

    @Bean
    public PostProcessorStatistics postProcessorStatistics(Clock clock) {
        return new PostProcessorStatistics(this.meterRegistry, this.collectorRegistry, clock);
    }

    @Bean
    public FragmentationSettings fragmentationSettings(
            @Value("${storage.fragmentationSettings.revisionsInBucket}") int revisionsInBucket,
            @Value("${storage.fragmentationSettings.numberOfBucketsInRegion}") int numberOfBucketsInRegion
    ) {
        return FragmentationSettings.create(revisionsInBucket, numberOfBucketsInRegion);
    }
}
