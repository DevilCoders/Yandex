package ru.yandex.ci.flow.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.DispatcherJobScheduler;
import ru.yandex.ci.flow.engine.runtime.bazinga.BazingaJobScheduler;
import ru.yandex.ci.flow.engine.runtime.temporal.TemporalJobScheduler;
import ru.yandex.ci.flow.spring.temporal.CiTemporalServiceConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Configuration
@Import({
        FlowBazingaConfig.class,
        YdbCiConfig.class,
        CiTemporalServiceConfig.class
})
public class FlowJobSchedulerConfig {

    @Bean
    public DispatcherJobScheduler jobScheduler(
            CiDb ciDb,
            BazingaTaskManager bazingaTaskManager,
            TemporalService temporalService
    ) {
        var bazingaJobScheduler = new BazingaJobScheduler(bazingaTaskManager);
        var temporalJobScheduler = new TemporalJobScheduler(temporalService);

        return new DispatcherJobScheduler(
                temporalJobScheduler,
                bazingaJobScheduler,
                ciDb
        );
    }
}
