package ru.yandex.ci.observer.reader.proto;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.proto.CiCoreProtoMappers;
import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.LinkName;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common.StorageAttribute;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.utils.YandexUrls;

public class ObserverProtoMappers {
    private ObserverProtoMappers() {
    }

    public static CheckEntity.Id toCheckId(String checkId) {
        return CheckEntity.Id.of(checkId);
    }

    public static CheckIterationEntity.Id toIterationId(CheckIteration.IterationId iterationId) {
        return new CheckIterationEntity.Id(
                toCheckId(iterationId.getCheckId()),
                iterationId.getCheckType(),
                iterationId.getNumber()
        );
    }

    public static CheckTaskEntity.Id toTaskId(CheckTaskOuterClass.FullTaskId taskId) {
        return new CheckTaskEntity.Id(toIterationId(taskId.getIterationId()), taskId.getTaskId());
    }

    public static CheckEntity toCheck(CheckOuterClass.Check check) {
        var left = check.getLeftRevision();
        var right = check.getRightRevision();

        return CheckEntity.builder()
                .id(CheckEntity.Id.of(check.getId()))
                .tags(Set.copyOf(check.getTagsList()))
                .left(CheckProtoMappers.toRevision(left))
                .right(CheckProtoMappers.toRevision(right))
                .author(check.getOwner())
                .diffSetId(check.getDiffSetId())
                .created(ProtoConverter.convert(check.getCreated()))
                .type(check.getType())
                .status(check.getStatus())
                .diffSetEventCreated(ProtoConverter.convert(check.getDiffSetEventCreated()))
                .build();
    }

    public static CheckIterationEntity toIteration(
            CheckOuterClass.Check check,
            CheckIteration.Iteration iteration
    ) {
        Integer parentIterationNumber = (
                iteration.getId().getNumber() > 1
                        && CheckIterationEntity.ITERATION_TYPES_WITH_RECHECK.contains(iteration.getId().getCheckType())
        ) ? iteration.getId().getNumber() - 1 : null;

        return CheckIterationEntity.builder()
                .id(toIterationId(iteration.getId()))
                .checkType(check.getType())
                .author(check.getOwner())
                .left(CheckProtoMappers.toRevision(check.getLeftRevision()))
                .right(CheckProtoMappers.toRevision(check.getRightRevision()))
                .pessimized(iteration.getInfo().getPessimized())
                .stressTest(getStressTest(check))
                .advisedPool(iteration.getInfo().getAdvisedPool())
                .created(ProtoConverter.convert(iteration.getCreated()))
                .testenvId(iteration.getInfo().getTestenvCheckId())
                .checkRelatedLinks(
                        createCheckRelatedLinks(check, iteration.getId(), iteration.getInfo())
                )
                .expectedCheckTasks(toTaskKeys(iteration.getExpectedTasksList()))
                .parentIterationNumber(parentIterationNumber)
                .diffSetEventCreated(ProtoConverter.convert(check.getDiffSetEventCreated()))
                .build();
    }

    private static boolean getStressTest(CheckOuterClass.Check check) {
        var stressTest = check.getAttributesMap().get(StorageAttribute.SA_STRESS_TEST.name());
        return Boolean.parseBoolean(stressTest);
    }

    public static CheckTaskEntity toTask(@Nonnull CheckTaskOuterClass.CheckTask checkTask,
                                         @Nonnull CheckOuterClass.CheckType checkType,
                                         @Nonnull StorageRevision right) {
        return CheckTaskEntity.builder()
                .id(toTaskId(checkTask.getId()))
                .checkType(checkType)
                .numberOfPartitions(checkTask.getNumberOfPartitions())
                .status(checkTask.getStatus())
                .right(checkTask.getIsRight())
                .jobName(checkTask.getJobName())
                .completedPartitions(Set.of())
                .created(ProtoConverter.convert(checkTask.getCreated()))
                .rightRevisionTimestamp(right.getTimestamp())
                .build();
    }

    public static List<CheckIterationEntity.CheckTaskKey> toTaskKeys(
            List<CheckIteration.ExpectedTask> expectedTasks
    ) {
        return expectedTasks.stream()
                .map(t -> new CheckIterationEntity.CheckTaskKey(t.getJobName(), t.getRight()))
                .collect(Collectors.toList());
    }

    public static String toProtoCheckId(CheckEntity.Id id) {
        return id.getId().toString();
    }

    public static CheckIteration.IterationId toProtoIterationId(CheckIterationEntity.Id id) {
        return CheckIteration.IterationId.newBuilder()
                .setCheckId(id.getCheckId().getId().toString())
                .setCheckType(id.getIterType())
                .setNumber(id.getNumber())
                .build();
    }

