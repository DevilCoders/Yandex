package ru.yandex.ci.engine.config;

import java.nio.file.Path;
import java.time.Clock;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.MapDifference;
import com.google.common.collect.Maps;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.arcanum.ArcanumReviewDataDto;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcCommitWithPath;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.ConfigCreationInfo;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigPermissions;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.a.model.Permissions;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.core.security.PermissionsProcessor;
import ru.yandex.ci.engine.branch.BranchTraverseService;
import ru.yandex.ci.engine.flow.SecurityStateService;

@Slf4j
@RequiredArgsConstructor
public class ConfigurationService {

    @Nonnull
    private final SecurityStateService securityStateService;
    @Nonnull
    private final ArcService arcService;
    @Nonnull
    private final ConfigParseService configParseService;
    @Nonnull
    private final ArcanumClientImpl arcanumClient;
    @Nonnull
    private final RevisionNumberService revisionNumberService;
    @Nonnull
    private final BranchTraverseService branchTraverseService;
    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final Clock clock;

    public Optional<ConfigBundle> findConfig(Path configPath, OrderedArcRevision revision) {
        log.info("Looking for configuration {} on revision {}", configPath, revision);
        return db.currentOrReadOnly(() -> db.configHistory().findById(configPath, revision))
                .map(this::createBundle);
    }

    public ConfigBundle getConfig(Path configPath, OrderedArcRevision revision) {
        return findConfig(configPath, revision).orElseThrow(() ->
                new NoSuchElementException(String.format("Unable to find configuration for %s on revision %s",
                        configPath, revision)));
    }

    private Optional<ConfigBundle> findLastConfig(Path configPath, ArcBranch branch) {
        return db.currentOrReadOnly(() -> {
            log.info("Looking for last configuration {} in branch {}", configPath, branch);

            var entityOpt = db.configHistory().findLastConfig(configPath, branch);
            if (entityOpt.isEmpty()) {
                log.info("No configuration at all for {} in branch {}", configPath, branch);
                return Optional.empty();
            }

            var entity = entityOpt.get();
            log.info("Last configuration {} in branch {} found on rev {}", configPath, branch, entity.getRevision());

            return Optional.of(createBundle(entity));
        });
    }

    public ConfigBundle getLastConfig(Path configPath, ArcBranch branch) {
        return findLastConfig(configPath, branch).orElseThrow(() ->
                new NoSuchElementException(String.format("Unable to find last configuration for %s at %s",
                        configPath, branch)));
    }

    public Optional<ConfigBundle> findLastValidConfig(Path configPath, ArcBranch branch) {
        return db.currentOrReadOnly(() -> {
            log.info("Looking for last valid configuration {} in branch {}", configPath, branch);

            Optional<ConfigEntity> entityOptional = db.configHistory().findLastConfig(configPath, branch);
            if (entityOptional.isEmpty()) {

                if (branch.isTrunk()) {
                    log.info("No configuration at all for {} in trunk", configPath);
                    return Optional.empty();
                } else if (branch.isRelease()) {
                    log.info("No configuration at all for {} in branch {}, looking at trunk", configPath, branch);
                    var baseRevision = branchTraverseService.getBaseRevision(branch);

                    entityOptional = db.configHistory().findLastConfig(
                            configPath, ArcBranch.trunk(), baseRevision.getNumber());

                    if (entityOptional.isEmpty()) {
                        log.info("Not configuration for {} in trunk since revision {}",
                                configPath, baseRevision.getNumber());
                        return Optional.empty();
                    }
                } else {
                    log.info("No configuration {} found in branch {}", configPath, branch);
                    return Optional.empty();
                }
            }

            ConfigEntity entity = entityOptional.get();
            if (!entity.getStatus().isValidCiConfig()) {
                if (entity.getPreviousValidRevision().isEmpty()) {
                    log.info("Last configuration {} in branch {} rev {} is invalid, no valid configuration present",
                            configPath, branch, entity.getRevision());
                    return Optional.empty();
                }
                entity = db.configHistory().getById(configPath, entity.getPreviousValidRevision().get());
            }

            Preconditions.checkState(entity.getStatus().isValidCiConfig());

            log.info("Last valid configuration {} in branch {} found on rev {}",
                    configPath, branch, entity.getRevision());

            return Optional.of(createBundle(entity));
        });
    }

