package ru.yandex.ci.storage.core.large;

import java.util.Optional;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.util.CiJson;

@Slf4j
@RequiredArgsConstructor
public class AutocheckTasksFactory {

    @Nonnull
    private final StorageCoreCache<?> storageCache;

    public StorageApi.GetLargeTaskResponse loadLargeTask(StorageApi.GetLargeTaskRequest request) {
        var largeTaskId = CheckProtoMappers.toLargeTaskEntityId(request);
        log.info("Loading Large Task configuration: {}", largeTaskId);

        var largeTask = storageCache.largeTasks().getOrThrow(largeTaskId);
        var check = storageCache.checks().getOrThrow(largeTaskId.getIterationId().getCheckId());

        return buildResponse(largeTask, check);
    }

    private StorageApi.GetLargeTaskResponse buildResponse(LargeTaskEntity largeTask, CheckEntity check) {
        var response = StorageApi.GetLargeTaskResponse.newBuilder();
        Optional.ofNullable(buildJob(false, largeTask, check))
                .ifPresent(response::addJobs);
        Optional.ofNullable(buildJob(true, largeTask, check))
                .ifPresent(response::addJobs);
        return response.build();
    }

    @Nullable
    private StorageApi.LargeTestJob buildJob(boolean right, LargeTaskEntity largeTask, CheckEntity check) {
        var checkTaskId = right ? largeTask.toRightTaskId() : largeTask.toLeftTaskId();

        if (checkTaskId == null) {
            return null;
        }

        var job = StorageApi.LargeTestJob.newBuilder();
        var checkType = check.getType();

        job.setId(CheckProtoMappers.toProtoCheckTaskId(checkTaskId));
        job.setCheckType(checkType);

        var checkTaskType = largeTask.getId().getCheckTaskType();
        job.setTitle("%s %s".formatted(
                LargeStartService.matchCheckTaskType(checkTaskType, "Large", "Native Build"),
                right ? "Right" : "Left")
        );
        job.setRight(right);
        job.setPrecommit(LargeStartService.isPrecommit(checkType));
        job.setTarget(largeTask.getTarget());

        Optional.ofNullable(largeTask.getNativeTarget()).ifPresent(job::setNativeTarget);

        Optional.ofNullable(largeTask.getNativeSpecification()).ifPresent(
                specification -> job.setNativeSpecification(CiJson.writeValueAsString(specification))
        );

        var testInfoSource = right
                ? largeTask.getRightLargeTestInfo()
                : largeTask.getLeftLargeTestInfo();

        job.setTestInfo(CiJson.writeValueAsString(testInfoSource));

        job.setTestInfoSource(StorageApi.LargeTestJob.TestInfo.newBuilder()
                .setToolchain(testInfoSource.getToolchain())
                .addAllTags(testInfoSource.getTags())
                .setSuiteName(testInfoSource.getSuiteName())
                .setSuiteId(testInfoSource.getSuiteId())
                .setSuiteHid(testInfoSource.getSuiteHid().longValue())
        );

        Optional.ofNullable(largeTask.getStartedBy()).ifPresent(job::setStartedBy);

        job.setCheckTaskType(checkTaskType);

        var rev = right ? check.getRight() : check.getLeft();
        job.setArcadiaUrl("arcadia-arc:/#" + rev.getRevision());

        var zipatch = check.getZipatch();
        if (right && zipatch != null) {
            job.setArcadiaBase(String.valueOf(zipatch.getBaseRevision()));
            job.setArcadiaPatch(zipatch.getUrl());
        } else {
            job.setArcadiaBase(String.valueOf(rev.getRevisionNumber()));
        }

        Optional.ofNullable(check.getDistbuildPriority())
                .map(CheckProtoMappers::toProtoDistbuildPriority)
                .ifPresent(job::setDistbuildPriority);

        return job.build();
    }
}
