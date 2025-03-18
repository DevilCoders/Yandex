package ru.yandex.ci.engine.discovery;

import java.nio.file.Path;
import java.nio.file.PathMatcher;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.util.CommitPathFetcherMemoized;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.a.AffectedAYaml;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.model.TriggerConfig;
import ru.yandex.ci.core.config.a.validation.AYamlStaticValidator;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePostCommits;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.discovery.util.ConfigStates;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.util.GlobMatchers;

@Slf4j
@AllArgsConstructor
public class DiscoveryServicePostCommits {

    @Nonnull
    private final AffectedAYamlsFinder affectedAYamlsFinder;

    @Nonnull
    private final ArcService arcService;

    @Nonnull
    private final BranchService branchService;

    @Nonnull
    private final CiMainDb db;

    @Nonnull
    private final ConfigurationService configurationService;

    @Nonnull
    private final DiscoveryServicePostCommitTriggers triggers;

    @Nonnull
    private final DiscoveryServiceProcessor processor;

    @Nonnull
    private final AutocheckBootstrapServicePostCommits autocheckBootstrapServicePostCommits;

    @Nonnull
    private final DiscoveryProgressService discoveryProgressService;

    @Nonnull
    private final RevisionNumberService revisionNumberService;

    private final boolean autocheckPostcommitsEnabled;

    public void processPostCommit(ArcBranch branch, ArcRevision revision) {
        processPostCommit(branch, revision, autocheckPostcommitsEnabled);
    }

    public void processPostCommit(ArcBranch branch, ArcRevision revision, boolean enableAutocheck) {
        if (branch.getType() == ArcBranch.Type.TRUNK || branch.getType() == ArcBranch.Type.RELEASE_BRANCH) {
            log.info("Accept discovering post commit changes for {} at {}", branch, revision);
            var orderedRevision = revisionNumberService.getOrderedArcRevision(branch, revision);
            processPostCommitConfig(branch, orderedRevision, enableAutocheck);
            discoveryProgressService.markAsDiscovered(orderedRevision, DiscoveryType.DIR);
        } else {
            log.info("Discovering post commit changes for {} at {} skipped", branch, revision);
        }
    }

    private void processPostCommitConfig(ArcBranch origBranch, OrderedArcRevision revision, boolean enableAutocheck) {
        var commit = arcService.getCommit(revision);
        var previousRevision = ArcCommitUtils.firstParentArcRevision(commit)
                .orElseThrow(() -> new IllegalArgumentException(
                        "commit %s should have at least one parent".formatted(revision)
                ));
        log.info("Discovering post commit changes for revision {}, previous {} (original branch {})",
                revision, previousRevision, origBranch);

        if (enableAutocheck) {
            autocheckBootstrapServicePostCommits.runAutocheckIfRequired(origBranch, revision, commit);
        }

        var affectedConfigs = affectedAYamlsFinder.getAffectedConfigs(revision.toRevision(), previousRevision);

        if (revision.getBranch().isRelease()) {
            var affectedByBranch = db.currentOrTx(() -> {
                        var affectedProcesses = branchService.getProcessesForBranch(revision.getBranch());

                        branchService.addProcessedCommitToBranchItemStats(
                                affectedProcesses.stream()
                                        .map(process -> TimelineBranchItem.Id.of(process, revision.getBranch()))
                                        .collect(Collectors.toSet())
                        );

                        return affectedProcesses;
                    })
                    .stream()
                    .peek(process -> log.info("Process {} affected by commit, because commit is in branch", process))
                    .map(process -> new AffectedAYaml(process.getPath(), ConfigChangeType.NONE))
                    .collect(Collectors.toList());

            affectedConfigs = affectedConfigs.merge(affectedByBranch);
        }

        new YamlCommitProcessor(origBranch, commit, affectedConfigs.getYamls(), revision, previousRevision)
                .processAll();
    }

