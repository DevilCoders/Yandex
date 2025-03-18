package ru.yandex.ci.tools.client;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.tsum.TsumClient;
import ru.yandex.ci.tms.spring.clients.TsumClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(TsumClientConfig.class)
@Configuration
public class TsumClientTest extends AbstractSpringBasedApp {

    @Autowired
    TsumClient tsumClient;

    @Override
    protected void run() {
        log.info("Projects: {}", tsumClient.getProjects());

        var project = tsumClient.getProject("pricelabs");
        log.info("Project: {}", project);

        var cfg  = tsumClient.getPipelineConfiguration("pricelabs-v2", 48);
        log.info("Pipeline config: {}", cfg);

        var fullCfg  = tsumClient.getPipelineFullConfiguration("60d3853d1a58db280caa420b");
        log.info("Pipeline full cfg: {}", fullCfg);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

}
