package ru.yandex.ci.engine.config;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.base.Splitter;
import com.google.common.collect.Lists;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.branch.BranchYamlParser;
import ru.yandex.ci.core.config.branch.model.BranchYamlConfig;
import ru.yandex.ci.core.config.branch.validation.BranchYamlValidationReport;
import ru.yandex.misc.lang.StringUtils;

@Slf4j
@RequiredArgsConstructor
public class BranchYamlService {
    private static final Path CONFIG_BASE_PATH = Path.of("config/branches");

    @Nonnull
    private final ArcService arcService;

    @Nonnull
    private final ConfigurationService configurationService;

    public Optional<BranchConfigBundle> findBranchConfigWithAutocheckSection(ArcBranch branch) {
        Preconditions.checkArgument(branch.isRelease(), "Only release branches supported. Got: %s", branch);
        ArcRevision revision = arcService.getLastRevisionInBranch(ArcBranch.trunk());
        log.info("Looking for config for branch {} in trunk on revision {}", branch, revision);

        var possiblePath = getPossibleConfigPaths(branch);
        for (Path path : possiblePath) {
            log.info("Looking for config in path {}", path);
            var configBundleOptional = getConfigBundle(revision, path);
            if (configBundleOptional.isEmpty()) {
                log.info("No config in path {}", path);
                continue;
            }
            var configBundle = configBundleOptional.get();
            if (!configBundle.isValid()) {
                log.warn(
                        "Found invalid config {}. Stopping config processing. Problems: {}",
                        path, configBundle.getProblems()
                );
                return configBundleOptional;
            }
            if (!configBundle.hasAutocheckSection()) {
                log.info("Config {} without autocheck section. Skipping...", path);
                continue;
            }
            log.info("Found suitable config {}", path);
            return configBundleOptional;
        }

        log.info("No branch config found for branch {}. Lookup was on trunk revision {}.", branch, revision);
        return Optional.empty();
    }


    Optional<BranchConfigBundle> getConfigBundle(ArcRevision revision, Path path) {
        Optional<String> content = arcService.getFileContent(path, revision);
        if (content.isEmpty()) {
            return Optional.empty();
        }
        try {
            var report = BranchYamlParser.parseAndValidate(content.get());
            var problems = Stream.of(report.getSchemaReportMessages(), report.getStaticErrors())
                    .flatMap(Collection::stream)
                    .map(ConfigProblem::crit)
                    .collect(Collectors.toList());

            var delegatedConfig = lookupDelegatedConfig(report, problems);
            return Optional.of(BranchConfigBundle.forParsed(revision, path, report.getConfig(),
                    delegatedConfig, problems));
        } catch (Exception e) {
            return Optional.of(BranchConfigBundle.forException(revision, path, e));
        }
    }

    @Nullable
    private ConfigBundle lookupDelegatedConfig(BranchYamlValidationReport report, List<ConfigProblem> problems) {
        BranchYamlConfig config = report.getConfig();
        if (config == null || config.getCi() == null ||
                StringUtils.isEmpty(config.getCi().getDelegatedConfig())) {
            return null;
        }
        var configPath = config.getCi().getDelegatedConfig();
        var lastDelegatedConfig = configurationService.findLastValidConfig(Path.of(configPath), ArcBranch.trunk());
        if (lastDelegatedConfig.isEmpty()) {
            problems.add(ConfigProblem.crit("Unable to find valid config: %s"
                    .formatted(configPath)));
            return null;
        }

        var delegatedConfig = lastDelegatedConfig.get();
        var delegatedService = delegatedConfig.getValidAYamlConfig().getService();
        if (!Objects.equals(config.getService(), delegatedService)) {
            problems.add(ConfigProblem.crit("Branch service [%s] must be same as delegated service [%s]"
                    .formatted(config.getService(), delegatedService)));
            return null;
        }

        return lastDelegatedConfig.get();
    }

    /**
     * Возвращает возможные конфиги для бранча в порядке приоритета
     */
    @VisibleForTesting
    List<Path> getPossibleConfigPaths(ArcBranch branch) {
        Preconditions.checkArgument(branch.isRelease(), "Only release branches supported. Got: %s", branch);
        List<String> branchPathParts = Splitter.on('/').splitToList(branch.asString());
        List<Path> possiblePaths = new ArrayList<>();

        Path basePath = CONFIG_BASE_PATH;
        for (int i = 0; i < branchPathParts.size(); i++) {
            String part = branchPathParts.get(i);
            if (i > 0) { //Skipping first (releases) level
                possiblePaths.add(basePath.resolve(part + ".yaml"));
            }
            basePath = basePath.resolve(part);
        }

        return Lists.reverse(possiblePaths);
    }
}