    public void processTriggeredProcesses(DiscoveryContext context, TriggeredProcesses processes) {
        processor.processTriggeredProcesses(context, processes);
    }

    public TriggeredProcesses findTriggeredProcesses(DiscoveryContext context, CiConfig ciConfig) {
        return triggers.findTriggeredProcesses(context, ciConfig);
    }

    @RequiredArgsConstructor
    class YamlCommitProcessor {

        @Nonnull
        final ArcBranch origBranch;

        @Nonnull
        final ArcCommit commit;

        @Nonnull
        final List<AffectedAYaml> yamls;

        @Nonnull
        final OrderedArcRevision revision;

        @Nonnull
        final ArcRevision previousRevision;

        void processAll() {
            var commitPaths = fetchCommitPaths();
            var discoveredByAbsPath = collectAYamlsAffectedByAbsPaths(commitPaths);

            for (var affectedAYaml : yamls) {
                var path = affectedAYaml.getPath().toString();
                if (discoveredByAbsPath.discoveredAYAml.remove(path)) {
                    log.info("{} works as both as dir-discovery and default", path);
                }
                processYaml(affectedAYaml, false, commitPaths, null);
            }
            log.info("All {} affected configs processed for revision {}", yamls.size(), revision);

            for (var config : discoveredByAbsPath.discoveredAYAml) {
                var yaml = new AffectedAYaml(Path.of(config), ConfigChangeType.NONE);
                Preconditions.checkState(yaml.getChangeType() == ConfigChangeType.NONE,
                        "Make sure change type is none, got %s", yaml.getChangeType());
                // Discovery-dir must accept all changes from Arcadia root but restrict filter by discovery dir

                // Разрешаем применять фильтр, только если там есть условие по dir discovery, которое и позволило
                // ему попасть в текущую выборку.
                // Сохранять конкретный action/release видится опасным - у нас нет привязки конфига к ревизии.
                processYaml(
                        yaml,
                        true,
                        commitPaths,
                        filter -> !filter.getAbsPaths().isEmpty() &&
                                (filter.getDiscovery() == FilterConfig.Discovery.ANY ||
                                        filter.getDiscovery() == FilterConfig.Discovery.DIR ||
                                        filter.getDiscovery() == FilterConfig.Discovery.DEFAULT)
                );
            }
            log.info("All {} additional affected configs from discovery-dirs processed for revision {}",
                    discoveredByAbsPath.discoveredAYAml.size(), revision);
        }

        private void processYaml(AffectedAYaml aYaml,
                                 boolean allowEmptyConfigs,
                                 List<String> commitPaths,
                                 @Nullable Predicate<FilterConfig> filterPredicate) {

            log.info("Processing post commit config {}. Config change type {}", aYaml.getPath(), aYaml.getChangeType());

            boolean isTrunk = revision.getBranch().isTrunk();
            if (!aYaml.getChangeType().isExisting()) {
                if (isTrunk) {
                    new YamlProcessorCleanup(aYaml, commit, revision).updateDeletedConfigState();
                    log.info("Config {} doesn't exist any more", aYaml.getPath());
                } else {
                    log.info("Config {} doesn't exist in branch {} any more. Stop processing commit",
                            aYaml.getPath(), revision.getBranch());
                }
            } else {
                var currentConfigOpt = configurationService.getOrCreateConfig(aYaml.getPath(), revision);
                if (currentConfigOpt.isEmpty()) {
                    if (allowEmptyConfigs) {
                        log.warn("No configuration found for {} at revision {}", aYaml.getPath(), revision);
                        return; // --- Continue anyway, this is a dir discovery issue
                    } else {
                        throw new RuntimeException("Unable to find configuration for " + aYaml.getPath() +
                                " at " + revision);
                    }
                }
                ConfigBundle currentConfig = currentConfigOpt.get();
                ConfigBundle actualConfig = configurationService.getLastActualConfig(currentConfig);
                new YamlProcessor(aYaml, commit, revision, previousRevision, currentConfig, actualConfig,
                        commitPaths, filterPredicate).process();
            }
        }

