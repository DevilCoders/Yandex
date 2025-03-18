package ru.yandex.ci.storage.core.proto;

import java.time.Instant;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.primitives.UnsignedLongs;

import ru.yandex.ci.api.storage.StorageApi.DelegatedConfig;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.flow.CiActionReference;
import ru.yandex.ci.core.proto.CiCoreProtoMappers;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.api.StorageFrontHistoryApi;
import ru.yandex.ci.storage.core.AggregateStatisticsOuterClass;
import ru.yandex.ci.storage.core.AggregateStatisticsOuterClass.AggregateExtendedStatistics;
import ru.yandex.ci.storage.core.AggregateStatisticsOuterClass.AggregateMainStatistics;
import ru.yandex.ci.storage.core.AggregateStatisticsOuterClass.ExtendedStatisticsGroup;
import ru.yandex.ci.storage.core.AggregateStatisticsOuterClass.StatisticsGroup;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.Common.StorageAttribute;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check.DistbuildPriority;
import ru.yandex.ci.storage.core.db.model.check.LargeAutostart;
import ru.yandex.ci.storage.core.db.model.check.LargeTestsConfig;
import ru.yandex.ci.storage.core.db.model.check.NativeBuild;
import ru.yandex.ci.storage.core.db.model.check.Zipatch;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.FatalErrorInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StrongModePolicy;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.ExpectedTask;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTestDelegatedConfig;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateStatistics;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffPageCursor;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.SuitePageCursor;
import ru.yandex.ci.storage.core.db.model.test_diff.SuiteSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusWithStatistics;
import ru.yandex.ci.storage.core.logbroker.LogbrokerTopics;
import ru.yandex.ci.storage.core.utils.HtmlSnippetFormatter;

@SuppressWarnings("UnstableApiUsage")
public class CheckProtoMappers {
    private CheckProtoMappers() {

    }

    public static CheckEntity toCheck(StorageApi.RegisterCheckRequest request) {
        var left = request.getLeftRevision();
        var right = request.getRightRevision();
        var topic = LogbrokerTopics.get(ArcBranch.ofString(left.getBranch()), ArcBranch.ofString(right.getBranch()));

        var info = request.getInfo();

        return CheckEntity.builder()
                .id(CheckEntity.Id.of(0L))
                .author(request.getOwner())
                .diffSetId(request.getDiffSetId())
                .left(toRevision(left))
                .right(toRevision(right))
                .tags(Set.copyOf(request.getTagsList()))
                .status(CheckStatus.CREATED)
                .created(Instant.now())
                .type(topic.getType())
                .autostartLargeTests(toLargeAutostartList(info.getLargeAutostartList()))
                .nativeBuilds(toNativeBuildList(info.getNativeBuildsList()))
                .largeTestsConfig(toLargeTestsConfig(info.getDefaultLargeConfig()))
                .reportStatusToArcanum(request.getReportStatusToArcanum())
                .autocheckConfigLeftRevision(request.getAutocheckConfigLeftRevision().getCommitId())
                .autocheckConfigRightRevision(request.getAutocheckConfigRightRevision().getCommitId())
                .testRestartsAllowed(request.getTestRestartsAllowed())
                .distbuildPriority(toDistbuildPriority(request.getDistbuildPriority()))
                .zipatch(toZipatch(request.getZipatch()))
                .diffSetEventCreated(ProtoConverter.convert(request.getDiffSetEventCreated()))
                .attributes(Map.of(
                        StorageAttribute.SA_STRESS_TEST, String.valueOf(request.getStressTest()),
                        StorageAttribute.SA_NOTIFICATIONS_DISABLED, String.valueOf(request.getNotificationsDisabled())
                ))
                .build();
    }


    public static CheckEntity toCheck(CheckOuterClass.Check check) {
        var left = check.getLeftRevision();
        var right = check.getRightRevision();

        return CheckEntity.builder()
                .id(CheckEntity.Id.of(check.getId()))
                .tags(Set.copyOf(check.getTagsList()))
                .left(toRevision(left))
                .right(toRevision(right))
                .author(check.getOwner())
                .diffSetId(check.getDiffSetId())
                .created(ProtoConverter.convert(check.getCreated()))
                .type(check.getType())
                .status(check.getStatus())
                .autocheckConfigLeftRevision(check.getAutocheckConfigLeftRevision().getCommitId())
                .autocheckConfigRightRevision(check.getAutocheckConfigRightRevision().getCommitId())
                .attributes(
                        check.getAttributesMap().entrySet().stream().collect(
                                Collectors.toMap(e -> StorageAttribute.valueOf(e.getKey()), Map.Entry::getValue)
                        )
                )
                .diffSetEventCreated(ProtoConverter.convert(check.getDiffSetEventCreated()))
                .build();
    }

