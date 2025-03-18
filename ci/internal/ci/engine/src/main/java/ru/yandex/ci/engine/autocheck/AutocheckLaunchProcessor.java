package ru.yandex.ci.engine.autocheck;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import com.google.common.collect.Sets;
import lombok.AccessLevel;
import lombok.RequiredArgsConstructor;
import lombok.experimental.FieldDefaults;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.autocheck.Autocheck;
import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.arcanum.ArcanumReviewDataDto;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.poller.Poller;
import ru.yandex.ci.core.storage.StorageUtils;
import ru.yandex.ci.engine.autocheck.config.AutocheckConfigurationConfig;
import ru.yandex.ci.engine.autocheck.config.AutocheckYamlService;
import ru.yandex.ci.engine.autocheck.model.AutocheckLaunchConfig;
import ru.yandex.ci.engine.autocheck.model.CheckLaunchParams;
import ru.yandex.ci.engine.autocheck.model.CheckRecheckLaunchParams;
import ru.yandex.ci.engine.autocheck.model.CheckRegistrationParams;
import ru.yandex.ci.engine.config.BranchConfigBundle;
import ru.yandex.ci.engine.config.BranchYamlService;
import ru.yandex.ci.engine.pcm.PCMSelector;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.util.CiJson;

@Slf4j
@RequiredArgsConstructor
@FieldDefaults(makeFinal = true, level = AccessLevel.PRIVATE)
public class AutocheckLaunchProcessor {
    public static final String DEFAULT_BRANCH_POOL = "autocheck/branches/public";

    @Nonnull
    ArcService arcService;

    @Nonnull
    AYamlService aYamlService;

    @Nonnull
    PCMSelector pcmSelector;

    @Nonnull
    AutocheckYamlService autocheckYamlService;

    @Nonnull
    BranchYamlService branchYamlService;

    @Nonnull
    ArcanumClientImpl arcanumClient;

    @Nonnull
    AutocheckRegistrationService registrationService;

    @Nonnull
    FastCircuitTargetsAutoResolver fastTargetResolver;

    @Nonnull
    CiMainDb db;

    public Autocheck.AutocheckLaunch create(CheckLaunchParams params) {
        Preconditions.checkArgument(params.isPrecommitCheck(), "Only precommit check is supported");

        var leftRevision = params.getLeftRevision();
        var rightRevision = params.getRightRevision();
        var checkId = params.getArcanumCheckId();
        var checkAuthor = params.getCheckAuthor();

        var baseBranch = leftRevision.getBranch();

        log.info(
                "Creating autocheck launch. " +
                        "Base branch {}, left revision {}, right revision {}, check author {}, CI check {}",
                baseBranch, leftRevision, rightRevision, checkAuthor, checkId
        );

        Preconditions.checkState(
                baseBranch.isTrunk() || baseBranch.isRelease(),
                "Autocheck is not supported for branch type " + baseBranch.getType()
        );
        var checkRegistrationParams = getPrecommitCheckRegistrationParams(params, leftRevision, rightRevision,
                checkAuthor, baseBranch);
        return registrationService.register(checkRegistrationParams);
    }

    private CheckRegistrationParams getPrecommitCheckRegistrationParams(
            CheckLaunchParams params,
            OrderedArcRevision leftRevision,
            OrderedArcRevision rightRevision,
            String checkAuthor,
            ArcBranch baseBranch
    ) {
        var launchConfig = baseBranch.isTrunk() ?
                findTrunkAutocheckLaunchParams(leftRevision, rightRevision, checkAuthor) :
                findBranchAutocheckLaunchParams(leftRevision.getBranch(), leftRevision);

        var disabledConfigurations = getDisabledConfigurations();

        var configurations = mergeConfigurations(launchConfig, disabledConfigurations);

        return CheckRegistrationParams.builder()
                .launchParams(params)
                .autocheckLaunchConfig(launchConfig)
                .autocheckConfigurations(configurations)
                .disabledConfigurations(disabledConfigurations)
                .distbuildPriority(calcDistbuildPriority(baseBranch, leftRevision.getNumber()))
                .zipatch(calcZipatch(params))
                .build();
    }

