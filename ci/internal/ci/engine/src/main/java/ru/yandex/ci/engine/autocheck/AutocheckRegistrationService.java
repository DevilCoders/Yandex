package ru.yandex.ci.engine.autocheck;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.function.Supplier;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.experimental.FieldDefaults;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.autocheck.Autocheck;
import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.client.arcanum.RegisterCheckRequestDto;
import ru.yandex.ci.client.arcanum.UpdateCheckStatusRequest;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.poller.Poller;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.engine.autocheck.config.AutocheckConfigurationConfig;
import ru.yandex.ci.engine.autocheck.model.AutocheckLaunchConfig;
import ru.yandex.ci.engine.autocheck.model.CheckLaunchParams;
import ru.yandex.ci.engine.autocheck.model.CheckRegistrationParams;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;

@Slf4j
@RequiredArgsConstructor
@FieldDefaults(makeFinal = true, level = AccessLevel.PROTECTED)
public class AutocheckRegistrationService {
    public static final int FIRST_ITERATION_NUMBER = 1;

    @Nonnull
    ArcService arcService;

    @Nonnull
    StorageApiClient storageApiClient;

    @Nonnull
    ArcanumClientImpl arcanumClient;

    public Autocheck.AutocheckLaunch register(CheckRegistrationParams params) {
        return register(params, Autocheck.AutocheckLaunch.newBuilder());
    }

    private Autocheck.AutocheckLaunch register(
            CheckRegistrationParams params,
            Autocheck.AutocheckLaunch.Builder autocheckLaunchBuilder
    ) {
        log.info("Creating autocheck launch");
        logAutocheckConfigurations(params);

        var launchConfig = params.getAutocheckLaunchConfig();
        var disabledConfigurations = params.getDisabledConfigurations();

        var check = registerCheck(params);

        log.info("Autocheck launch config: {}", launchConfig);

        var checkInfoBuilder = createAutocheckLaunchCheckInfo(check, params.getAutocheckLaunchConfig().getPoolName(),
                params.getLaunchParams().getGsidBase());

        autocheckLaunchBuilder.setCheckInfo(checkInfoBuilder.build());

        registerCheckIterations(params, autocheckLaunchBuilder, check, disabledConfigurations);
        registerCheckInArcanum(params.getLaunchParams(), check);

        var autocheckLaunch = autocheckLaunchBuilder.build();
        log.info("Autocheck launch created: {}", autocheckLaunch);

        return autocheckLaunch;
    }

    Autocheck.CheckInfo.Builder createAutocheckLaunchCheckInfo(CheckOuterClass.Check check,
                                                               String poolName,
                                                               String gsidBase) {
        return Autocheck.CheckInfo.newBuilder()
                .setCheckId(check.getId())
                .setLeftRevision(AutocheckProtoMappers.toProtoCommitId(check.getLeftRevision()))
                .setRightRevision(AutocheckProtoMappers.toProtoCommitId(check.getRightRevision()))
                .setCheckAuthor(check.getOwner())
                .setPoolName(poolName)
                .setGsid(toGsid(gsidBase, check))
                .setDistbuildPriority(check.getDistbuildPriority())
                .setZipatch(check.getZipatch());
    }

    public String toGsid(String gsidBase, CheckOuterClass.Check check) {
        return gsidBase + " " + "CI_STORAGE:" + check.getId();
    }

    private CheckOuterClass.Check registerCheck(CheckRegistrationParams params) {
        var autocheckLaunchConfig = params.getAutocheckLaunchConfig();

        var checkInfo = createStorageCheckInfo(params, autocheckLaunchConfig);

        // tags used for identifying check across several checks on same revisions
        var tags = getCheckRegistrationTags(params);

        var check = storageApiClient.registerCheck(getRegisterCheckRequest(params, checkInfo, tags));

        log.info("Check registered: {}", check.getId());
        return check;
    }

    List<String> getCheckRegistrationTags(CheckRegistrationParams params) {
        return List.of(
                params.getLaunchParams().getLaunchId(),
                "arcanum_check_id=" + params.getLaunchParams().getArcanumCheckId()
        );
    }

