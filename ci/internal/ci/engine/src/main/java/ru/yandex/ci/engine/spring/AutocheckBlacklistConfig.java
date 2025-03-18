package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.staff.StaffClient;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.spring.clients.ArcClientConfig;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.spring.clients.StaffClientConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;

@Configuration
@Import({
        ArcClientConfig.class,
        StaffClientConfig.class,
        YdbCiConfig.class,
})
public class AutocheckBlacklistConfig {

    @Bean(initMethod = "startAsync")
    public AutocheckBlacklistService autocheckBlacklistService(
            ArcService arcService,
            StaffClient staffClient,
            CiMainDb db
    ) {
        return new AutocheckBlacklistService(arcService, staffClient, db);
    }

}
