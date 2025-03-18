package ru.yandex.ci.tools.yamls;

import java.nio.file.Paths;
import java.util.Arrays;
import java.util.List;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.spring.ConfigurationServiceConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.ci.util.ResourceUtils;

@Slf4j
@Import(ConfigurationServiceConfig.class)
@Configuration
public class UpdateConfigurations extends AbstractSpringBasedApp {

    @Autowired
    private ConfigurationService configurationService;

    @Autowired
    private ArcService arcService;

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

    @Override
    protected void run() throws Exception {
        var head = arcService.getLastRevisionInBranch(ArcBranch.trunk());
        var headCommit = arcService.getCommit(head);

        var configPaths = loadConfigs();
        log.info("Loaded {} config paths", configPaths.size());
        for (var configPath : configPaths) {
            log.info("Processing at r{} {}", headCommit.getSvnRevision(), headCommit.getMessage());
            configurationService.getOrCreateConfig(Paths.get(configPath), headCommit.toOrderedTrunkArcRevision());
        }
    }

    private List<String> loadConfigs() {
        return Arrays.stream(ResourceUtils.textResource("rm-configs.txt").split("\n"))
                .filter(c -> !c.isBlank())
                .toList();
    }
}
