package ru.yandex.ci.engine.autocheck.jobs.autocheck.spring.ci;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.temporal.spring.TemporalTestConfig;
import ru.yandex.ci.core.spring.CommonTestConfig;
import ru.yandex.ci.core.spring.ydb.CommonYdbTestConfig;
import ru.yandex.ci.engine.spring.AutocheckStressTestConfig;
import ru.yandex.ci.engine.spring.EngineTestConfig;
import ru.yandex.ci.engine.spring.clients.TestenvClientTestConfig;
import ru.yandex.ci.engine.spring.tasks.EngineTasksConfig;
import ru.yandex.ci.flow.spring.YdbCiTestConfig;

@Configuration
@Import({
        //в обратном порядке из extends классов
        CommonTestConfig.class,
        CommonYdbTestConfig.class,
        YdbCiTestConfig.class,

        EngineTasksConfig.class,

        EngineTestConfig.class,
        StorageClientViaGrpcTestConfig.class,
        AutocheckStressTestConfig.class,

        TemporalTestConfig.class,

        TestenvClientTestConfig.class,
})

public class AutocheckForStorageIntegrationTestConfig {

}