    public static CheckOuterClass.Check toProtoCheck(CheckEntity entity) {
        return CheckOuterClass.Check.newBuilder()
                .setLeftRevision(toProtoRevision(entity.getLeft()))
                .setRightRevision(toProtoRevision(entity.getRight()))
                .setDiffSetId(entity.getDiffSetId())
                .setId(entity.getId().getId().toString())
                .setOwner(entity.getAuthor())
                .addAllTags(entity.getTags())
                .setCreated(ProtoConverter.convert(entity.getCreated()))
                .setType(entity.getType())
                .setLogbrokerTopic(LogbrokerTopics.get(entity.getType()).getPath())
                .setStatus(entity.getStatus())
                .setAutocheckConfigLeftRevision(toCommitId(entity.getAutocheckConfigLeftRevision()))
                .setAutocheckConfigRightRevision(toCommitId(entity.getAutocheckConfigRightRevision()))
                .putAllAttributes(
                        entity.getAttributes().entrySet().stream().collect(
                                Collectors.toMap(e -> e.getKey().name(), Map.Entry::getValue)
                        )
                )
                .setInfo(CheckOuterClass.CheckInfo.newBuilder()
                        .addAllLargeAutostart(toProtoLargeAutostart(entity.getAutostartLargeTests()))
                        .addAllNativeBuilds(toProtNativeBuild(entity.getNativeBuilds()))
                        .setDefaultLargeConfig(toProtoLargeTestsConfig(entity.getLargeTestsConfig())))
                .setDistbuildPriority(toProtoDistbuildPriority(entity.getDistbuildPriority()))
                .setZipatch(toProtoZipatch(entity.getZipatch()))
                .setDiffSetEventCreated(ProtoConverter.convert(entity.getDiffSetEventCreated()))
                .build();
    }

    public static ru.yandex.ci.api.proto.Common.CommitId toCommitId(String commitId) {
        return ru.yandex.ci.api.proto.Common.CommitId.newBuilder()
                .setCommitId(commitId)
                .build();
    }

    public static StorageRevision toRevision(Common.OrderedRevision revision) {
        return new StorageRevision(
                revision.getBranch(),
                revision.getRevision(),
                revision.getRevisionNumber(),
                ProtoConverter.convert(revision.getTimestamp())
        );
    }

    public static Common.OrderedRevision toProtoRevision(StorageRevision revision) {
        return Common.OrderedRevision.newBuilder()
                .setBranch(revision.getBranch())
                .setRevision(revision.getRevision())
                .setRevisionNumber(revision.getRevisionNumber())
                .setTimestamp(ProtoConverter.convert(revision.getTimestamp()))
                .build();
    }

    public static CheckIteration.IterationId toProtoIterationId(CheckIterationEntity.Id id) {
        return CheckIteration.IterationId.newBuilder()
                .setCheckId(id.getCheckId().getId().toString())
                .setCheckType(id.getIterationType())
                .setNumber(id.getNumber())
                .build();
    }

    public static CheckIteration.Iteration toProtoIteration(CheckIterationEntity iteration) {
        return CheckIteration.Iteration.newBuilder()
                .setId(toProtoIterationId(iteration.getId()))
                .setStatus(iteration.getStatus())
                .setCreated(ProtoConverter.convert(iteration.getCreated()))
                .setInfo(toProtoIterationInfo(iteration.getInfo()))
                .addAllExpectedTasks(
                        iteration.getExpectedTasks().stream()
                                .map(CheckProtoMappers::toProtoExpectedTask)
                                .collect(Collectors.toList())
                )
                .addAllRegisteredExpectedTasks(
                        iteration.getRegisteredExpectedTasks().stream()
                                .map(CheckProtoMappers::toProtoExpectedTask)
                                .collect(Collectors.toList())
                )
                .putAllAttributes(iteration.getAttributes())
                .setUseSuiteDiffIndex(iteration.isUseSuiteDiffIndex())
                .setUseImportantDiffIndex(iteration.isUseImportantDiffIndex())
                .setTasksType(iteration.getTasksType())
                .build();
    }

    private static CheckIteration.ExpectedTask toProtoExpectedTask(ExpectedTask x) {
        return CheckIteration.ExpectedTask.newBuilder()
                .setJobName(x.getJobName())
                .setRight(x.isRight())
                .build();
    }

    public static CheckIterationEntity toCheckIteration(CheckIteration.Iteration iteration) {
        return CheckIterationEntity.builder()
                .id(toIterationId(iteration.getId()))
                .status(iteration.getStatus())
                .created(ProtoConverter.convert(iteration.getCreated()))
                .statistics(IterationStatistics.EMPTY)
                .info(toIterationInfo(iteration.getInfo()))
                .registeredExpectedTasks(
                        iteration.getRegisteredExpectedTasksList().stream()
                                .map(task -> new ExpectedTask(task.getJobName(), task.getRight()))
                                .collect(Collectors.toSet())
                )
                .testenvId(iteration.getInfo().getTestenvCheckId())
                .attributes(iteration.getAttributesMap())
                .expectedTasks(
                        iteration.getExpectedTasksList().stream()
                                .map(task -> new ExpectedTask(task.getJobName(), task.getRight()))
                                .collect(Collectors.toSet())
                )
                .useSuiteDiffIndex(iteration.getUseSuiteDiffIndex())
                .useImportantDiffIndex(iteration.getUseImportantDiffIndex())
                .tasksType(iteration.getTasksType())
                .build();
    }

    public static CheckIterationEntity.Id toIterationId(CheckIteration.IterationId iterationId) {
        return CheckIterationEntity.Id.of(
                CheckEntity.Id.of(iterationId.getCheckId()),
                iterationId.getCheckType(),
                iterationId.getNumber()
        );
    }

    public static CheckTaskOuterClass.CheckTask toProtoCheckTask(CheckTaskEntity task) {
        return CheckTaskOuterClass.CheckTask.newBuilder()
                .setId(toProtoCheckTaskId(task.getId()))
                .setNumberOfPartitions(task.getNumberOfPartitions())
                .setStatus(task.getStatus())
                .setIsRight(task.isRight())
                .setJobName(task.getJobName())
                .setCreated(ProtoConverter.convert(task.getCreated()))
                .setLogbrokerSourceId(task.getLogbrokerSourceId())
                .setType(task.getType())
                .build();
    }

