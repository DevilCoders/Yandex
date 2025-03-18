package ru.yandex.ci.engine.spring;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.sandbox.ProxySandboxClient;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.spring.clients.ArcClientConfig;
import ru.yandex.ci.core.spring.clients.SandboxProxyClientConfig;
import ru.yandex.ci.engine.pciexpress.GetPciDssCommitResultJob;
import ru.yandex.ci.engine.pciexpress.StartPciDssCommitStatusJob;

@Configuration
@Import({
        ArcClientConfig.class,
        SandboxProxyClientConfig.class,
})
public class PciExpressConfig {
    @Bean
    public StartPciDssCommitStatusJob startPciDssCommitStatusJob(
            ArcService arcService,
            @Value("${ci.arcService.endpoint}") String arcEndpoint) {
        return new StartPciDssCommitStatusJob(arcService, arcEndpoint);
    }

    @Bean
    public GetPciDssCommitResultJob getPciDssCommitResult(
            ProxySandboxClient proxySandboxClient
    ) {
        return new GetPciDssCommitResultJob(proxySandboxClient);
    }
}
