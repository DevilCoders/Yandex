package ru.yandex.ci.core;

import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.spring.AYamlServiceConfig;
import ru.yandex.ci.core.spring.TaskletMetadataConfig;
import ru.yandex.ci.core.spring.TaskletV2MetadataConfig;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.core.spring.clients.SandboxClientTestConfig;
import ru.yandex.ci.core.spring.clients.TaskletV2ClientTestConfig;

@ContextConfiguration(classes = {
        TaskletMetadataConfig.class,
        TaskletV2MetadataConfig.class,
        AYamlServiceConfig.class,
        SandboxClientTestConfig.class,
        ArcClientTestConfig.class,
        TaskletV2ClientTestConfig.class
})
public class CoreYdbTestBase extends CommonYdbTestBase {
}