    @Nonnull
    private CheckOuterClass.CheckInfo createStorageCheckInfo(CheckRegistrationParams params,
                                                             AutocheckLaunchConfig autocheckLaunchConfig) {
        if (params.getLaunchParams().isStressTest()) {
            log.info("Return CheckInfo.getDefaultInstance cause stressTest=true");
            return CheckOuterClass.CheckInfo.getDefaultInstance();
        }

        log.info(
                "Large auto-start is scheduling for user {}: {}",
                params.getLaunchParams().getCheckAuthor(), autocheckLaunchConfig.getLargeTests()
        );

        return CheckOuterClass.CheckInfo.newBuilder()
                .addAllNativeBuilds(autocheckLaunchConfig.getNativeBuilds())
                .addAllLargeAutostart(autocheckLaunchConfig.getLargeTests())
                .setDefaultLargeConfig(autocheckLaunchConfig.getLargeConfig())
                .build();
    }

    private StorageApi.RegisterCheckRequest getRegisterCheckRequest(
            CheckRegistrationParams params, CheckOuterClass.CheckInfo checkInfo, List<String> tags
    ) {
        var launchParams = params.getLaunchParams();
        var builder = StorageApi.RegisterCheckRequest.newBuilder()
                .setLeftRevision(AutocheckProtoMappers.toProtoOrderedRevision(launchParams.getLeftRevision()))
                .setRightRevision(AutocheckProtoMappers.toProtoOrderedRevision(launchParams.getRightRevision()))
                .setOwner(launchParams.getCheckAuthor())
                .addAllTags(tags)
                .setInfo(checkInfo)
                .setDiffSetId(launchParams.getDiffSetId() != null ? launchParams.getDiffSetId() : 0)
                .setAutocheckConfigLeftRevision(ProtoMappers.toCommitId(params.getAutocheckConfigLeftRevision()))
                .setAutocheckConfigRightRevision(ProtoMappers.toCommitId(params.getAutocheckConfigRightRevision()))
                .setReportStatusToArcanum(true)
                .setTestRestartsAllowed(params.getLaunchParams().getLeftRevision().getBranch().isTrunk())
                .setDistbuildPriority(params.getDistbuildPriority())
                .setZipatch(params.getZipatch())
                .setDiffSetEventCreated(ProtoConverter.convert(launchParams.getDiffSetEventCreated()))
                .setNotificationsDisabled(params.getLaunchParams().isStressTest())
                .setStressTest(params.getLaunchParams().isStressTest());

        return builder.build();
    }


    private void registerCheckIterations(
            CheckRegistrationParams params,
            Autocheck.AutocheckLaunch.Builder autocheckLaunchBuilder,
            CheckOuterClass.Check check,
            Set<String> disabledToolchains
    ) {
        log.info("Registering iterations for check {}", check.getId());
        registerFastIteration(params, autocheckLaunchBuilder, check, disabledToolchains);
        registerFullIteration(params, autocheckLaunchBuilder, check, disabledToolchains);
        log.info("Iterations created for check {}", check.getId());
    }

    private void registerFastIteration(
            CheckRegistrationParams params,
            Autocheck.AutocheckLaunch.Builder autocheckLaunchBuilder,
            CheckOuterClass.Check check, Set<String> disabledToolchains
    ) {
        if (!params.getAutocheckLaunchConfig().hasFastCircuit()) {
            log.info("Fast circuit not required");
            return;
        }

        log.info(
                "Register fast circuit. Check {}, targets: {}",
                check.getId(), params.getAutocheckLaunchConfig().getFastTargets()
        );
        var jobs = registerIterationAndAutocheckJobs(
                params, check,
                CheckIteration.IterationType.FAST,
                params.getAutocheckLaunchConfig().getFastTargets(),
                params.getAutocheckLaunchConfig().isAutodetectedFastCircuit(),
                disabledToolchains
        );
        autocheckLaunchBuilder.addAllLeftFastCircuitJobs(jobs.leftJobs);
        autocheckLaunchBuilder.addAllRightFastCircuitJobs(jobs.rightJobs);
        log.info("Fast circuit registered for check {}", check.getId());
    }