    public static CheckTaskOuterClass.FullTaskId toProtoCheckTaskId(CheckTaskEntity.Id id) {
        return CheckTaskOuterClass.FullTaskId.newBuilder()
                .setIterationId(toProtoIterationId(id.getIterationId()))
                .setTaskId(id.getTaskId())
                .build();
    }

    public static CheckTaskEntity toCheckTask(
            CheckTaskOuterClass.CheckTask checkTask
    ) {
        CheckTaskEntity.Id taskId = new CheckTaskEntity.Id(
                toIterationId(checkTask.getId().getIterationId()), checkTask.getId().getTaskId()
        );

        return CheckTaskEntity.builder()
                .id(taskId)
                .numberOfPartitions(checkTask.getNumberOfPartitions())
                .status(checkTask.getStatus())
                .right(checkTask.getIsRight())
                .jobName(checkTask.getJobName())
                .completedPartitions(Set.of())
                .created(ProtoConverter.convert(checkTask.getCreated()))
                .type(checkTask.getType())
                .build();
    }

    public static AggregateStatisticsOuterClass.ChunkAggregateStatistics toProtoStatistics(
            ChunkAggregateStatistics statistics
    ) {
        var map = new HashMap<String, AggregateStatisticsOuterClass.ToolchainStatistics>();
        statistics.getToolchains().forEach((key, value) -> map.put(key, toProtoStatistics(value)));
        return AggregateStatisticsOuterClass.ChunkAggregateStatistics.newBuilder()
                .putAllToolchains(map)
                .build();
    }

    private static AggregateStatisticsOuterClass.ToolchainStatistics toProtoStatistics(
            ChunkAggregateStatistics.ToolchainStatistics value
    ) {
        return AggregateStatisticsOuterClass.ToolchainStatistics.newBuilder()
                .setMain(toProtoStatistics(value.getMain()))
                .setExtended(toProtoStatistics(value.getExtended()))
                .build();
    }

    private static AggregateExtendedStatistics toProtoStatistics(ExtendedStatistics value) {
        return AggregateExtendedStatistics.newBuilder()
                .setAdded(toProtoStatistics(value.getAddedOrEmpty()))
                .setDeleted(toProtoStatistics(value.getDeletedOrEmpty()))
                .setFlaky(toProtoStatistics(value.getFlakyOrEmpty()))
                .setMuted(toProtoStatistics(value.getMutedOrEmpty()))
                .setTimeout(toProtoStatistics(value.getTimeoutOrEmpty()))
                .setInternal(toProtoStatistics(value.getInternalOrEmpty()))
                .setExternal(toProtoStatistics(value.getExternalOrEmpty()))
                .build();
    }

    private static AggregateMainStatistics toProtoStatistics(MainStatistics value) {
        return AggregateMainStatistics.newBuilder()
                .setTotal(toProtoStatistics(value.getTotalOrEmpty()))
                .setBuild(toProtoStatistics(value.getBuildOrEmpty()))
                .setConfigure(toProtoStatistics(value.getConfigureOrEmpty()))
                .setStyle(toProtoStatistics(value.getStyleOrEmpty()))
                .setSmallTests(toProtoStatistics(value.getSmallTestsOrEmpty()))
                .setMediumTests(toProtoStatistics(value.getMediumTestsOrEmpty()))
                .setLargeTests(toProtoStatistics(value.getLargeTestsOrEmpty()))
                .setTeTests(toProtoStatistics(value.getTeTestsOrEmpty()))
                .setNativeBuilds(toProtoStatistics(value.getNativeBuildsOrEmpty()))
                .build();
    }

    private static StatisticsGroup toProtoStatistics(StageStatistics value) {
        return StatisticsGroup.newBuilder()
                .setTotal(value.getTotal())
                .setFailed(value.getFailed())
                .setFailedAdded(value.getFailedAdded())
                .setFailedInStrongMode(value.getFailedInStrongMode())
                .setFailedByDeps(value.getFailedByDeps())
                .setFailedByDepsAdded(value.getFailedByDepsAdded())
                .setPassedByDepsAdded(value.getPassedByDepsAdded())
                .setPassed(value.getPassed())
                .setPassedAdded(value.getPassedAdded())
                .setSkipped(value.getSkipped())
                .setSkippedAdded(value.getSkippedAdded())
                .build();
    }

    private static ExtendedStatisticsGroup toProtoStatistics(ExtendedStageStatistics value) {
        return ExtendedStatisticsGroup.newBuilder()
                .setTotal(value.getTotal())
                .setFailedAdded(value.getFailedAdded())
                .setPassedAdded(value.getPassedAdded())
                .build();
    }

    public static ChunkAggregateStatistics toStatistics(
            AggregateStatisticsOuterClass.ChunkAggregateStatistics statistics
    ) {
        var map = new HashMap<String, ChunkAggregateStatistics.ToolchainStatistics>();
        statistics.getToolchainsMap().forEach((key, value) -> map.put(key, toStatistics(value)));
        return new ChunkAggregateStatistics(map);
    }

