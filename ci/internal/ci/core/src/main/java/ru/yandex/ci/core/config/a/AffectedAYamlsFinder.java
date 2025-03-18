package ru.yandex.ci.core.config.a;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.function.Consumer;
import java.util.regex.Pattern;

import com.google.common.annotations.VisibleForTesting;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Shared;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.lang.NonNullApi;

@Slf4j
@NonNullApi
public class AffectedAYamlsFinder {

    public static final String CONFIG_FILE_NAME = "a.yaml";

    private static final List<Pattern> EXCLUDED_DIRS = List.of(
            Pattern.compile("^ci/.+?/src/test/resources/.*$"),
            Pattern.compile("^ci/.+?/src/it/resources/.*$"),
            Pattern.compile("^testenv/core/ci_migrate/templates/.*$")
    );
    private static final Path EMPTY_PATH = Path.of("");

    private final ArcService arcService;


    public AffectedAYamlsFinder(ArcService arcService) {
        this.arcService = arcService;
    }


    public AYamlService.AffectedConfigs getAffectedConfigs(ArcRevision revision, ArcRevision previousRevision) {
        ChangeConsumer changeConsumer = new ChangeConsumer();
        arcService.processChanges(revision, previousRevision, changeConsumer);

        Set<String> processedConfigs = new HashSet<>();
        List<AffectedAYaml> aYamls = new ArrayList<>();

        aYamls.addAll(
                addAffectedConfigs(revision, changeConsumer.newConfigs, ConfigChangeType.ADD, processedConfigs)
        );
        aYamls.addAll(
                addAffectedConfigs(revision, changeConsumer.deletedConfigs, ConfigChangeType.DELETE, processedConfigs)
        );
        aYamls.addAll(
                addAffectedConfigs(revision, changeConsumer.modifiedConfigs, ConfigChangeType.MODIFY, processedConfigs)
        );
        aYamls.addAll(
                addAffectedConfigs(
                        revision, changeConsumer.potentialAffectedConfigs, ConfigChangeType.NONE, processedConfigs
                )
        );

        log.info(
                "Got {} affected configs between revisions {}:{}",
                aYamls.size(),
                revision,
                previousRevision
        );
        if (!changeConsumer.affectedYmlNotYaml.isEmpty()) {
            log.info("Got {} affected a.yml configs", changeConsumer.affectedYmlNotYaml.size());
        }

        return AYamlService.AffectedConfigs.of(aYamls, changeConsumer.affectedYmlNotYaml);
    }

    private List<AffectedAYaml> addAffectedConfigs(ArcRevision revision,
                                                   Collection<Path> configPaths,
                                                   ConfigChangeType changeType,
                                                   Set<String> processedConfigs) {
        List<AffectedAYaml> aYamls = new ArrayList<>();
        for (Path path : configPaths) {
            if (!processedConfigs.add(path.toString())) {
                continue;
            }
            if (changeType == ConfigChangeType.NONE && !arcService.isFileExists(path, revision)) {
                continue;
            }
            if (isInExcludedDirs(path)) {
                continue;
            }
            aYamls.add(new AffectedAYaml(path, changeType));
        }
        return aYamls;
    }

    /**
     * Рекурсивно добавляет затронутые a.yaml конфиги
     */
    public static void addPotentialConfigsRecursively(Path startPath, boolean isDir, Collection<Path> target) {
        Path currentDir = isDir ? startPath : startPath.getParent();

        Path root = startPath.getRoot();
        if (EMPTY_PATH.equals(currentDir) || (currentDir != null && currentDir.equals(root))) {
            currentDir = null;
        }

        while (currentDir != null) {
            target.add(currentDir.resolve(CONFIG_FILE_NAME));
            currentDir = currentDir.getParent();
        }

        Path rootAYaml = root != null
                ? root.resolve(CONFIG_FILE_NAME)
                : Path.of(CONFIG_FILE_NAME);
        target.add(rootAYaml);
    }

    static boolean isInExcludedDirs(Path path) {
        for (Pattern excludedDir : EXCLUDED_DIRS) {
            if (excludedDir.matcher(path.toString()).matches()) {
                log.info("Config {} is in excluded directories list: {}", path, excludedDir);
                return true;
            }
        }

        return false;
    }

    static class ChangeConsumer implements Consumer<Repo.ChangelistResponse.Change> {

        private final Set<Path> potentialAffectedConfigs = new HashSet<>();
        private final Set<Path> modifiedConfigs = new HashSet<>();
        private final Set<Path> newConfigs = new HashSet<>();
        private final Set<Path> deletedConfigs = new HashSet<>();
        private final Set<Path> affectedYmlNotYaml = new HashSet<>();

        @VisibleForTesting
        Set<Path> getPotentialAffectedConfigs() {
            return potentialAffectedConfigs;
        }

        @VisibleForTesting
        Set<Path> getModifiedConfigs() {
            return modifiedConfigs;
        }

        @VisibleForTesting
        Set<Path> getNewConfigs() {
            return newConfigs;
        }

        @VisibleForTesting
        Set<Path> getDeletedConfigs() {
            return deletedConfigs;
        }

        @VisibleForTesting
        Set<Path> getAffectedYmlNotYaml() {
            return affectedYmlNotYaml;
        }

        @Override
        public void accept(Repo.ChangelistResponse.Change change) {
            Path path = Path.of(change.getPath());
            if (AYamlService.isAYaml(path)) {
                processAYamlChange(path, change);
            }
            if (AYamlService.isAYmlNotYaml(path)) {
                processYmlNotYamlChange(path, change);
            }

            boolean isDir = change.getType() == Shared.TreeEntryType.TreeEntryDir;
            addPotentialConfigsRecursively(path, isDir, potentialAffectedConfigs);
            if (change.getChange() == Repo.ChangelistResponse.ChangeType.Move) {
                addPotentialConfigsRecursively(Path.of(change.getSource().getPath()), isDir, potentialAffectedConfigs);
            }
        }

        private void processYmlNotYamlChange(Path path, Repo.ChangelistResponse.Change change) {
            switch (change.getChange()) {
                case Add, Copy, Modify, Move -> affectedYmlNotYaml.add(path);
                default -> {
                    // Ignore
                }
            }
        }

        private void processAYamlChange(Path path, Repo.ChangelistResponse.Change change) {
            switch (change.getChange()) {
                case Add, Copy -> newConfigs.add(path);
                case Modify -> modifiedConfigs.add(path);
                case Delete -> deletedConfigs.add(path);
                case Move -> {
                    newConfigs.add(path);
                    deletedConfigs.add(Path.of(change.getSource().getPath()));
                }
                case None, UNRECOGNIZED -> {
                    log.warn("Unsupported change type {} for file {}", change.getChange(), change.getPath());
                }
                default -> log.warn("Unknown change type {} for file {}", change.getChange(), change.getPath());
            }
        }
    }

}
