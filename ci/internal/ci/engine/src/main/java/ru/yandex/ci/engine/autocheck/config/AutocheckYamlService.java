package ru.yandex.ci.engine.autocheck.config;

import java.nio.file.Path;

import com.google.common.annotations.VisibleForTesting;
import lombok.Value;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;

public class AutocheckYamlService {
    @VisibleForTesting
    static final Path AUTOCHECK_YAML_PATH = Path.of("autocheck/autocheck.yaml");

    private final ArcService arcService;

    public AutocheckYamlService(ArcService arcService) {
        this.arcService = arcService;
    }

    public ConfigBundle getLastConfigForRevision(CommitId revision) {
        var lastChangeRevision = arcService.getLastCommit(AUTOCHECK_YAML_PATH, revision)
                .orElseThrow(() -> new RuntimeException("Unable to find last commit for " + AUTOCHECK_YAML_PATH));
        AutocheckYamlConfig config = getConfig(lastChangeRevision);
        return new ConfigBundle(lastChangeRevision.getRevision(), config);
    }

    public ConfigBundle getLastTrunkConfig() {
        var trunkHead = arcService.getLastRevisionInBranch(ArcBranch.trunk());
        var lastChangeRevision = arcService.getLastCommit(AUTOCHECK_YAML_PATH, trunkHead)
                .orElseThrow(() -> new RuntimeException("Unable to find " + AUTOCHECK_YAML_PATH + "in trunk"))
                .getRevision();
        AutocheckYamlConfig config = getConfig(lastChangeRevision);
        return new ConfigBundle(lastChangeRevision, config);
    }

    public AutocheckYamlConfig getConfig(CommitId revision) {
        String yaml = arcService.getFileContent(AUTOCHECK_YAML_PATH, revision)
                .orElseThrow(() -> new RuntimeException("Unable to load file content of " + AUTOCHECK_YAML_PATH +
                        " on revision " + revision));
        try {
            return AutocheckYamlParser.parse(yaml);
        } catch (Exception e) {
            throw new RuntimeException("Failed to parse " + AUTOCHECK_YAML_PATH + " on revision " + revision, e);
        }
    }

    @Value
    public static class ConfigBundle {
        ArcRevision revision;
        AutocheckYamlConfig config;
    }

}