    private static ChunkAggregateStatistics.ToolchainStatistics toStatistics(
            AggregateStatisticsOuterClass.ToolchainStatistics value
    ) {
        return new ChunkAggregateStatistics.ToolchainStatistics(
                toStatistics(value.getMain()),
                toStatistics(value.getExtended())
        );
    }

    private static ExtendedStatistics toStatistics(AggregateExtendedStatistics value) {
        return ExtendedStatistics.builder()
                .added(toStatistics(value.getAdded()))
                .deleted(toStatistics(value.getDeleted()))
                .flaky(toStatistics(value.getFlaky()))
                .muted(toStatistics(value.getMuted()))
                .timeout(toStatistics(value.getTimeout()))
                .internal(toStatistics(value.getInternal()))
                .external(toStatistics(value.getExternal()))
                .build();
    }

    private static MainStatistics toStatistics(AggregateMainStatistics value) {
        return MainStatistics.builder()
                .total(toStatistics(value.getTotal()))
                .build(toStatistics(value.getBuild()))
                .configure(toStatistics(value.getConfigure()))
                .style(toStatistics(value.getStyle()))
                .smallTests(toStatistics(value.getSmallTests()))
                .mediumTests(toStatistics(value.getMediumTests()))
                .largeTests(toStatistics(value.getLargeTests()))
                .teTests(toStatistics(value.getTeTests()))
                .nativeBuilds(toStatistics(value.getNativeBuilds()))
                .build();
    }

    @Nullable
    private static ExtendedStageStatistics toStatistics(AggregateStatisticsOuterClass.ExtendedStatisticsGroup value) {
        var result = ExtendedStageStatistics.builder()
                .total(value.getTotal())
                .failedAdded(value.getFailedAdded())
                .passedAdded(value.getPassedAdded())
                .build();

        if (result.equals(ExtendedStageStatistics.EMPTY)) {
            return null;
        }

        return result;
    }

    @Nullable
    private static StageStatistics toStatistics(StatisticsGroup value) {
        var result = StageStatistics.builder()
                .total(value.getTotal())
                .failed(value.getFailed())
                .failedAdded(value.getFailedAdded())
                .failedInStrongMode(value.getFailedInStrongMode())
                .failedByDeps(value.getFailedByDeps())
                .failedByDepsAdded(value.getFailedByDepsAdded())
                .passedByDepsAdded(value.getPassedByDepsAdded())
                .passed(value.getPassed())
                .passedAdded(value.getPassedAdded())
                .skipped(value.getSkipped())
                .skippedAdded(value.getSkippedAdded())
                .build();

        if (result.equals(StageStatistics.EMPTY)) {
            return null;
        }

        return result;
    }

    public static ShardOut.ChunkAggregate toProtoAggregate(ChunkAggregateEntity value) {
        return ShardOut.ChunkAggregate.newBuilder()
                .setId(toProtoAggregateId(value.getId()))
                .setCreated(ProtoConverter.convert(value.getCreated()))
                .setStatistics(toProtoStatistics(value.getStatistics()))
                .setState(value.getState())
                .build();
    }

    public static ChunkAggregateEntity toAggregate(ShardOut.ChunkAggregate value) {
        return ChunkAggregateEntity.builder()
                .id(toAggregateId(value.getId()))
                .created(ProtoConverter.convert(value.getCreated()))
                .statistics(toStatistics(value.getStatistics()))
                .state(value.getState())
                .build();
    }

    public static ChunkAggregateEntity.Id toAggregateId(ShardOut.ChunkAggregateId id) {
        return new ChunkAggregateEntity.Id(
                toIterationId(id.getIterationId()),
                ChunkEntity.Id.of(id.getChunkId().getType(), id.getChunkId().getNumber())
        );
    }

    public static AggregateStatisticsOuterClass.AggregateStatistics toProtoStatistics(
            IterationStatistics iterationStatistics, IterationStatistics.ToolchainStatistics toolchain
    ) {
        var technical = iterationStatistics.getTechnical();
        var metrics = iterationStatistics.getMetrics();

        return AggregateStatisticsOuterClass.AggregateStatistics.newBuilder()
                .setMain(toProtoStatistics(toolchain.getMain()))
                .setExtended(toProtoStatistics(toolchain.getExtended()))
                .setTechnical(
                        AggregateStatisticsOuterClass.AggregateTechnicalStatistics.newBuilder()
                                .setTotalNumberOfNodes(metrics.getValueOrElse(Metrics.Name.NUMBER_OF_NODES,
                                        technical.getTotalNumberOfNodes()))
                                .setMachineHours(metrics.getValueOrElse(Metrics.Name.MACHINE_HOURS,
                                        technical.getMachineHours()))
                                .setCacheHit(metrics.getValueOrElse(Metrics.Name.CACHE_HIT, technical.getCacheHit()))
                                .build()
                )
                .addAllMetrics(
                        toProtoMetrics(iterationStatistics.getMetrics())
                )
                .build();
    }

    private static List<Common.Metric> toProtoMetrics(Metrics metrics) {
        return metrics.getValues().entrySet()
                .stream()
                .map(e -> {
                    var metric = e.getValue();
                    return Common.Metric.newBuilder()
                            .setName(e.getKey())
                            .setValue(metric.getValue())
                            .setSize(metric.getSize())
                            .setAggregate(metric.getFunction())
                            .build();
                })
                .toList();
    }