    private void registerFullIteration(
            CheckRegistrationParams params,
            Autocheck.AutocheckLaunch.Builder autocheckLaunchBuilder,
            CheckOuterClass.Check check,
            Set<String> disabledToolchains
    ) {
        //TODO support AutocheckInfo.sequentialMode
        log.info("Registering full circuit for check {}", check.getId());
        var jobs = registerIterationAndAutocheckJobs(
                params,
                check,
                CheckIteration.IterationType.FULL,
                params.getAutocheckLaunchConfig().getFullTargets(),
                false,
                disabledToolchains
        );
        autocheckLaunchBuilder.addAllLeftFullCircuitJobs(jobs.leftJobs);
        autocheckLaunchBuilder.addAllRightFullCircuitJobs(jobs.rightJobs);
        log.info("Full circuit registered for check {}", check.getId());
    }


    private AutocheckJobs registerIterationAndAutocheckJobs(
            CheckRegistrationParams params,
            CheckOuterClass.Check check,
            CheckIteration.IterationType iterationType,
            Set<String> targets,
            boolean autoFastTargets,
            @Nonnull Set<String> disabledToolchains
    ) {
        var expectedTasks = createExpectedTasks(params);

        log.info(
                "Registering iteration. Check: {}, iterationType {}, expected tasks: {}.",
                check.getId(),
                iterationType,
                expectedTasks
        );

        var iteration = storageApiClient.registerCheckIteration(
                check.getId(),
                iterationType,
                FIRST_ITERATION_NUMBER,
                expectedTasks,
                createIterationInfo(params, iterationType, targets, autoFastTargets, disabledToolchains)
        );

        log.info("Iteration registered: {}", iteration.getId());

        log.info(
                "Registering autocheck jobs for {} autocheck configuration(s)",
                params.getAutocheckConfigurations().size()
        );

        var autocheckJobs = new AutocheckJobs();
        for (var autocheckConfiguration : params.getAutocheckConfigurations()) {
            autocheckJobs.add(registerAutocheckJob(params, check, iteration, autocheckConfiguration, targets));
        }

        log.info("All autocheck job registered for iteration {}", iteration.getId());

        return autocheckJobs;
    }

    @Value
    protected static class AutocheckJobs {
        List<Autocheck.AutocheckJob> leftJobs = new ArrayList<>();
        List<Autocheck.AutocheckJob> rightJobs = new ArrayList<>();

        private void add(AutocheckJobs jobs) {
            leftJobs.addAll(jobs.getLeftJobs());
            rightJobs.addAll(jobs.getRightJobs());
        }
    }

    private CheckIteration.IterationInfo createIterationInfo(
            CheckRegistrationParams params,
            CheckIteration.IterationType iterationType,
            Set<String> targets,
            boolean autoFastTargets,
            Set<String> disabledToolchains
    ) {
        var builder = CheckIteration.IterationInfo.newBuilder()
                .setAdvisedPool(params.getAutocheckLaunchConfig().getPoolName())
                .setStrongModePolicy(params.getAutocheckLaunchConfig().getStrongModePolicy())
                .setFlowProcessId(ProtoMappers.toProtoFlowProcessId(params.getLaunchParams().getCiProcessId()))
                .setFlowLaunchNumber(params.getLaunchParams().getLaunchNumber())
                .addAllDisabledToolchains(disabledToolchains);

        if (iterationType == CheckIteration.IterationType.FAST) {
            builder.addAllFastTargets(targets);
            builder.setAutodetectedFastCircuit(autoFastTargets);
        }

        return builder.build();
    }

    private List<CheckIteration.ExpectedTask> createExpectedTasks(CheckRegistrationParams params) {
        var expectedTasks = new ArrayList<CheckIteration.ExpectedTask>();

        for (var configuration : params.getAutocheckConfigurations()) {
            if (configuration.hasLeftTask()) {
                expectedTasks.add(createExpectedTask(configuration.getLeftConfig(), false));
            }
            if (configuration.hasRightTask()) {
                expectedTasks.add(createExpectedTask(configuration.getRightConfig(), true));
            }
        }

        return expectedTasks;
    }