    private Set<String> getDisabledConfigurations() {

        var configurationStatuses = db.currentOrReadOnly(() ->
                db.keyValue().findObject(
                        ConfigurationStatuses.KEY.getNamespace(),
                        ConfigurationStatuses.KEY.getKey(),
                        ConfigurationStatuses.class
                )
        ).orElse(new ConfigurationStatuses(Map.of()));

        var disabledConfigurations = configurationStatuses.getPlatforms().entrySet().stream()
                .filter(x -> !x.getValue())
                .map(Map.Entry::getKey)
                .collect(Collectors.toSet());

        log.info("Disabled autocheck configurations: {}", disabledConfigurations);
        return disabledConfigurations;
    }


    static List<AutocheckConfiguration> mergeConfigurations(AutocheckLaunchConfig launchConfig,
                                                            Set<String> disabledConfigurations) {
        var leftConfigs = toConfigs(launchConfig.getLeftConfigBundle());
        var rightConfigs = toConfigs(launchConfig.getRightConfigBundle());

        var ids = new LinkedHashSet<String>();
        ids.addAll(leftConfigs.keySet());
        ids.addAll(rightConfigs.keySet());

        var result = new ArrayList<AutocheckConfiguration>();
        for (var id : ids) {
            if (disabledConfigurations.contains(id)) {
                log.info("Skipping '{}' configuration, cause it is disabled by degradation", id);
                continue;
            }
            result.add(new AutocheckConfiguration(id, leftConfigs.get(id), rightConfigs.get(id)));
        }
        return result;
    }

    private static Map<String, AutocheckConfigurationConfig> toConfigs(AutocheckYamlService.ConfigBundle configBundle) {

        return configBundle.getConfig().getConfigurations()
                .stream()
                .filter(AutocheckConfigurationConfig::isEnabled)
                .collect(Collectors.toMap(
                        AutocheckConfigurationConfig::getId,
                        Function.identity(),
                        (config1, config2) -> {
                            throw new IllegalStateException(
                                    String.format("Two configuration with same id: %s, %s", config1, config2)
                            );
                        },
                        LinkedHashMap::new
                ));
    }