    public static IterationInfo toIterationInfo(@Nullable CheckIteration.IterationInfo info) {
        if (info == null) {
            return IterationInfo.EMPTY;
        }

        return IterationInfo.builder()
                .fastTargets(new HashSet<>(info.getFastTargetsList()))
                .disabledToolchains(new HashSet<>(info.getDisabledToolchainsList()))
                .pessimized(info.getPessimized())
                .pessimizationInfo(info.getPessimizationInfo())
                .advisedPool(info.getAdvisedPool())
                .notRecheckReason(info.getNotRecheckReason())
                .testenvCheckId(info.getTestenvCheckId())
                .fatalErrorInfo(convert(info.getFatalErrorInfo()))
                .progress(info.getProgress())
                .strongModePolicy(toStrongModePolicy(info.getStrongModePolicy()))
                .autodetectedFastCircuit(info.getAutodetectedFastCircuit())
                .ciActionReferences(
                        info.getFlowLaunchNumber() == 0 ?
                                List.of() :
                                List.of(
                                        new CiActionReference(
                                                CiCoreProtoMappers.toFlowFullId(info.getFlowProcessId()),
                                                info.getFlowLaunchNumber()
                                        )
                                )
                )
                .build();
    }

    private static FatalErrorInfo convert(CheckIteration.FatalErrorInfo value) {
        return FatalErrorInfo.builder()
                .message(value.getMessage())
                .details(value.getDetails())
                .sandboxTaskId(value.getSandboxTaskId())
                .build();
    }

    public static CheckIteration.IterationInfo toProtoIterationInfo(@Nullable IterationInfo info) {
        if (info == null) {
            return CheckIteration.IterationInfo.newBuilder().build();
        }

        var builder = CheckIteration.IterationInfo.newBuilder()
                .addAllFastTargets(info.getFastTargets())
                .addAllDisabledToolchains(info.getDisabledToolchains())
                .setPessimized(info.isPessimized())
                .setPessimizationInfo(info.getPessimizationInfo())
                .setNotRecheckReason(info.getNotRecheckReason())
                .setAdvisedPool(info.getAdvisedPool())
                .setFatalErrorMessage(info.getFatalErrorInfo().getMessage())
                .setFatalErrorDetails(info.getFatalErrorInfo().getDetails())
                .setFatalErrorInfo(convert(info.getFatalErrorInfo()))
                .setTestenvCheckId(info.getTestenvCheckId())
                .setProgress(info.getProgress())
                .setStrongModePolicy(
                        toProtoStrongModePolicy(info.getStrongModePolicy())
                )
                .setAutodetectedFastCircuit(info.isAutodetectedFastCircuit());

        if (!info.getCiActionReferences().isEmpty()) {
            // only meta iteration has many references, other iterations has only one reference
            var reference = info.getCiActionReferences().get(0);
            builder.setFlowProcessId(CiCoreProtoMappers.toProtoFlowProcessId(reference.getFlowId()))
                    .setFlowLaunchNumber(reference.getLaunchNumber());
        }

        return builder.build();
    }

    private static CheckIteration.FatalErrorInfo convert(FatalErrorInfo value) {
        return CheckIteration.FatalErrorInfo.newBuilder()
                .setMessage(value.getMessage())
                .setDetails(value.getDetails())
                .setSandboxTaskId(value.getSandboxTaskId())
                .build();
    }

    static StrongModePolicy toStrongModePolicy(CheckIteration.StrongModePolicy policy) {
        return switch (policy) {
            case BY_A_YAML -> StrongModePolicy.BY_A_YAML;
            case FORCE_ON -> StrongModePolicy.FORCE_ON;
            case FORCE_OFF -> StrongModePolicy.FORCE_OFF;
            case UNRECOGNIZED -> throw new IllegalArgumentException(
                    "unexpected strong mode policy " + policy
            );
        };
    }

    static CheckIteration.StrongModePolicy toProtoStrongModePolicy(StrongModePolicy policy) {
        return switch (policy) {
            case BY_A_YAML -> CheckIteration.StrongModePolicy.BY_A_YAML;
            case FORCE_ON -> CheckIteration.StrongModePolicy.FORCE_ON;
            case FORCE_OFF -> CheckIteration.StrongModePolicy.FORCE_OFF;
        };
    }

    public static CheckTaskEntity.Id toTaskId(CheckTaskOuterClass.FullTaskId fullTaskId) {
        return new CheckTaskEntity.Id(toIterationId(fullTaskId.getIterationId()), fullTaskId.getTaskId());
    }

    public static SuiteSearchFilters toSearchFilters(StorageFrontApi.SuiteSearch search) {
        return toSearchFilters(search, StorageFrontApi.SuitePageId.newBuilder().build());
    }

    public static SuiteSearchFilters toSearchFilters(
            StorageFrontApi.SuiteSearch search, StorageFrontApi.SuitePageId pageId
    ) {
        return SuiteSearchFilters.builder()
                .status(search.getStatusFilter())
                .resultTypes(
                        search.getResultTypeList().stream()
                                .map(ResultTypeUtils::toSuiteType)
                                .collect(Collectors.toSet())
                )
                .category(search.getCategory())
                .toolchain(search.getToolchain().isEmpty() ? TestEntity.ALL_TOOLCHAINS : search.getToolchain())
                .path(search.getPath())
                .pageCursor(toPageCursor(pageId))
                .specialCases(search.getSpecialCases())
                .testName(search.getTestName())
                .subtestName(search.getSubtestName())
                .build();
    }