    public ConfigBundle getLastValidConfig(Path configPath, ArcBranch branch) {
        return findLastValidConfig(configPath, branch).orElseThrow(() ->
                new NoSuchElementException(String.format("Unable to find last valid configuration for %s at %s",
                        configPath, branch)));
    }

    public Optional<ConfigBundle> getLastReadyOrNotCiConfig(Path configPath, OrderedArcRevision maxRevision) {
        return db.currentOrReadOnly(() -> {
            log.info("Looking for last valid configuration {} with max revision {}", configPath, maxRevision);
            var configOpt = db.configHistory().findLastConfig(
                    configPath,
                    maxRevision,
                    ConfigStatus.READY,
                    ConfigStatus.NOT_CI
            );

            if (configOpt.isEmpty()) {
                log.info("No configuration found");
                return Optional.empty();
            }
            var config = configOpt.get();
            log.info("Last {} found on rev {}", config.getStatus(), config.getRevision());
            return switch (config.getStatus()) {
                case READY -> Optional.of(createBundle(config));
                case NOT_CI -> Optional.empty();
                default -> throw new IllegalStateException("Internal error. Unsupported status " + config.getStatus());
            };
        });
    }

    public Optional<ConfigBundle> getOrCreateConfig(Path configPath, OrderedArcRevision revision) {
        return switch (revision.getBranch().getType()) {
            case RELEASE_BRANCH, TRUNK -> getOrCreateBranchConfig(configPath, revision);
            case PR -> Optional.of(getOrCreatePrConfig(configPath, revision).getConfig());
            case USER_BRANCH, GROUP_BRANCH, UNKNOWN -> throw new IllegalStateException("Unsupported type: " +
                    revision.getBranch().getType());
            default -> throw new IllegalStateException("Unknown type: " + revision.getBranch().getType());
        };
    }

    public ConfigBundle getLastActualConfig(ConfigBundle bundle) {
        var entity = bundle.getConfigEntity();
        if (entity.getStatus().isValidCiConfig()) {
            return bundle;
        }

        var previousValidRevision = entity.getPreviousValidRevision().orElse(null);
        if (previousValidRevision != null) {
            log.info("Configuration {} invalid on revision {}. Using configuration on last valid revision {}",
                    entity.getConfigPath(), entity.getRevision(), previousValidRevision);
            return getConfig(entity.getConfigPath(), previousValidRevision);
        }

        log.info(
                "Configuration {} invalid on revision {}. No previous valid configs found.",
                entity.getConfigPath(), entity.getRevision()
        );

        return bundle;
    }

    /**
     * Если в транке есть конфиг производим сравнение в первую очередь с ним.
     * Предыдущую итерацию считаем доп бонусом, из которой могут быть взяты токены или апрувы.
     * Если было изменение конфига в ревью, новую версию создаём всегда.
     */
    public PullRequestConfigInfo getOrCreatePrConfig(Path configPath, OrderedArcRevision revision) {
        var branch = revision.getBranch();
        Preconditions.checkState(branch.isPr(), "Not pr revisions %s", revision);

        return db.currentOrTx(() -> {
            /*
             * TODO здесь на самом деле правильнее пробежаться по итерациям.
             * Иначе может быть гонка между итерациями PR.
             * Как альтернатива - гарантировать последовательную обработку итераций,
             * что сложнее и мы нигде так не делаем.
             */
            ConfigBundle previousPrConfig = db.configHistory().findLastConfig(configPath, revision)
                    .map(this::createBundle)
                    .orElse(null);

            var diffSet = db.pullRequestDiffSetTable().getById(branch.getPullRequestId(), revision.getNumber());
            var vcsInfo = diffSet.getVcsInfo();
            var upstreamBranch = vcsInfo.getUpstreamBranch();
            var upstreamConfig = getOrCreateBranchConfig(configPath, upstreamBranch, vcsInfo.getUpstreamRevision())
                    .orElse(null);

            if (previousPrConfig != null && previousPrConfig.getRevision().equals(revision)) {
                // возвращаем конфиг из пулл реквеста
                return PullRequestConfigInfo.of(previousPrConfig, upstreamConfig);
            }

            ArcCommit commit = arcService.getCommit(revision);
            if (commit.getAuthor().equals("robot-arcanum")) {
                //TODO ARC-2668 Для PRов созданных из UI арканума или тех,
                // на которых был rebase при синке SVN-ARC автором становиться robot-arcanum.
                // В таких случаях явно перетираем автором ревью, чтобы валидация не сходила с ума.
                commit = commit.withAuthor(diffSet.getAuthor());
            }

            ArcCommit lastConfigCommit = arcService.getLastCommit(configPath, revision)
                    .orElseThrow();

            if (lastConfigCommit.isTrunk()) {
                Preconditions.checkState(upstreamConfig != null,
                        "Last change in %s, but no configuration found", upstreamBranch);

                //Последний конфиг менялся в транке остаётся проверить только версии тасок
                var configBundle = createConfigIfTaskRevisionChanged(configPath, commit, revision, upstreamConfig,
                        previousPrConfig);
                return PullRequestConfigInfo.of(configBundle, upstreamConfig);
            }

            //Изменения были в ПРе (в предыдущих итерациях). В таком случае всегда создаём конфиг.
            var configBundle = createConfig(configPath, commit, revision, upstreamConfig, previousPrConfig);
            return PullRequestConfigInfo.of(configBundle, upstreamConfig);
        });
    }