    private CheckIteration.ExpectedTask createExpectedTask(
            AutocheckConfigurationConfig autocheckConfiguration,
            boolean isRight
    ) {
        return CheckIteration.ExpectedTask.newBuilder()
                .setJobName(createJobName(autocheckConfiguration))
                .setRight(isRight)
                .build();
    }


    protected AutocheckJobs registerAutocheckJob(
            CheckRegistrationParams params,
            CheckOuterClass.Check check,
            CheckIteration.Iteration iteration,
            AutocheckConfiguration configuration,
            Set<String> targets
    ) {
        log.info(
                "Registering autocheck job. Check: {}, iteration: {}, configuration: {}, targets: {}.",
                check.getId(), iteration.getId(), configuration, targets
        );

        var autocheckJobBase = Autocheck.AutocheckJob.newBuilder()
                .addAllTargets(targets)
                .setCustomTargetsList("[]")
                .setTestsRetries(1)
                .build();

        AutocheckJobs autocheckJobs = new AutocheckJobs();

        Preconditions.checkState(
                configuration.hasLeftTask() || configuration.hasRightTask(),
                "Configuration %s dont have both (left and right) tasks"
        );

        if (configuration.hasLeftTask()) {
            var leftTask = registerStorageTask(check, iteration, configuration.getLeftConfig(), false, null, null);

            var leftConfig = AutocheckProtoMappers.toProtoAutocheckConfiguration(
                    configuration.getLeftConfig(), params.getAutocheckConfigLeftRevision()
            );

            var leftJob = Autocheck.AutocheckJob.newBuilder(autocheckJobBase)
                    .setStorageTask(leftTask)
                    .setConfiguration(leftConfig)
                    .setSide(Autocheck.AutocheckJob.Side.LEFT)
                    .build();
            autocheckJobs.leftJobs.add(leftJob);
            log.info("Left autocheck job registered {} for iteration {}", leftJob, iteration.getId());
        } else {
            log.warn("Configuration {} does not have left task", configuration.getId());
        }

        if (configuration.hasRightTask()) {
            var rightTask = registerStorageTask(check, iteration, configuration.getRightConfig(), true, null, null);

            var rightConfig = AutocheckProtoMappers.toProtoAutocheckConfiguration(
                    configuration.getRightConfig(), params.getAutocheckConfigRightRevision()
            );

            var rightJob = Autocheck.AutocheckJob.newBuilder(autocheckJobBase)
                    .setStorageTask(rightTask)
                    .setConfiguration(rightConfig)
                    .setSide(Autocheck.AutocheckJob.Side.RIGHT)
                    .build();
            autocheckJobs.rightJobs.add(rightJob);
            log.info("Right autocheck job registered {} for iteration {}", rightJob, iteration.getId());

        } else {
            log.warn("Configuration {} does not have right task", configuration.getId());
        }

        log.info("Autocheck job registered for iteration {}", iteration.getId());
        return autocheckJobs;
    }

    public Autocheck.CiStorageTask registerStorageTask(
            CheckOuterClass.Check check,
            CheckIteration.Iteration iteration,
            AutocheckConfigurationConfig configuration,
            boolean isRight,
            @Nullable String customJobName,
            @Nullable Integer customNumberOfPartitions
    ) {
        return registerStorageTask(check, iteration, configuration, isRight, customJobName, customNumberOfPartitions,
                null);
    }