    private static SuitePageCursor toPageCursor(StorageFrontApi.SuitePageId pageId) {
        return new SuitePageCursor(
                pageId.getForwardSuiteId().isEmpty()
                        ? null
                        : toPage(pageId.getForwardSuiteId(), pageId.getForwardPath()),
                pageId.getBackwardSuiteId().isEmpty()
                        ? null
                        : toPage(pageId.getBackwardSuiteId(), pageId.getBackwardPath())

        );
    }

    private static SuitePageCursor.Page toPage(String suiteId, String path) {
        return new SuitePageCursor.Page(UnsignedLongs.decode(suiteId), path);
    }

    public static DiffSearchFilters toSearchFilters(
            StorageFrontApi.DiffsSearch search, StorageFrontApi.DiffPageId pageId
    ) {
        return DiffSearchFilters.builder()
                .resultTypes(new HashSet<>(search.getResultTypeList()))
                .diffTypes(new HashSet<>(search.getDiffTypeList()))
                .toolchain(search.getToolchain().isEmpty()
                        ? TestEntity.ALL_TOOLCHAINS
                        : search.getToolchain())
                .path(search.getPath())
                .name(search.getName())
                .subtestName(search.getSubtestName())
                .tags(new HashSet<>(search.getTagsList()))
                .notificationFilter(search.getNotificationFilter())
                .page(pageId.getId())
                .build();
    }

    public static Common.ChunkId toProtoChunkId(ChunkEntity.Id value) {
        return Common.ChunkId.newBuilder()
                .setType(value.getChunkType())
                .setNumber(value.getNumber())
                .build();
    }

    public static ChunkEntity.Id toChunkId(Common.ChunkId value) {
        return ChunkEntity.Id.of(value.getType(), value.getNumber());
    }

    public static ShardOut.ChunkAggregateId toProtoAggregateId(ChunkAggregateEntity.Id value) {
        return ShardOut.ChunkAggregateId.newBuilder()
                .setIterationId(toProtoIterationId(value.getIterationId()))
                .setChunkId(CheckProtoMappers.toProtoChunkId(value.getChunkId()))
                .build();
    }

    public static StorageFrontApi.SuitePaging toSuitePaging(SuitePageCursor pageCursor) {
        return StorageFrontApi.SuitePaging.newBuilder()
                .setForwardPageId(
                        StorageFrontApi.SuitePageId.newBuilder()
                                .setForwardSuiteId(
                                        pageCursor.getForward() == null ? ""
                                                : UnsignedLongs.toString(pageCursor.getForward().getSuiteId())
                                )
                                .setForwardPath(
                                        pageCursor.getForward() == null ? "" : pageCursor.getForward().getPath()
                                )
                                .build()
                )
                .setBackwardPageId(
                        StorageFrontApi.SuitePageId.newBuilder()
                                .setBackwardSuiteId(
                                        pageCursor.getBackward() == null ? "" :
                                                UnsignedLongs.toString(pageCursor.getBackward().getSuiteId())
                                )
                                .setBackwardPath(
                                        pageCursor.getBackward() == null ? "" : pageCursor.getBackward().getPath())
                                .build()
                )
                .build();
    }

    public static StorageFrontApi.DiffId toProtoDiffId(TestDiffEntity.Id value) {
        return StorageFrontApi.DiffId.newBuilder()
                .setIterationId(toProtoIterationId(value.getIterationId()))
                .setTestId(CheckProtoMappers.toProtoTestId(value.getCombinedTestId()))
                .setResultType(value.getResultType())
                .setPath(value.getPath())
                .build();

    }

    public static Common.TestId toProtoTestId(TestEntity.Id value) {
        return Common.TestId.newBuilder()
                .setSuiteId(value.getSuiteIdString())
                .setToolchain(value.getToolchain())
                .setTestId(value.getIdString())
                .build();
    }


    public static TaskMessages.AutocheckTestId toProtoAutocheckTestId(TestEntity.Id value) {
        return TaskMessages.AutocheckTestId.newBuilder()
                .setSuiteHid(value.getSuiteId())
                .setToolchain(value.getToolchain())
                .setHid(value.getId())
                .build();
    }

    public static TestStatusEntity.Id toTestStatusId(Common.TestStatusId value) {
        return new TestStatusEntity.Id(
                value.getBranch(),
                new TestEntity.Id(
                        UnsignedLongs.parseUnsignedLong(value.getSuiteHid()),
                        value.getToolchain(),
                        UnsignedLongs.parseUnsignedLong(value.getHid())
                )
        );
    }

    public static TestEntity.Id toTestId(TaskMessages.AutocheckTestId value) {
        return new TestEntity.Id(value.getSuiteHid(), value.getToolchain(), value.getHid());
    }

    public static TestEntity.Id toTestId(Common.TestId testId) {
        return TestEntity.Id.parse(testId.getSuiteId(), testId.getToolchain(), testId.getTestId());
    }

    public static TestDiffEntity.Id toDiffId(StorageFrontApi.DiffId id) {
        return new TestDiffEntity.Id(
                toIterationId(id.getIterationId()),
                id.getResultType(),
                id.getPath(),
                toTestId(id.getTestId())
        );
    }

    public static StorageFrontApi.DiffPaging toDiffPaging(DiffPageCursor pageCursor) {
        return StorageFrontApi.DiffPaging.newBuilder()
                .setForwardPageId(
                        StorageFrontApi.DiffPageId.newBuilder()
                                .setId(pageCursor.getForwardPageId())
                                .build()
                )
                .setBackwardPageId(
                        StorageFrontApi.DiffPageId.newBuilder()
                                .setId(pageCursor.getBackwardPageId())
                                .build()
                )
                .build();
    }