    private Optional<ConfigBundle> getOrCreateBranchConfig(Path configPath, OrderedArcRevision orderedRevision) {
        return getOrCreateBranchConfig(configPath, arcService.getCommit(orderedRevision), orderedRevision);
    }

    private Optional<ConfigBundle> getOrCreateBranchConfig(Path configPath, ArcBranch branch, ArcRevision revision) {
        OrderedArcRevision orderedRevision = revisionNumberService.getOrderedArcRevision(branch, revision);
        return getOrCreateBranchConfig(configPath, orderedRevision);
    }

    public Optional<ConfigBundle> getOrCreateBranchConfig(Path configPath,
                                                          ArcCommit commit,
                                                          OrderedArcRevision revision) {
        ArcBranch upstreamBranch = revision.getBranch();
        Preconditions.checkState(upstreamBranch.isTrunk() || upstreamBranch.isRelease());

        log.info("Looking for configuration {} at {}", configPath, revision);
        ArcCommit lastChangeCommit = arcService.getLastCommit(configPath, commit).orElse(null);

        if (lastChangeCommit == null) {
            log.info("Configuration {} does not exist on revision {}", configPath, commit.getCommitId());
            return Optional.empty();
        }

        return db.currentOrTx(() -> {
            Optional<ConfigBundle> lastConfigInDatabase = db.configHistory().findLastConfig(configPath, revision)
                    .map(this::createBundle);

            if (lastConfigInDatabase.isPresent()) {
                OrderedArcRevision configRevision = lastConfigInDatabase.get().getRevision();
                log.info("Last known existing configuration {} on revision {}", configPath, configRevision);
                if (configRevision.equals(revision)) {
                    return lastConfigInDatabase;
                }

                OrderedArcRevision lastChangedRevision = revisionNumberService.getOrderedArcRevision(
                        upstreamBranch, lastChangeCommit
                );

                if (isBeforeOrSame(lastChangedRevision, configRevision)
                        || (lastChangedRevision.getBranch().isTrunk() && !configRevision.getBranch().isTrunk())
                ) {
                    // Мы уже ранее обработали все изменения конфига, осталось проверить, что не изменились таски.
                    return Optional.of(
                            createConfigIfTaskRevisionChanged(
                                    configPath,
                                    commit,
                                    revision,
                                    lastConfigInDatabase.get(),
                                    getPullRequestConfigChange(configPath, commit)
                            )
                    );
                }
            }

            ConfigBundle configBundle = doGetOrCreateConfigRecursive(configPath, lastChangeCommit, upstreamBranch);
            return Optional.of(
                    createConfigIfTaskRevisionChanged(
                            configPath,
                            commit,
                            revision,
                            configBundle,
                            getPullRequestConfigChange(configPath, commit)
                    )
            );
        });
    }

    private static boolean isBeforeOrSame(OrderedArcRevision rev2, OrderedArcRevision rev3) {
        return rev2.getBranch().equals(rev3.getBranch()) && rev2.isBeforeOrSame(rev3);
    }

