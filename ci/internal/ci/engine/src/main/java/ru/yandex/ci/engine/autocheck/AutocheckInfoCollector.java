package ru.yandex.ci.engine.autocheck;

import java.nio.file.Path;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.NavigableSet;
import java.util.Set;
import java.util.TreeSet;
import java.util.function.Consumer;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Value;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Shared;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.AutocheckConfig;
import ru.yandex.ci.engine.autocheck.config.AutocheckYamlService;
import ru.yandex.ci.engine.autocheck.model.AutocheckLaunchConfig;
import ru.yandex.ci.engine.pcm.PCMSelector;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;

public class AutocheckInfoCollector implements Consumer<Repo.ChangelistResponse.Change> {
    private static final Logger log = LoggerFactory.getLogger(AutocheckInfoCollector.class);

    private final AutocheckYamlService autocheckYamlService;
    private final AYamlService aYamlService;
    private final FastCircuitTargetsAutoResolver fastTargetResolver;
    private final CommitId leftRevision;
    private final CommitId rightRevision;
    private final PCMSelector pcmSelector;
    private final Set<Path> processedDirs = new LinkedHashSet<>();
    private final Set<String> invalidAYamls = new LinkedHashSet<>();
    private final Set<Path> affectedPaths = new LinkedHashSet<>();
    @Nullable
    private final String checkAuthor;

    private final Set<CheckOuterClass.LargeAutostart> largeTests = new HashSet<>();
    private final Set<CheckOuterClass.NativeBuild> nativeBuilds = new HashSet<>();
    private final NavigableSet<String> fastTargets = new TreeSet<>();
    private boolean sequentialMode = false;

    public AutocheckInfoCollector(AutocheckYamlService autocheckYamlService,
                                  AYamlService aYamlService,
                                  FastCircuitTargetsAutoResolver fastTargetResolver,
                                  CommitId leftRevision,
                                  CommitId rightRevision,
                                  PCMSelector pcmSelector,
                                  @Nullable String checkAuthor) {
        this.autocheckYamlService = autocheckYamlService;
        this.aYamlService = aYamlService;
        this.leftRevision = leftRevision;
        this.rightRevision = rightRevision;
        this.pcmSelector = pcmSelector;
        this.checkAuthor = checkAuthor;
        this.fastTargetResolver = fastTargetResolver;
    }

    @Override
    public void accept(Repo.ChangelistResponse.Change change) {
        Path path = Path.of(change.getPath());
        affectedPaths.add(path);
        boolean isDir = change.getType() == Shared.TreeEntryType.TreeEntryDir;

        processConfigsFromDirToRoot(AYamlService.pathToDir(path, isDir));
        if (change.getChange() == Repo.ChangelistResponse.ChangeType.Move) {
            processConfigsFromDirToRoot(AYamlService.pathToDir(Path.of(change.getSource().getPath()), isDir));
        }
    }

    public AutocheckLaunchConfig getAutocheckLaunchConfig() {
        String poolName = pcmSelector.selectPool(affectedPaths, checkAuthor);
        boolean autoFastTargets = false;
        NavigableSet<String> calculatedFastTargets = fastTargets;
        if (calculatedFastTargets.isEmpty()) {
            calculatedFastTargets = calculateFastTargets();
            autoFastTargets = !calculatedFastTargets.isEmpty();
        }


        var leftConfig = autocheckYamlService.getLastConfigForRevision(leftRevision);
        var rightConfig = autocheckYamlService.getLastConfigForRevision(rightRevision);
        log.info(
                "Left config revision {}, right config revision {}",
                leftConfig.getRevision(), rightConfig.getRevision()
        );

        return new AutocheckLaunchConfig(
                nativeBuilds,
                largeTests,
                CheckOuterClass.LargeConfig.getDefaultInstance(), // No default large configuration
                AutocheckConstants.FULL_CIRCUIT_TARGETS,
                calculatedFastTargets,
                sequentialMode,
                invalidAYamls,
                poolName,
                autoFastTargets,
                CheckIteration.StrongModePolicy.BY_A_YAML,
                leftConfig,
                rightConfig
        );
    }

    private void processConfigsFromDirToRoot(Path startDir) {
        Path aYamlDir = startDir;
        boolean skipFastTargets = false;
        while (aYamlDir != null) {
            if (processedDirs.contains(aYamlDir)) {
                return;
            }

            if (!skipFastTargets) {
                processedDirs.add(aYamlDir);
            }
            Path aYamlPath = AYamlService.dirToConfigPath(aYamlDir);

            if (!aYamlService.isFileNotFound(rightRevision, aYamlPath)) {
                skipFastTargets = processAYaml(aYamlPath, skipFastTargets);
            }

            aYamlDir = aYamlDir.getParent();
        }
    }

    /**
     * Process a.yaml file
     *
     * @param aYamlPath       path to yaml file
     * @param skipFastTargets flag defines should fast targets be processed
     * @return updated skipFastTargets flag, true if fast targets must be skipped since this iteration
     */
    private boolean processAYaml(Path aYamlPath, boolean skipFastTargets) {
        try {
            AYamlConfig aYamlConfig = aYamlService.getConfig(aYamlPath, rightRevision);

            if (aYamlConfig.getCi() == null || aYamlConfig.getCi().getAutocheck() == null) {
                return skipFastTargets;
            }

            AutocheckConfig autocheckConfig = aYamlConfig.getCi().getAutocheck();

            processNativeBuildConfigs(autocheckConfig, aYamlPath);
            processLargeTestConfigs(autocheckConfig, aYamlPath);
            if (!skipFastTargets) {
                skipFastTargets = processFastTargets(autocheckConfig);
            }
        } catch (Exception e) {
            log.warn("Config {} is invalid", aYamlPath, e);
            invalidAYamls.add(aYamlPath.toString());

            return true;
        }

        return skipFastTargets;
    }

    private boolean processFastTargets(AutocheckConfig autocheckConfig) {
        if (autocheckConfig.getFastTargets() == null) {
            return false;
        }

        if (autocheckConfig.getFastTargets().isEmpty()) {
            return true;
        }

        fastTargets.addAll(autocheckConfig.getFastTargets());
        // We ignore autocheckConfig.getFastMode() since we don't support sequential mode anymore

        return true;
    }

    @Nonnull
    NavigableSet<String> calculateFastTargets() {
        return fastTargetResolver.getFastTarget(processedDirs)
                .map(path -> (NavigableSet<String>) new TreeSet<>(Set.of(path)))
                .orElse(fastTargets);
    }

    private void processNativeBuildConfigs(AutocheckConfig autocheckConfig, Path aYamlPath) {
        for (var config : autocheckConfig.getNativeBuilds()) {
            nativeBuilds.add(ProtoMappers.toProtoNativeBuild(aYamlPath, config));
        }
    }

    private void processLargeTestConfigs(AutocheckConfig autocheckConfig, Path aYamlPath) {
        for (var config : autocheckConfig.getLargeAutostart()) {
            largeTests.add(ProtoMappers.toProtoLargeAutostart(aYamlPath, config));
        }
    }

    @Value
    @AllArgsConstructor
    public static class ToolchainAndPath {
        String toolchain;
        String path;
    }

}