    public static CheckTaskOuterClass.FullTaskId toProtoTaskId(CheckTaskEntity.Id id) {
        return CheckTaskOuterClass.FullTaskId.newBuilder()
                .setIterationId(toProtoIterationId(id.getIterationId()))
                .setTaskId(id.getTaskId())
                .build();
    }

    public static CheckOuterClass.Check toProtoCheck(CheckEntity entity) {
        return CheckOuterClass.Check.newBuilder()
                .setLeftRevision(CheckProtoMappers.toProtoRevision(entity.getLeft()))
                .setRightRevision(CheckProtoMappers.toProtoRevision(entity.getRight()))
                .setDiffSetId(entity.getDiffSetId())
                .setId(entity.getId().getId().toString())
                .setOwner(entity.getAuthor())
                .addAllTags(entity.getTags())
                .setCreated(ProtoConverter.convert(entity.getCreated()))
                .setType(entity.getType())
                .setStatus(entity.getStatus())
                .build();
    }

    public static CheckIteration.Iteration toProtoIteration(CheckIterationEntity iteration) {
        return CheckIteration.Iteration.newBuilder()
                .setId(toProtoIterationId(iteration.getId()))
                .setCreated(ProtoConverter.convert(iteration.getCreated()))
                .setStatus(iteration.getStatus())
                .setInfo(toIterationInfo(iteration))
                .addAllExpectedTasks(toExpectedTasks(iteration.getExpectedCheckTasks()))
                .build();
    }

    public static CheckTaskOuterClass.CheckTask toProtoTask(CheckTaskEntity checkTask) {
        return CheckTaskOuterClass.CheckTask.newBuilder()
                .setId(toProtoTaskId(checkTask.getId()))
                .setNumberOfPartitions(checkTask.getNumberOfPartitions())
                .setStatus(checkTask.getStatus())
                .setIsRight(checkTask.isRight())
                .setJobName(checkTask.getJobName())
                .setCreated(ProtoConverter.convert(checkTask.getCreated()))
                .build();
    }

    public static List<CheckIteration.ExpectedTask> toExpectedTasks(
            List<CheckIterationEntity.CheckTaskKey> taskKeys
    ) {
        return taskKeys.stream()
                .map(t -> CheckIteration.ExpectedTask.newBuilder()
                        .setJobName(t.getJobName())
                        .setRight(t.isRight())
                        .build()
                )
                .collect(Collectors.toList());
    }

    public static CheckIteration.IterationInfo toIterationInfo(CheckIterationEntity iteration) {
        return CheckIteration.IterationInfo.newBuilder()
                .setAdvisedPool(iteration.getAdvisedPool())
                .setPessimized(iteration.isPessimized())
                .setTestenvCheckId(Objects.requireNonNullElse(iteration.getTestenvId(), ""))
                .build();
    }

    public static TechnicalStatistics toTechnicalStatistics(Internal.TechnicalStatistics statistics) {
        return new TechnicalStatistics(
                statistics.getMachineHours(), statistics.getNumberOfNodes(), statistics.getCacheHit()
        );
    }

    public static Map<String, String> createCheckRelatedLinks(
            CheckOuterClass.Check check,
            CheckIteration.IterationId iterationId,
            CheckIteration.IterationInfo iterationInfo
    ) {
        var testenvId = iterationInfo.getTestenvCheckId();
        var checkId = CheckEntity.Id.of(check.getId()).getId();

        var links = new HashMap<>(Map.of(
                LinkName.CI_BADGE,
                YandexUrls.ciBadgeUrl(checkId),
                LinkName.SANDBOX,
                testenvId.isEmpty()
                        ? YandexUrls.autocheckSandboxTasksUrl(CheckProtoMappers.toIterationId(iterationId))
                        : YandexUrls.testenvSandboxTasksUrl(testenvId),
                LinkName.DISTBUILD_VIEWER,
                testenvId.isEmpty()
                        ? YandexUrls.distbuildViewerUrl(checkId.toString())
                        : YandexUrls.distbuildViewerUrl(testenvId)
        ));

        var reviewId = check.getType().equals(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT) ||
                check.getType().equals(CheckOuterClass.CheckType.BRANCH_PRE_COMMIT) ?
                ArcBranch.ofString(check.getRightRevision().getBranch()).getPullRequestId() : null;

        if (reviewId != null) {
            links.put(LinkName.DISTBUILD, YandexUrls.distbuildProfilerUrl(reviewId));
            links.put(LinkName.REVIEW, YandexUrls.reviewUrl(reviewId));
        }

        if (iterationInfo.hasFlowProcessId()) {
            links.put(
                    LinkName.createFlowLinkName(iterationId),
                    YandexUrls.autocheckFlow(
                            CiCoreProtoMappers.toFlowFullId(iterationInfo.getFlowProcessId()),
                            iterationInfo.getFlowLaunchNumber()
                    )
            );
        }

        return links;
    }
}