    private ConfigBundle createConfigIfTaskRevisionChanged(
            Path configPath,
            ArcCommit commit,
            OrderedArcRevision revision,
            ConfigBundle configBundle,
            @Nullable ConfigBundle previousPullRequestConfig
    ) {
        if (configBundle.getRevision().getCommitId().equals(revision.getCommitId())) {
            return configBundle;
        }
        if (!configBundle.getStatus().isValidCiConfig()) {
            log.info(
                    "Configuration {} on revision {} invalid. Skipping tasks revision check",
                    configBundle.getConfigPath(),
                    configBundle.getRevision()
            );
            return configBundle;
        }
        var taskChanges = getTaskRevisionChanges(configBundle, revision.toRevision());
        if (taskChanges.areEqual()) {
            log.info("Tasks have not been updated between revision {} and {}", configBundle.getRevision(), revision);
            return configBundle;
        }

        log.info(
                "Updating configuration {} from revision {} to {} because of tasks revision change: {}",
                configBundle.getConfigPath(),
                configBundle.getRevision(),
                revision,
                taskChanges
        );
        return createConfig(configPath, commit, revision, configBundle, previousPullRequestConfig);
    }

    /**
     * Проходим по истории назад, создавая по пути конфигурации на каждое изменение a.yaml.
     *
     * @param configPath    путь к a.yaml
     * @param commit        коммит, от которого начинается процедура
     * @param currentBranch текущая ветка. Может отличаться от транка
     * @return версия конфига на заданном коммите.
     */
    private ConfigBundle doGetOrCreateConfigRecursive(Path configPath, ArcCommit commit, ArcBranch currentBranch) {
        log.info("Getting configuration {} at {} in branch {}", configPath, commit.getRevision(), currentBranch);

        Optional<ConfigBundle> existingBundle = findConfig(
                configPath, revisionNumberService.getOrderedArcRevision(currentBranch, commit)
        );
        if (existingBundle.isPresent()) {
            log.info("Found existing");
            return existingBundle.get();
        }

        Optional<ArcCommit> previousChangeCommit = getPreviousChangeCommit(configPath, commit.getRevision());
        if (previousChangeCommit.isPresent()) {
            log.info(
                    "Previous change for configuration {} on revision {} is {}",
                    configPath,
                    commit.getCommitId(),
                    previousChangeCommit.get().getCommitId()
            );

            if (commit.isTrunk() && !previousChangeCommit.get().isTrunk()) {
                log.info("Current commit {} is trunk (svn revision = {}), but previous {} is not." +
                                " Treat current commit as first for config. https://st.yandex-team.ru/CI-1762",
                        commit.getCommitId(), commit.getRevision(), previousChangeCommit.get()
                );
                previousChangeCommit = Optional.empty();
            }
        } else {
            log.info("No previous change for configuration {} on revision {}.", configPath, commit.getCommitId());
        }

        Optional<ConfigBundle> previousConfig = previousChangeCommit.map(
                previousCommit -> doGetOrCreateConfigRecursive(configPath, previousCommit, currentBranch)
        );

        return createConfig(
                configPath,
                commit,
                revisionNumberService.getOrderedArcRevision(currentBranch, commit),
                previousConfig.orElse(null),
                getPullRequestConfigChange(configPath, commit)
        );
    }


    @Nullable
    private ConfigBundle getPullRequestConfigChange(Path configPath, ArcCommit commit) {
        if (!commit.isTrunk()) {
            log.info("Fetching review data for commit in branch is not supported yet");
            return null;
        }

        Optional<ArcanumReviewDataDto> reviewData =
                arcanumClient.getReviewRequestBySvnRevision(commit.getSvnRevision());
        if (reviewData.isEmpty()) {
            log.info(
                    "Configuration {} on revision {} has no PR",
                    configPath,
                    commit.getCommitId()
            );

            return null;
        }

        Optional<ConfigEntity> configEntity = db.currentOrReadOnly(() ->
                db.configHistory().findLastConfig(configPath, ArcBranch.ofPullRequest(reviewData.get().getId())));

        if (configEntity.isPresent()) {
            log.info(
                    "Configuration {} on revision {} was changed in pr {}",
                    configPath,
                    commit.getCommitId(),
                    reviewData.get().getId()
            );
        } else {
            log.info(
                    "Configuration {} on revision {} was NOT changed in pr {}",
                    configPath,
                    commit.getCommitId(),
                    reviewData.get().getId()
            );
        }
        return configEntity
                .map(this::createBundle)
                .orElse(null);
    }