    @Nullable
    public static LargeTestsConfig toLargeTestsConfig(CheckOuterClass.LargeConfig config) {
        var path = config.getPath();
        if (!path.isEmpty() && config.hasRevision()) {
            return new LargeTestsConfig(path, toRevision(config.getRevision()));
        } else {
            return null;
        }
    }

    public static CheckOuterClass.LargeConfig toProtoLargeTestsConfig(@Nullable LargeTestsConfig config) {
        if (config == null) {
            return CheckOuterClass.LargeConfig.getDefaultInstance();
        } else {
            return CheckOuterClass.LargeConfig.newBuilder()
                    .setPath(config.getPath())
                    .setRevision(toProtoRevision(config.getRevision()))
                    .build();
        }
    }

    @Nullable
    public static DistbuildPriority toDistbuildPriority(CheckOuterClass.DistbuildPriority priority) {
        if (priority.getPriorityRevision() > 0) {
            return new DistbuildPriority(priority.getFixedPriority(), priority.getPriorityRevision());
        } else {
            return null;
        }
    }

    public static CheckOuterClass.DistbuildPriority toProtoDistbuildPriority(@Nullable DistbuildPriority priority) {
        if (priority == null) {
            return CheckOuterClass.DistbuildPriority.getDefaultInstance();
        } else {
            return CheckOuterClass.DistbuildPriority.newBuilder()
                    .setFixedPriority(priority.getFixedPriority())
                    .setPriorityRevision(priority.getPriorityRevision())
                    .build();
        }
    }

    @Nullable
    public static Zipatch toZipatch(CheckOuterClass.Zipatch zipatch) {
        if (zipatch.getBaseRevision() > 0) {
            return new Zipatch(zipatch.getUrl(), zipatch.getBaseRevision());
        } else {
            return null;
        }
    }

    public static CheckOuterClass.Zipatch toProtoZipatch(@Nullable Zipatch zipatch) {
        if (zipatch == null) {
            return CheckOuterClass.Zipatch.getDefaultInstance();
        } else {
            return CheckOuterClass.Zipatch.newBuilder()
                    .setUrl(zipatch.getUrl())
                    .setBaseRevision(zipatch.getBaseRevision())
                    .build();
        }
    }

    public static List<LargeAutostart> toLargeAutostartList(List<CheckOuterClass.LargeAutostart> list) {
        return list.stream()
                .map(auto -> new LargeAutostart(
                        auto.getPath(),
                        auto.getTarget(),
                        auto.getToolchainsList())
                )
                .toList();
    }

    public static List<CheckOuterClass.LargeAutostart> toProtoLargeAutostart(List<LargeAutostart> list) {
        return list.stream()
                .map(
                        auto -> CheckOuterClass.LargeAutostart.newBuilder()
                                .setPath(Objects.requireNonNullElse(auto.getPath(), ""))
                                .setTarget(auto.getTarget())
                                .addAllToolchains(auto.getToolchains())
                                .build()
                )
                .toList();
    }


    public static List<NativeBuild> toNativeBuildList(List<CheckOuterClass.NativeBuild> list) {
        return list.stream()
                .map(build -> new NativeBuild(
                        build.getPath(),
                        build.getToolchain(),
                        build.getTargetList())
                )
                .toList();
    }

    public static List<CheckOuterClass.NativeBuild> toProtNativeBuild(List<NativeBuild> list) {
        return list.stream()
                .map(
                        build -> CheckOuterClass.NativeBuild.newBuilder()
                                .setPath(build.getPath())
                                .setToolchain(build.getToolchain())
                                .addAllTarget(build.getTargets())
                                .build()
                )
                .toList();
    }

    public static boolean hasDelegatedConfig(DelegatedConfig delegatedConfig) {
        return !delegatedConfig.getPath().isEmpty() &&
                CiCoreProtoMappers.hasOrderedArcRevision(delegatedConfig.getRevision());
    }

    public static LargeTestDelegatedConfig toDelegatedConfig(DelegatedConfig delegatedConfig) {
        return new LargeTestDelegatedConfig(
                delegatedConfig.getPath(),
                CiCoreProtoMappers.toOrderedArcRevision(delegatedConfig.getRevision())
        );
    }

    public static DelegatedConfig toDelegatedConfig(LargeTestDelegatedConfig delegatedConfig) {
        return DelegatedConfig.newBuilder()
                .setPath(delegatedConfig.getPath())
                .setRevision(CiCoreProtoMappers.toProtoOrderedArcRevision(delegatedConfig.getRevision()))
                .build();
    }


