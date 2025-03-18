package ru.yandex.ci.observer.api.spring;

import java.time.Clock;
import java.time.Duration;
import java.util.List;
import java.util.stream.IntStream;

import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.prometheus.PrometheusConfig;
import io.micrometer.prometheus.PrometheusMeterRegistry;
import io.prometheus.client.CollectorRegistry;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.arcanum.ArcanumClient;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.core.spring.clients.ArcClientConfig;
import ru.yandex.ci.observer.api.statistics.ChecksCountService;
import ru.yandex.ci.observer.api.statistics.DetailedStatisticsService;
import ru.yandex.ci.observer.api.statistics.aggregated.AggregatedStatisticsCollector;
import ru.yandex.ci.observer.api.statistics.aggregated.AggregatedStatisticsService;
import ru.yandex.ci.observer.api.statistics.model.StatisticsRegistry;
import ru.yandex.ci.observer.api.stress_test.StressTestService;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.spring.ObserverYdbConfig;

@Configuration
@Import({
        CommonConfig.class,
        ObserverYdbConfig.class,
        ClientsConfig.class,
        ArcClientConfig.class,
})
public class ObserverApiConfig {
    @Bean
    public StatisticsRegistry mainStatisticsRegistry(CollectorRegistry registry) {
        return new StatisticsRegistry(
                new PrometheusMeterRegistry(
                        PrometheusConfig.DEFAULT,
                        registry,
                        io.micrometer.core.instrument.Clock.SYSTEM
                ),
                registry
        );
    }

    @Bean
    public DetailedStatisticsService detailedStatisticsService(
            CiObserverDb ciObserverDb,
            ArcanumClient arcanumClient
    ) {
        return new DetailedStatisticsService(ciObserverDb, arcanumClient);
    }

    @Bean
    public ChecksCountService checksCountService(CiObserverDb ciObserverDb, Clock clock) {
        return new ChecksCountService(ciObserverDb, clock);
    }

    @Bean
    public AggregatedStatisticsService aggregatedStatisticsService(
            Clock clock,
            CiObserverDb ciObserverDb,
            @Value("${observer.aggregatedStatisticsService.percentileLevels}") int[] percentileLevels,
            @Value("${observer.aggregatedStatisticsService.stagesToAggregate}") String[] stagesToAggregate,
            @Value("${observer.aggregatedStatisticsService.aggregateWindows}") Duration[] aggregateWindows,
            @Value("${observer.aggregatedStatisticsService.lastSaveDelay}") Duration lastSaveDelay
    ) {
        return new AggregatedStatisticsService(
                clock,
                ciObserverDb,
                IntStream.of(percentileLevels).boxed().toList(),
                List.of(stagesToAggregate),
                List.of(aggregateWindows),
                lastSaveDelay
        );
    }

    @Bean
    public AggregatedStatisticsCollector aggregatedStatisticsCollector(
            AggregatedStatisticsService aggregatedStats,
            MeterRegistry meterRegistry,
            CiObserverDb db
    ) {
        return new AggregatedStatisticsCollector(aggregatedStats, meterRegistry, db);
    }

    @Bean
    public StressTestService stressTestService(CiObserverDb db, ArcService arcService) {
        return new StressTestService(db, arcService);
    }

}
