package ru.yandex.monlib.metrics.example;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.monlib.metrics.example.services.series.SeriesService;
import ru.yandex.monlib.metrics.example.services.series.metrics.SeriesMetrics;
import ru.yandex.monlib.metrics.example.web.controllers.SeriesRestController;
import ru.yandex.monlib.metrics.example.web.controllers.validation.SeriesValidator;

/**
 * @author Alexey Trushkin
 */
@Configuration
@Import({
        SeriesRestController.class
})
public class ExamplesDataModelContext {

    @Bean
    public ObjectMapper objectMapper() {
        return new ObjectMapper();
    }

    @Bean
    public SeriesService seriesService() {
        return new SeriesService(seriesServiceMetrics());
    }

    @Bean
    public SeriesMetrics seriesServiceMetrics() {
        return new SeriesMetrics();
    }

    @Bean
    public SeriesValidator seriesValidator() {
        return new SeriesValidator();
    }
}
