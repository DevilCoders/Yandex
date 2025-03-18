package ru.yandex.ci.engine.spring;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.BazingaCoreStubConfig;
import ru.yandex.ci.common.bazinga.spring.TestZkConfig;
import ru.yandex.ci.core.spring.AbcStubConfig;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.core.spring.clients.PciExpressTestConfig;
import ru.yandex.ci.core.spring.clients.SandboxClientTestConfig;
import ru.yandex.ci.core.spring.clients.SandboxProxyClientTestConfig;
import ru.yandex.ci.core.spring.clients.TaskletV2ClientTestConfig;
import ru.yandex.ci.engine.spring.clients.ArcanumClientTestConfig;
import ru.yandex.ci.engine.spring.clients.CalendarClientTestConfig;
import ru.yandex.ci.engine.spring.clients.SecurityClientsTestConfig;
import ru.yandex.ci.engine.spring.clients.StaffClientTestConfig;
import ru.yandex.ci.engine.spring.clients.StorageClientTestConfig;
import ru.yandex.ci.engine.spring.clients.XivaClientTestConfig;

@Configuration
@Import({
        TestZkConfig.class,
        BazingaCoreStubConfig.class,
        TestServicesHelpersConfig.class,

        AbcStubConfig.class,
        LogbrokerLaunchEventTestConfig.class,

        ArcClientTestConfig.class,
        ArcanumClientTestConfig.class,
        CalendarClientTestConfig.class,
        SandboxClientTestConfig.class,
        SandboxProxyClientTestConfig.class,
        SecurityClientsTestConfig.class,
        StaffClientTestConfig.class,
        StorageClientTestConfig.class,
        XivaClientTestConfig.class,
        PciExpressTestConfig.class,
        TaskletV2ClientTestConfig.class
})
public class EngineTestConfig {
}
