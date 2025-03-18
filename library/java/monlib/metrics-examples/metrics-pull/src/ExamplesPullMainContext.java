package ru.yandex.monlib.metrics.example.pull;

import java.util.List;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.web.reactive.config.DelegatingWebFluxConfiguration;
import org.springframework.web.reactive.result.method.annotation.RequestMappingHandlerMapping;

import ru.yandex.monlib.metrics.example.ExamplesDataModelContext;
import ru.yandex.monlib.metrics.example.pull.web.controllers.MetricsRestController;
import ru.yandex.monlib.metrics.registry.MetricRegistry;
import ru.yandex.monlib.metrics.webflux.filters.HttpStatsFilter;
import ru.yandex.monlib.metrics.webflux.filters.RequestPatterns;

/**
 * @author Alexey Trushkin
 */
@Configuration
@Import({
        MetricsRestController.class,
        ExamplesDataModelContext.class
})
public class ExamplesPullMainContext extends DelegatingWebFluxConfiguration {

    @Bean
    public ObjectMapper objectMapper() {
        return new ObjectMapper();
    }

    @Bean
    public MetricRegistry metricRegistry() {
        return MetricRegistry.root();
    }

    @Bean
    public HttpStatsFilter httpMetricsFilter(RequestMappingHandlerMapping handlerMapping) {
        return new HttpStatsFilter(metricRegistry(),
                requestPatterns(handlerMapping), List.of());
    }

    @Bean
    public RequestPatterns requestPatterns(RequestMappingHandlerMapping handlerMapping) {
        return new RequestPatterns(handlerMapping);
    }
}
