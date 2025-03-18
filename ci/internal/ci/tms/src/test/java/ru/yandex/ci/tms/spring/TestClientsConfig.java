package ru.yandex.ci.tms.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.spring.AbcStubConfig;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.core.spring.clients.SandboxProxyClientTestConfig;
import ru.yandex.ci.engine.spring.clients.ArcanumClientTestConfig;
import ru.yandex.ci.engine.spring.clients.CalendarClientTestConfig;
import ru.yandex.ci.engine.spring.clients.StaffClientTestConfig;
import ru.yandex.ci.engine.spring.clients.StorageClientTestConfig;
import ru.yandex.ci.engine.spring.clients.TestenvClientTestConfig;
import ru.yandex.ci.engine.spring.clients.XivaClientTestConfig;
import ru.yandex.ci.tms.spring.clients.SandboxClientsStubConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.JugglerClientTestConfig;

@Configuration
@Import({
        AbcStubConfig.class,
        ArcClientTestConfig.class,
        ArcanumClientTestConfig.class,
        CalendarClientTestConfig.class,
        JugglerClientTestConfig.class,
        StorageClientTestConfig.class,
        TestenvClientTestConfig.class,
        SandboxProxyClientTestConfig.class,
        SandboxClientsStubConfig.class,
        StaffClientTestConfig.class,
        XivaClientTestConfig.class
})
public class TestClientsConfig {
}