        private DiscoveryDirsResult collectAYamlsAffectedByAbsPaths(List<String> commitPaths) {
            if (!origBranch.isTrunk()) {
                log.info("Skip discovery dirs, original branch is {}, current branch is {}",
                        origBranch, revision.getBranch());
                return new DiscoveryDirsResult(List.of()); // ---
            }

            log.info("Try processing configs by discovery dirs...");

            if (commitPaths.isEmpty()) {
                return new DiscoveryDirsResult(List.of()); // ---
            }

            var configs = db.currentOrReadOnly(() ->
                    db.configDiscoveryDirs().lookupAffectedConfigs(Set.copyOf(commitPaths)));
            log.info("Configs to process: {}", configs);
            return new DiscoveryDirsResult(configs);
        }

        private List<String> fetchCommitPaths() {
            return new CommitPathFetcherMemoized(arcService, revision).get();
        }

    }

    @Value
    static class DiscoveryDirsResult {
        Set<String> discoveredAYAml;

        private DiscoveryDirsResult(@Nonnull List<String> discoveredAYAml) {
            this.discoveredAYAml = new HashSet<>(discoveredAYAml);
        }
    }

    @AllArgsConstructor
    class YamlProcessorCleanup {

        @Nonnull
        private final AffectedAYaml aYaml;

        @Nonnull
        private final ArcCommit commit;

        @Nonnull
        private final OrderedArcRevision revision;

        private void updateDeletedConfigState() {
            Preconditions.checkArgument(revision.getBranch().isTrunk(),
                    "cannot delete config from branch {}, only from trunk",
                    revision.getBranch()
            );
            ConfigState deletedState = ConfigState.builder()
                    .configPath(aYaml.getPath())
                    .created(commit.getCreateTime())
                    .updated(commit.getCreateTime())
                    .lastRevision(revision)
                    .status(ConfigState.Status.DELETED)
                    .build();

            db.currentOrTx(() -> {
                db.configStates().upsertIfNewer(deletedState);
                db.configDiscoveryDirs().deleteAllPath(revision, aYaml.getPath());
            });
        }
    }

    @AllArgsConstructor
    class YamlProcessor {

        @Nonnull
        private final AffectedAYaml aYaml;

        @Nonnull
        private final ArcCommit commit;

        @Nonnull
        private final OrderedArcRevision revision;

        @Nonnull
        private final ArcRevision previousRevision;

        // Current (i.e. new) configuration, we trying to update right now
        @Nonnull
        private final ConfigBundle currentConfig;

        // Last valid configuration, maybe current or maybe previous (if current configuration is not valid)
        @Nonnull
        private final ConfigBundle actualConfig;

        @Nonnull
        private final List<String> commitPaths;

        @Nullable
        private final Predicate<FilterConfig> filterPredicate;

        void process() {
            if (revision.getBranch().isTrunk() && aYaml.getChangeType() != ConfigChangeType.NONE) {
                updateConfigState();
            }
            processPostCommitConfig();
        }

        private void processPostCommitConfig() {
            if (!actualConfig.getStatus().isValidCiConfig()) {
                log.info("Config {} not in valid state ({}), skipping.", aYaml.getPath(), actualConfig.getStatus());
                return;
            }
            CiConfig ciConfig = actualConfig.getValidAYamlConfig().getCi();
            DiscoveryContext context = createContext(actualConfig);

            var triggeredProcesses = triggers.findTriggeredProcesses(context, ciConfig);
            processor.processTriggeredProcesses(context, triggeredProcesses);
            log.info("Processed config {}", aYaml.getPath());
        }