    public Autocheck.AutocheckRecheckLaunch createRecheck(CheckRecheckLaunchParams params) {
        var check = params.getCheck();
        if (check.getDistbuildPriority().getPriorityRevision() == 0) {
            check = check.toBuilder()
                    .setDistbuildPriority(
                            CheckOuterClass.DistbuildPriority.newBuilder()
                                    .setFixedPriority(0)
                                    .setPriorityRevision(params.getCheck().getRightRevision().getRevisionNumber())
                                    .build()
                    )
                    .build();
        }
        var left = check.getLeftRevision();
        var right = check.getRightRevision();

        var branch = ArcBranch.ofString(left.getBranch());

        var leftRevision = ArcRevision.of(left.getRevision());
        var rightRevision = ArcRevision.of(right.getRevision());

        // todo take config from original launch
        AutocheckLaunchConfig autocheckLaunchConfig;
        if (check.getType() == CheckOuterClass.CheckType.TRUNK_POST_COMMIT ||
                check.getType() == CheckOuterClass.CheckType.BRANCH_POST_COMMIT) {

            var leftConfigRevision = ProtoMappers.toArcRevision(check.getAutocheckConfigLeftRevision());
            var rightConfigRevision = ProtoMappers.toArcRevision(check.getAutocheckConfigLeftRevision());

            var leftConfig = autocheckYamlService.getLastConfigForRevision(leftConfigRevision);
            var rightConfig = autocheckYamlService.getLastConfigForRevision(rightConfigRevision);

            autocheckLaunchConfig = new AutocheckLaunchConfig(
                    Set.of(),   // native builds
                    Set.of(),   // large tests
                    CheckOuterClass.LargeConfig.getDefaultInstance(), // No default large configuration
                    AutocheckConstants.FULL_CIRCUIT_TARGETS,
                    new TreeSet<>(),    // fast targets
                    false,
                    Set.of(),           // invalid ayamls
                    AutocheckConstants.PostCommits.DEFAULT_POOL,
                    false,
                    CheckIteration.StrongModePolicy.BY_A_YAML,
                    leftConfig,
                    rightConfig
            );
        } else {
            autocheckLaunchConfig = branch.isTrunk()
                    ? findTrunkAutocheckLaunchParams(leftRevision, rightRevision, check.getOwner())
                    : findBranchAutocheckLaunchParams(ArcBranch.ofString(left.getBranch()), leftRevision);
        }

        var targets = params.getIteration().getId().getCheckType() == CheckIteration.IterationType.FAST
                ? autocheckLaunchConfig.getFastTargets()
                : autocheckLaunchConfig.getFullTargets();


        var autocheckConfigLeftRevision = check.getAutocheckConfigLeftRevision().getCommitId().isEmpty() ?
                leftRevision : ArcRevision.of(check.getAutocheckConfigLeftRevision().getCommitId());

        var autocheckConfigRightRevision = check.getAutocheckConfigRightRevision().getCommitId().isEmpty() ?
                rightRevision : ArcRevision.of(check.getAutocheckConfigRightRevision().getCommitId());

        var checkInfo = registrationService.createAutocheckLaunchCheckInfo(check, autocheckLaunchConfig.getPoolName(),
                params.getGsidBase());

        var leftJobs = createRecheckJobs(params, autocheckConfigLeftRevision, targets, false);
        var rightJobs = createRecheckJobs(params, autocheckConfigRightRevision, targets, true);

        var autocheckRecheckLaunchBuilder = Autocheck.AutocheckRecheckLaunch.newBuilder()
                .setCheckInfo(checkInfo)
                .setIterationId(params.getIteration().getId())
                .addAllLeftJobs(leftJobs)
                .addAllRightJobs(rightJobs);

        return autocheckRecheckLaunchBuilder.build();
    }

    private List<Autocheck.AutocheckJob> createRecheckJobs(
            CheckRecheckLaunchParams params,
            ArcRevision configRevision,
            Set<String> targets,
            boolean isRight
    ) {

        var iteration = params.getIteration();


        log.info(
                "Creating {} recheck jobs for iteration {}. Config revision {}.",
                (isRight ? "right" : "left"), iteration.getId(), configRevision
        );


        var autocheckYamlConfig = autocheckYamlService.getConfig(configRevision);

        var jobs = new ArrayList<Autocheck.AutocheckJob>();

        var suitesGrouped = params.getSuites().stream().collect(
                Collectors.groupingBy(
                        StorageApi.SuiteRestart::getJobName,
                        Collectors.mapping(
                                Function.identity(), Collectors.groupingBy(StorageApi.SuiteRestart::getIsRight)
                        )
                )
        );

        for (var configuration : autocheckYamlConfig.getConfigurations()) {
            var jobName = configuration.getId();

            if (params.getCheck().getType() == CheckOuterClass.CheckType.TRUNK_POST_COMMIT ||
                    params.getCheck().getType() == CheckOuterClass.CheckType.BRANCH_POST_COMMIT) {
                jobName = AutocheckConstants.PostCommits.getTestenvTestNameByConfigurationId(jobName);
            }

            var jobSuites = suitesGrouped.getOrDefault(jobName, Map.of()).get(isRight);
            if (jobSuites == null) {
                log.info("No suites for configuration {}, job name: {}", configuration.getId(), jobName);
                continue;
            }

            var job = createRecheckJob(params, configuration, jobName, configRevision, isRight, jobSuites, targets);
            log.info("Recheck job created: {}", job);
            jobs.add(job);
        }

        log.info(
                "Created {} {} recheck jobs for iteration {}",
                jobs.size(), (isRight ? "right" : "left"), iteration.getId()
        );

        return jobs;
    }