    private Optional<ArcCommit> getPreviousChangeCommit(Path configPath, ArcRevision revision) {
        return doGetPreviousChangeCommit(configPath, revision)
                .filter(commit -> {
                    if (!commit.getPath().orElseThrow().equals(configPath)) {
                        log.info(
                                "Configuration {} had name {} at previous revision {}. Treat it as new",
                                configPath,
                                commit.getPath(),
                                commit.getArcCommit().getRevision()
                        );
                        return false;
                    }

                    return true;
                })
                .map(ArcCommitWithPath::getArcCommit);
    }

    private Optional<ArcCommitWithPath> doGetPreviousChangeCommit(Path configPath, ArcRevision revision) {
        var commits = arcService.getCommits(
                revision, null, configPath, 2
        );
        if (commits.isEmpty()) {
            return Optional.empty();
        }
        var firstCommit = commits.get(0);

        if (!firstCommit.getArcCommit().getRevision().equals(revision)) {
            return Optional.of(firstCommit);
        }

        if (commits.size() > 1) {
            return Optional.of(commits.get(1));
        }
        return Optional.empty();
    }

    private MapDifference<TaskId, ArcRevision> getTaskRevisionChanges(ConfigBundle configBundle, ArcRevision revision) {
        var oldTaskRevisions = configBundle.getConfigEntity().getTaskRevisions();
        var newTaskVersions = configParseService.getTaskRevisions(
                configBundle.getValidAYamlConfig().getCi(),
                revision
        );
        return Maps.difference(oldTaskRevisions, newTaskVersions);
    }

    private ConfigBundle createBundle(ConfigEntity configEntity) {
        // Ignore tasks revisions if status is INVALID because we can't expect proper task configuration.
        // For instance, configuration could be invalid on this revision but after CI upgrade
        // same configuration (on same revision) could be valid.

        var status = configEntity.getStatus();
        var tasks = (status == ConfigStatus.INVALID) ? null : configEntity.getTaskRevisions();

        ConfigParseResult parseResult = configParseService.parseAndValidate(
                configEntity.getConfigPath(),
                configEntity.getRevision().toRevision(),
                tasks
        );

        if (!parseResult.getProblems().equals(configEntity.getProblems())) {
            log.warn(
                    "Problems list changed for configuration {} on revision {}. " +
                            "Stored in configEntity: {}, parsed: {}.",
                    configEntity.getConfigPath(),
                    configEntity.getRevision(),
                    configEntity.getProblems(),
                    parseResult.getProblems()
            );
        }

        return createBundle(configEntity, parseResult);
    }

    private static ConfigBundle createBundle(ConfigEntity configEntity, ConfigParseResult parseResult) {
        return new ConfigBundle(
                configEntity,
                parseResult.getAYamlConfig(),
                parseResult.getTaskConfigs()
        );
    }


    private ConfigBundle createConfig(
            Path configPath,
            ArcCommit commit,
            OrderedArcRevision revision,
            @Nullable ConfigBundle previousConfig,
            @Nullable ConfigBundle previousPullRequestConfig
    ) {

        log.info("Creating new version for configuration {} on revision {}", configPath, commit.getRevision());

        ConfigParseResult parseResult = configParseService.parseAndValidate(configPath, commit.getRevision(), null);

        if (!parseResult.getProblems().isEmpty()) {
            var errors = parseResult.getProblems().stream()
                    .map(ConfigProblem::toString)
                    .collect(Collectors.joining("\n"));
            log.info("Configuration {} on revision {} errors: {}", configPath, commit.getRevision(), errors);
        }

        ConfigBundle previousValidConfig = getPreviousValidConfig(configPath, previousConfig);
        if (previousValidConfig == null) {
            log.info("No previous valid configuration found");
        } else {
            log.info("Found previous valid configuration");
        }

        ConfigSecurityState securityState = securityStateService.getSecurityState(
                configPath,
                parseResult,
                commit,
                previousValidConfig,
                previousPullRequestConfig
        );
        log.info("Calculated security state: {}", securityState);

        var configStatus = calculateConfigStatus(parseResult, securityState);
        ConfigEntity configEntity = new ConfigEntity(
                configPath,
                revision,
                configStatus,
                parseResult.getProblems(),
                parseResult.getTaskRevisions(),
                securityState,
                createCreationInfo(previousConfig, previousValidConfig, previousPullRequestConfig),
                createConfigPermissions(parseResult, configStatus),
                commit.getAuthor()
        );
        db.configHistory().save(configEntity);
        return createBundle(configEntity, parseResult);
    }