        private void updateConfigState() {
            Preconditions.checkArgument(commit.isTrunk(),
                    "cannot update config state {} from branch, only from trunk",
                    currentConfig.getConfigPath()
            );
            var configStateBuilder = ConfigStates.prepareConfigState(actualConfig, commit);

            configStateBuilder.lastRevision(currentConfig.getRevision());

            if (currentConfig.getStatus().isValidCiConfig()) {
                configStateBuilder.status(ConfigState.Status.OK);
            } else if (currentConfig.getStatus() == ConfigStatus.NOT_CI) {
                configStateBuilder.status(ConfigState.Status.NOT_CI);
            } else if (actualConfig.getStatus().isValidCiConfig()) {
                configStateBuilder.status(ConfigState.Status.INVALID_PREVIOUS_VALID);
            } else {
                configStateBuilder.status(ConfigState.Status.INVALID);
            }

            var state = configStateBuilder.build();
            var pathPrefixes = collectPathPrefix(state);
            log.info("Discovery-dir paths prefixes: {}", pathPrefixes);
            db.currentOrTx(() -> {
                var updated = db.configStates().upsertIfNewer(state);
                if (updated) {
                    pathPrefixes.ifPresent(values ->
                            db.configDiscoveryDirs().updateAllPaths(
                                    revision, aYaml.getPath(), values, action -> action.accept(db)));
                }
            });
        }

        private Optional<Collection<String>> collectPathPrefix(ConfigState state) {
            return switch (state.getStatus()) {
                case DELETED, INVALID, NOT_CI, DRAFT -> Optional.of(Set.of()); // No data
                case INVALID_PREVIOUS_VALID -> Optional.empty(); // No need to update anything
                case OK -> {
                    var prefixes = new LinkedHashSet<String>();
                    var ciConfig = actualConfig.getValidAYamlConfig().getCi();
                    for (ActionConfig actionConfig : ciConfig.getMergedActions().values()) {
                        for (TriggerConfig trigger : actionConfig.getTriggers()) {
                            collectDirDiscovery(trigger.getFilters(), prefixes);
                        }
                    }
                    for (ReleaseConfig release : ciConfig.getReleases().values()) {
                        collectDirDiscovery(release.getFilters(), prefixes);
                    }
                    yield Optional.of(prefixes);
                }
            };
        }

        private void collectDirDiscovery(List<FilterConfig> filters, Set<String> paths) {
            for (var filter : filters) {
                DiscoveryServicePostCommits.collectAbsPathPrefixes(filter, paths);
            }
        }

        private DiscoveryContext createContext(ConfigBundle configBundle) {
            return DiscoveryContext.builder()
                    .revision(revision)
                    .previousRevision(previousRevision)
                    .commit(commit)
                    .configChange(aYaml.getChangeType())
                    .configBundle(configBundle)
                    .filterPredicate(filterPredicate)
                    .discoveryType(DiscoveryType.DIR)
                    .affectedPaths(commitPaths)
                    .build();
        }
    }


    static void collectAbsPathPrefixes(FilterConfig filter, Set<String> paths) {
        List<PathMatcher> pathExclusions = filter.getNotAbsPaths().stream()
                .map(GlobMatchers::getGlobMatcher)
                .toList();
        for (var path : filter.getAbsPaths()) {
            var globChar = StringUtils.indexOfAny(path, AYamlStaticValidator.GLOB_CHARS);

            boolean parent;
            if (globChar >= 0) {
                path = path.substring(0, globChar);
                parent = true;
            } else {
                parent = false;
            }
            path = path.trim();
            if (path.length() > 0) {
                var endsWithSlash = path.endsWith("/");
                var pathObject = Path.of(path); // Resolve all possible
                if (parent && !endsWithSlash) {
                    pathObject = pathObject.getParent();
                }
                if (pathObject != null) {
                    path = pathObject.toString();
                    if (path.length() > 0) {
                        var finalPathObj = pathObject;
                        if (pathExclusions.stream().noneMatch(matcher -> matcher.matches(finalPathObj))) {
                            paths.add(path);
                        }
                    }
                }
            }
        }
    }

}