    private Autocheck.AutocheckJob createRecheckJob(CheckRecheckLaunchParams params,
                                                    AutocheckConfigurationConfig configuration,
                                                    String jobName,
                                                    CommitId configRevision,
                                                    boolean isRight,
                                                    List<StorageApi.SuiteRestart> suites,
                                                    Set<String> targets) {
        var storageTask = registrationService.registerStorageTask(
                params.getCheck(),
                params.getIteration(),
                configuration,
                isRight,
                StorageUtils.toRestartJobName(jobName),
                (int) suites.stream().map(StorageApi.SuiteRestart::getPartition).distinct().count()
        );

        return Autocheck.AutocheckJob.newBuilder()
                .setStorageTask(storageTask)
                .setConfiguration(
                        AutocheckProtoMappers.toProtoAutocheckConfiguration(configuration, configRevision)
                )
                .addAllTargets(targets) //TODO разобраться, надо ли вообще их передавать в речеках
                .setCustomTargetsList(toCustomTargetList(suites))
                .setTestsRetries(3)
                .setSide(isRight ? Autocheck.AutocheckJob.Side.RIGHT : Autocheck.AutocheckJob.Side.LEFT)
                .build();
    }

    private String toCustomTargetList(List<StorageApi.SuiteRestart> suites) {
        var rootArray = CiJson.mapper().createArrayNode();

        for (var suite : suites) {
            var suiteArray = CiJson.mapper().createArrayNode();
            suiteArray.add(suite.getPath());
            suiteArray.add(suite.getToolchain());
            suiteArray.add(suite.getPartition());
            rootArray.add(suiteArray);
        }

        return rootArray.toString();
    }

    public AutocheckLaunchConfig findBranchAutocheckLaunchParams(ArcBranch branch, CommitId leftRevision) {
        Preconditions.checkState(branch.isRelease(), "Only release branches supported");
        BranchConfigBundle branchConfigBundle = branchYamlService.findBranchConfigWithAutocheckSection(branch)
                .orElseThrow(
                        () -> new RuntimeException("No config with autocheck section is present for branch: " + branch)
                );

        Preconditions.checkState(
                branchConfigBundle.isValid(),
                "Branch config %s is invalid. Problems: %s",
                branchConfigBundle.getPath(),
                branchConfigBundle.getProblems()
        );

        var config = branchConfigBundle.getAutocheckSection();
        var configPath = branchConfigBundle.getPath();

        var largeAutostart = config.getLargeAutostart().stream()
                .map(large -> ProtoMappers.toProtoLargeAutostart(configPath, large))
                .collect(Collectors.toSet());

        CheckOuterClass.LargeConfig largeConfig;
        var delegatedConfig = branchConfigBundle.getDelegatedConfig();
        if (delegatedConfig == null) {
            Preconditions.checkState(largeAutostart.isEmpty(),
                    "Internal error. Large autostart settings must not exists if no delegated config is provided");
            log.info("No delegated config for Large tests");
            largeConfig = CheckOuterClass.LargeConfig.getDefaultInstance();
        } else {
            largeConfig = CheckOuterClass.LargeConfig.newBuilder()
                    .setPath(delegatedConfig.getConfigPath().toString())
                    .setRevision(AutocheckProtoMappers.toProtoOrderedRevision(delegatedConfig.getRevision()))
                    .build();
        }

        var pool = Strings.isNullOrEmpty(config.getPool()) ? DEFAULT_BRANCH_POOL : config.getPool();

        var configBundle = autocheckYamlService.getLastConfigForRevision(leftRevision);

        return new AutocheckLaunchConfig(
                Set.of(), // No native builds in branches yet
                largeAutostart,
                largeConfig,
                Sets.newTreeSet(config.getDirs()),
                Collections.emptyNavigableSet(),
                false,
                Set.of(),
                pool,
                false,
                config.isStrong()
                        ? CheckIteration.StrongModePolicy.FORCE_ON
                        : CheckIteration.StrongModePolicy.FORCE_OFF,
                configBundle,
                configBundle
        );
    }