    public static StorageFrontHistoryApi.ToolchainStatus toToolchainStatus(TestStatusWithStatistics status) {
        return StorageFrontHistoryApi.ToolchainStatus.newBuilder()
                .setRevision(toProtoRevision(status.getRevision()))
                .setName(status.getStatus().getName())
                .setSubtestName(status.getStatus().getSubtestName())
                .setPath(status.getStatus().getPath())
                .addAllTags(status.getStatus().getTags())
                .setType(status.getStatus().getType())
                .setStatus(status.getStatus().getStatus())
                .setUid(status.getStatus().getUid())
                .setToolchain(status.getStatus().getId().getToolchain())
                .setFailScore(Math.round(100f * status.getStatistics().getStatistics().getFailureScore()))
                .setNotFlakyDays(status.getStatistics().getStatistics().getNoneFlakyDays())
                .setMuted(status.getStatus().isMuted())
                .setLastMuteAction(
                        status.getMute() == null ?
                                StorageFrontHistoryApi.MuteAction.getDefaultInstance() :
                                StorageFrontHistoryApi.MuteAction.newBuilder()
                                        .setRevision(status.getMute().getRevisionNumber())
                                        .setReason(status.getMute().getReason())
                                        .setIterationId(
                                                CheckProtoMappers.toProtoIterationId(status.getMute().getIterationId())
                                        )
                                        .setMuted(status.getMute().isMuted())
                                        .setTimestamp(ProtoConverter.convert(status.getMute().getId().getTimestamp()))
                                        .build()
                )
                .build();
    }

    public static StorageFrontHistoryApi.Revision toProtoRevision(RevisionEntity value) {
        return StorageFrontHistoryApi.Revision.newBuilder()
                .setCommitId(value.getCommitId())
                .setMessage(value.getMessage())
                .setNumber(value.getId().getNumber())
                .setCreated(ProtoConverter.convert(value.getCreated()))
                .setAuthor(value.getAuthor())
                .build();
    }

    public static StorageApi.GetLargeTaskRequest toProtoLargeTaskRequest(LargeTaskEntity.Id largeTaskId) {
        return StorageApi.GetLargeTaskRequest.newBuilder()
                .setId(toProtoLargeTaskEntityId(largeTaskId))
                .build();
    }

    public static Common.TestId toProtoTestId(LargeTaskEntity entity) {
        var testInfo = entity.getLeftLargeTestInfo();
        return Common.TestId.newBuilder()
                .setSuiteId(testInfo.getSuiteHid().toString())
                .setToolchain(testInfo.getToolchain())
                .setTestId(testInfo.getSuiteId())
                .build();
    }

    public static StorageApi.LargeTaskId toProtoLargeTaskEntityId(LargeTaskEntity.Id largeTaskId) {
        return StorageApi.LargeTaskId.newBuilder()
                .setIterationId(CheckProtoMappers.toProtoIterationId(largeTaskId.getIterationId()))
                .setCheckTaskType(largeTaskId.getCheckTaskType())
                .setIndex(largeTaskId.getIndex())
                .build();
    }

    public static LargeTaskEntity.Id toLargeTaskEntityId(StorageApi.LargeTaskId largeTaskId) {
        return new LargeTaskEntity.Id(
                CheckProtoMappers.toIterationId(largeTaskId.getIterationId()),
                largeTaskId.getCheckTaskType(),
                largeTaskId.getIndex()
        );
    }

    public static LargeTaskEntity.Id toLargeTaskEntityId(StorageApi.GetLargeTaskRequest request) {
        if (!request.getId().getIterationId().getCheckId().isEmpty()) {
            return toLargeTaskEntityId(request.getId());
        }
        return new LargeTaskEntity.Id(
                CheckProtoMappers.toIterationId(request.getIterationId()),
                request.getCheckTaskType(),
                request.getIndex()
        );
    }

    public static StorageFrontApi.TestRun toProtoTestRun(TestResultEntity run) {
        var links = new HashMap<String, StorageFrontApi.LinksValue>();

        run.getLinks().forEach(
                (key, value) -> links.put(key, StorageFrontApi.LinksValue.newBuilder().addAllLink(value).build())
        );

        return StorageFrontApi.TestRun.newBuilder()
                .setId(
                        StorageFrontApi.TestRunId.newBuilder()
                                .setIterationId(CheckProtoMappers.toProtoIterationId(run.getId().getIterationId()))
                                .setTestId(CheckProtoMappers.toProtoTestId(run.getId().getFullTestId()))
                                .setTaskId(run.getId().getTaskId())
                                .setNumber(run.getId().getRetryNumber())
                                .setIsRight(run.isRight())
                                .setPartition(run.getId().getPartition())
                                .build()
                )
                .setStatus(run.getStatus())
                .setOldTestId(run.getOldTestId())
                .setSnippet(HtmlSnippetFormatter.format(run.getSnippet()))
                .putAllMetrics(run.getMetrics())
                .putAllLinks(links)
                // temporary solution https://st.yandex-team.ru/DEVTOOLSUI-2069
                .putLinks(
                        "uid: " + run.getUid(),
                        StorageFrontApi.LinksValue.newBuilder()
                                .addLink("https://ci.yandex-team.ru/test/" + run.getOldTestId())
                                .build()
                )
                .setUid(run.getUid())
                .build();
    }

    public static Common.TestStatusId toProtoTestStatusId(TestStatusEntity.Id testId) {
        return toProtoTestStatusId(testId.getBranch(), testId.getFullTestId());
    }

    public static Common.TestStatusId toProtoTestStatusId(String branch, TestEntity.Id testId) {
        return Common.TestStatusId.newBuilder()
                .setBranch(branch)
                .setHid(UnsignedLongs.toString(testId.getId()))
                .setToolchain(testId.getToolchain())
                .setSuiteHid(UnsignedLongs.toString(testId.getSuiteId()))
                .build();
    }

    public static EventsStreamMessages.CancelMessage toProtoCancelMessage(CheckIterationEntity.Id id) {
        return EventsStreamMessages.CancelMessage.newBuilder()
                .setIterationId(toProtoIterationId(id))
                .build();
    }
}
