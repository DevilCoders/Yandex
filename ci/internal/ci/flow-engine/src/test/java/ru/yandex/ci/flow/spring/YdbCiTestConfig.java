package ru.yandex.ci.flow.spring;

import java.util.concurrent.ExecutorService;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.ydb.spring.YdbTestConfig;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.db.CiDbImpl;
import ru.yandex.ci.flow.db.CiDbRepository;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.ydb.YdbCleanupProxy;

@Configuration
@Import({
        YdbCiConfig.class,
        YdbTestConfig.class
})
public class YdbCiTestConfig {

    @Bean
    public CiDb db(CiDbRepository ciDbRepository, ExecutorService testExecutor) {
        return YdbCleanupProxy.withCleanupProxy(new CiDbImpl(ciDbRepository), testExecutor);
    }

}