    @Nullable
    private ConfigBundle getPreviousValidConfig(Path configPath, @Nullable ConfigBundle previousConfig) {
        if (previousConfig == null) {
            return null;
        }
        if (previousConfig.getConfigEntity().getStatus().isValidCiConfig()) {
            return previousConfig;
        }

        OrderedArcRevision previousValidRevision = previousConfig.getConfigEntity()
                .getCreationInfo()
                .getPreviousValidRevision()
                .orElse(null);

        if (previousValidRevision != null) {
            return getConfig(configPath, previousValidRevision);
        }
        return null;
    }

    private ConfigCreationInfo createCreationInfo(
            @Nullable ConfigBundle previousConfig,
            @Nullable ConfigBundle previousValidConfig,
            @Nullable ConfigBundle previousPullRequestConfig
    ) {
        return new ConfigCreationInfo(
                clock.instant(),
                (previousConfig == null) ? null : previousConfig.getRevision(),
                (previousValidConfig == null) ? null : previousValidConfig.getRevision(),
                (previousPullRequestConfig == null) ? null : previousPullRequestConfig.getRevision()
        );
    }

    @Nullable
    public ConfigPermissions createConfigPermissions(ConfigParseResult config, ConfigStatus configStatus) {
        if (!configStatus.isValidCiConfig()) {
            return null;
        }

        var builder = ConfigPermissions.builder();

        var yaml = config.getAYamlConfig();
        Preconditions.checkState(yaml != null, "Yaml config cannot be null if config is CI valid");

        var ci = yaml.getCi();

        builder.project(yaml.getService());
        builder.approvals(PermissionsProcessor.compressRules(ci.getApprovals()));

        var parent = ci.getPermissions();
        for (var release : ci.getReleases().values()) {
            var merge = merge(parent, release.getPermissions());
            if (!merge.getPermissions().isEmpty()) {
                builder.release(release.getId(), merge);
            }
        }

        for (var release : ci.getMergedActions().values()) {
            var merge = merge(parent, release.getPermissions());
            if (!merge.getPermissions().isEmpty()) {
                builder.action(release.getId(), merge);
            }
        }

        for (var flow : ci.getFlows().values()) {
            var flowBuilder = ConfigPermissions.FlowPermissions.builder();
            for (var jobs : List.of(flow.getJobs(), flow.getCleanupJobs())) {
                for (var job : jobs.values()) {
                    var manual = job.getManual();
                    if (manual != null) {
                        var approvers = manual.getApprovers();
                        if (!approvers.isEmpty()) {
                            flowBuilder.jobApprover(job.getId(), approvers);
                        }
                    }
                }
            }

            var flowPermissions = flowBuilder.build();
            if (!flowPermissions.getJobApprovers().isEmpty()) {
                builder.flow(flow.getId(), flowPermissions);
            }
        }

        return builder.build();
    }

    private ConfigStatus calculateConfigStatus(ConfigParseResult parseResult, ConfigSecurityState securityState) {
        return switch (parseResult.getStatus()) {
            case VALID -> securityState.isValid() ? ConfigStatus.READY : ConfigStatus.SECURITY_PROBLEM;
            case NOT_CI -> ConfigStatus.NOT_CI;
            case INVALID -> ConfigStatus.INVALID;
            default -> throw new UnsupportedOperationException("Unknown ConfigParseResult.Status: " +
                    parseResult.getStatus());
        };
    }

    private static Permissions merge(@Nullable Permissions parent, @Nullable Permissions child) {
        return PermissionsProcessor.finalizePermissions(PermissionsProcessor.overridePermissions(parent, child));
    }

}
