package ru.yandex.ci.storage.core.proto;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.storage.api.StoragePublicApi;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;

import static ru.yandex.ci.storage.core.proto.CheckProtoMappers.toProtoIterationId;
import static ru.yandex.ci.storage.core.proto.CheckProtoMappers.toProtoIterationInfo;
import static ru.yandex.ci.storage.core.proto.CheckProtoMappers.toProtoRevision;

public class PublicProtoMappers {
    private PublicProtoMappers() {

    }

    public static StoragePublicApi.CheckPublicViewModel toProtoCheck(CheckEntity entity) {
        return StoragePublicApi.CheckPublicViewModel.newBuilder()
                .setLeftRevision(toProtoRevision(entity.getLeft()))
                .setRightRevision(toProtoRevision(entity.getRight()))
                .setDiffSetId(entity.getDiffSetId())
                .setId(entity.getId().getId())
                .setOwner(entity.getAuthor())
                .setCreated(ProtoConverter.convert(entity.getCreated()))
                .setType(entity.getType())
                .setStatus(entity.getStatus())
                .setArchiveState(entity.getArchiveState())
                .build();
    }

    public static StoragePublicApi.IterationPublicViewModel toIteration(CheckIterationEntity iteration) {
        return StoragePublicApi.IterationPublicViewModel.newBuilder()
                .setId(toProtoIterationId(iteration.getId()))
                .setStatus(iteration.getStatus())
                .setCreated(ProtoConverter.convert(iteration.getCreated()))
                .setStart(ProtoConverter.convert(iteration.getStart()))
                .setFinish(ProtoConverter.convert(iteration.getFinish()))
                .setInfo(toProtoIterationInfo(iteration.getInfo()))
                .setStatistics(
                        CheckProtoMappers.toProtoStatistics(
                                iteration.getStatistics(),
                                iteration.getStatistics().getAllToolchain()
                        )
                )
                .build();
    }

    public static StoragePublicApi.TestResultPublicViewModel toProtoResult(TestResultEntity result) {
        return StoragePublicApi.TestResultPublicViewModel.newBuilder()
                .setHid(result.getId().getTestId())
                .setSuiteHid(result.getId().getSuiteId())
                .setToolchain(result.getId().getToolchain())
                .setBranch(result.getBranch())
                .setRevisionNumber(result.getRevisionNumber())
                .setAutocheckPartition(result.getId().getPartition())
                .setResultType(result.getResultType())
                .setTestStatus(result.getStatus())
                .setUid(result.getUid())
                .setPath(result.getPath())
                .setName(result.getName())
                .setSubtestName(result.getSubtestName())
                .addAllTags(result.getTags())
                .putAllMetrics(result.getMetrics())
                .setCreated(ProtoConverter.convert(result.getCreated()))
                .setSource(
                        StoragePublicApi.TestResultPublicViewModel.ResultSource.newBuilder()
                                .setIterationId(CheckProtoMappers.toProtoIterationId(result.getId().getIterationId()))
                                .setTaskId(result.getId().getTaskId())
                                .setTaskIsRight(result.isRight())
                                .build()
                )
                .build();
    }
}
