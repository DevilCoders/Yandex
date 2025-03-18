package ru.yandex.ci.flow.spring;

import java.time.Clock;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.CommonConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.state.calculator.ResourceProvider;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.flow.utils.UrlService;

@Configuration
@Import({
        YdbCiConfig.class,
        CommonConfig.class,
        UrlServiceConfig.class,
        FlowEngineJobsConfig.class
})

public class CommonServicesConfig {

    @Bean
    public TaskletContextProcessor taskletContextProcessor(UrlService urlService) {
        return new TaskletContextProcessor(urlService);
    }

    @Bean
    public ResourceProvider resourceProvider() {
        return new ResourceProvider();
    }

    @Bean
    public ResourceService resourceService(CiDb db, Clock clock) {
        return new ResourceService(db, clock);
    }
}
