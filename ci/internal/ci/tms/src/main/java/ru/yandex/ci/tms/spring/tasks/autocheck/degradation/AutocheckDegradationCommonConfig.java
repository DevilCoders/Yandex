package ru.yandex.ci.tms.spring.tasks.autocheck.degradation;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tms.task.autocheck.degradation.AutocheckDegradationStateKeeper;

@Configuration
@Import({
        BazingaCoreConfig.class,
        YdbCiConfig.class
})
public class AutocheckDegradationCommonConfig {

    @Bean
    public AutocheckDegradationStateKeeper autocheckDegradationStateKeeper(CiDb ciDb) {
        return new AutocheckDegradationStateKeeper(ciDb);
    }

}
