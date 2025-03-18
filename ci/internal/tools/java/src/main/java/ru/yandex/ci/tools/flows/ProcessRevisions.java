package ru.yandex.ci.tools.flows;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(ConfigurationServiceConfig.class)
public class ProcessRevisions extends AbstractSpringBasedApp {

    @Autowired
    RevisionNumberService revisionNumberService;

    @Override
    protected void run() {
        var revision = revisionNumberService.getOrderedArcRevision(
                ArcBranch.ofString("releases/marketsre-salt/production"),
                ArcRevision.of("dbb8ea31135edae4d19ed60fbd75ebada6af3c69")
        );
        log.info("Ordered revision: {}", revision);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}