    //TODO eventually move this to autocheck tasks
    public Autocheck.CiStorageTask registerStorageTask(
            CheckOuterClass.Check check,
            CheckIteration.Iteration iteration,
            AutocheckConfigurationConfig configuration,
            boolean isRight,
            @Nullable String customJobName,
            @Nullable Integer customNumberOfPartitions,
            @Nullable String customTaskId
    ) {
        var taskId = Objects.requireNonNullElseGet(customTaskId, () -> createTaskId(configuration, isRight));
        var jobName = customJobName != null ? customJobName : createJobName(configuration);
        var numberOfPartitions = customNumberOfPartitions != null ?
                customNumberOfPartitions : configuration.getPartitions().getCount();

        log.info(
                "Register storage task. Check: {}, iteration: {}, autocheck configuration: {}, " +
                        "isRight: {}, taskId {}, jobName: {}.",
                check.getId(), iteration.getId(), configuration.getId(), isRight, taskId, jobName
        );

        var checkType = iteration.getId().getCheckType();
        Preconditions.checkState(checkType == CheckIteration.IterationType.FAST ||
                        checkType == CheckIteration.IterationType.FULL,
                "Expect only autocheck tasks, got %s", checkType);
        var task = storageApiClient.registerTask(
                iteration.getId(),
                taskId,
                numberOfPartitions,
                isRight,
                jobName,
                Common.CheckTaskType.CTT_AUTOCHECK
        );

        log.info("Storage task registered: {}", task.getId());

        return Autocheck.CiStorageTask.newBuilder()
                .setId(task.getId())
                .setLogbrokerTopic(check.getLogbrokerTopic())
                .setLogbrokerSourceId(task.getLogbrokerSourceId())
                .build();
    }

    protected String createJobName(AutocheckConfigurationConfig autocheckConfiguration) {
        return autocheckConfiguration.getId();
    }

    private String createTaskId(AutocheckConfigurationConfig autocheckConfiguration, boolean isRight) {
        return autocheckConfiguration.getId() + (isRight ? "-right" : "-left");
    }

    private void logAutocheckConfigurations(CheckRegistrationParams params) {
        log.info(
                "Using {} autocheck configurations: {}.",
                params.getAutocheckConfigurations().size(),
                params.getAutocheckConfigurations().stream().map(AutocheckConfiguration::getId).toList()
        );

        for (var autocheckConfiguration : params.getAutocheckConfigurations()) {
            log.info("Autocheck configuration {}: {}", autocheckConfiguration.getId(), autocheckConfiguration);
        }
    }

    private void registerCheckInArcanum(CheckLaunchParams params, CheckOuterClass.Check check) {
        if (!isPrecommitCheck(check) || params.getPullRequestId() == null || params.getDiffSetId() == null
                || params.isStressTest()) {
            log.info("Skip check registration {} in arcanum: check type {}, pullRequestId {}, diffSetId {}" +
                            ", stressTest {}",
                    check.getId(), check.getType(), params.getPullRequestId(), params.getDiffSetId(),
                    params.isStressTest());
            return;
        }

        Supplier<String> actionName = () -> "null";
        var pollerBuilder = Poller.builder()
                .interval(30, TimeUnit.SECONDS)
                .timeout(2, TimeUnit.HOURS)
                .retryOnExceptionCount(240);

        try {
            actionName = () -> "arcanumClient.registerCiCheck";
            log.info("Registering check {} in arcanum", check.getId());
            pollerBuilder.poll(() -> {
                arcanumClient.registerCiCheck(
                        params.getDiffSetId(),
                        new RegisterCheckRequestDto(check.getId(), false, true)
                );
                log.info("Registered check {} in arcanum", check.getId());
                return null;
            }).build().run();

            for (var checkType : List.of(ArcanumCheckType.CI_BUILD, ArcanumCheckType.CI_TESTS)) {
                var request = UpdateCheckStatusRequest.builder()
                        .system(checkType.getSystem())
                        .type(checkType.getType())
                        .status(ArcanumMergeRequirementDto.Status.PENDING)
                        .systemCheckId(check.getId())
                        .build();

                actionName = () -> "arcanumClient.setMergeRequirementStatus: " + request;
                log.info("Changing check status for check {}: {}", check.getId(), request);

                pollerBuilder.poll(() -> {
                    arcanumClient.setMergeRequirementStatus(params.getPullRequestId(), params.getDiffSetId(), request);
                    log.info("Changed check status for check {}: {}", check.getId(), request);
                    return null;
                }).build().run();
            }
        } catch (TimeoutException | InterruptedException e) {
            throw new RuntimeException("failed to perform action " + actionName.get(), e);
        }
    }

    private static boolean isPrecommitCheck(CheckOuterClass.Check check) {
        return check.getType() == CheckOuterClass.CheckType.TRUNK_PRE_COMMIT
                || check.getType() == CheckOuterClass.CheckType.BRANCH_PRE_COMMIT;
    }
}