    public AutocheckLaunchConfig findTrunkAutocheckLaunchParams(
            CommitId leftRevision,
            CommitId rightRevision,
            @Nullable String checkAuthor
    ) {
        var autocheckInfoCollector = new AutocheckInfoCollector(
                autocheckYamlService, aYamlService, fastTargetResolver,
                leftRevision, rightRevision, pcmSelector, checkAuthor
        );
        arcService.processChanges(rightRevision, leftRevision, autocheckInfoCollector);
        return autocheckInfoCollector.getAutocheckLaunchConfig();
    }

    /**
     * В текущий момент в Distbuild'е не работают пулы, поэтому повторяем логику вычисления distbuild_priority
     * отсюда https://nda.ya.ru/t/EiYgn6eh4GbjNK
     * После внедрения пулов надо переосмыслить в рамках CI-2668
     */
    private CheckOuterClass.DistbuildPriority calcDistbuildPriority(ArcBranch branch, long revisionNumber) {
        long revision;
        if (branch.isTrunk()) {
            Preconditions.checkState(revisionNumber > 0,
                    "Revision number for distbuild priority must be > 0, got %s", revisionNumber);
            revision = revisionNumber;
            log.info("Using provided revision to calc distbuild priority {}", revision);
        } else {
            var lastTrunkRev = arcService.getLastRevisionInBranch(ArcBranch.trunk());
            var lastTrunkCommit = arcService.getCommit(lastTrunkRev);

            revision = lastTrunkCommit.getSvnRevision();
            log.info("Using trunk head revision to calc distbuild priority {}", revision);
        }

        var distbuildFixedPriority = -10_000_000L - revision + 200_000L;
        var priority = CheckOuterClass.DistbuildPriority.newBuilder()
                .setFixedPriority(distbuildFixedPriority)
                .setPriorityRevision(revision)
                .build();
        log.info("Distbuild Priority: {}", priority);
        return priority;
    }

    private CheckOuterClass.Zipatch calcZipatch(CheckLaunchParams params) {
        var empty = CheckOuterClass.Zipatch.getDefaultInstance();
        if (!params.getLeftRevision().getBranch().isTrunk()) {
            return empty;
        }
        if (params.getPullRequestId() == null || params.getDiffSetId() == null) {
            return empty;
        }
        var zipatch = getZipatch(
                params.getPullRequestId(),
                params.getDiffSetId()
        );
        return CheckOuterClass.Zipatch.newBuilder()
                .setUrl(zipatch.getUrlWithPrefix())
                .setBaseRevision(zipatch.getSvnBaseRevision())
                .build();
    }

    //TODO remove CI-2778
    private ArcanumReviewDataDto.Zipatch getZipatch(long reviewRequestId, long diffSetId) {
        try {
            ArcanumReviewDataDto reviewRequestData = Poller.poll(
                            () -> arcanumClient.getReviewRequestData(
                                    reviewRequestId,
                                    "id,author(name),commit_description," +
                                            "diff_sets(" +
                                            /* - */ "id,gsid,zipatch(url,svn_base_revision,svn_branch)," +
                                            /* - */ "arc_branch_heads(from_id,to_id,merge_id),patch_url" +
                                            ")," +
                                            "ci_settings(fast_circuit,check_fast_mode)"
                            ))
                    .canStopWhen(Optional::isPresent)
                    .interval(30, TimeUnit.SECONDS)
                    .timeout(2, TimeUnit.HOURS)
                    .retryOnExceptionCount(240)
                    .run()
                    .orElseThrow(() -> new RuntimeException("can't fetch review request %s from Arcanum"
                            .formatted(reviewRequestId)
                    ));

            var diffSet = reviewRequestData.getDiffSets().stream()
                    .filter(it -> it.getId() == diffSetId)
                    .findFirst()
                    .orElseThrow(() -> new RuntimeException("review request %s has no diff set %s: %s"
                            .formatted(reviewRequestId, diffSetId, reviewRequestData)
                    ));

            return diffSet.getZipatch();

        } catch (TimeoutException | InterruptedException e) {
            log.error("Failed to get zipatch", e);
            throw new RuntimeException(e);
        }

    }

}
