package ru.yandex.ci.storage.shard.spring;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.storage.shard.ShardStatistics;

@Import(CommonConfig.class)
@Configuration
public class StorageShardStatisticsConfig {

    @Autowired
    private MeterRegistry meterRegistry;

    @Autowired
    private CollectorRegistry collectorRegistry;

    @Bean
    public ShardStatistics shardStatistics() {
        return new ShardStatistics(meterRegistry, collectorRegistry);
    }

}
