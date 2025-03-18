package ru.yandex.ci.storage.post_processor.proto;

import java.util.HashSet;
import java.util.List;
import java.util.stream.Collectors;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.storage.core.PostProcessor;
import ru.yandex.ci.storage.core.db.model.task_result.ResultOwners;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.post_processor.processing.PostProcessorTestResult;

public class PostProcessorProtoMappers {
    private PostProcessorProtoMappers() {

    }

    public static List<PostProcessorTestResult> convert(List<PostProcessor.ResultForPostProcessor> results) {
        return results.stream().map(PostProcessorProtoMappers::convert).collect(Collectors.toList());
    }

    private static PostProcessorTestResult convert(PostProcessor.ResultForPostProcessor result) {
        var taskId = CheckProtoMappers.toTaskId(result.getFullTaskId());

        return PostProcessorTestResult.builder()
                .id(
                        new TestResultEntity.Id(
                                taskId.getIterationId(),
                                TestEntity.Id.of(result.getId()),
                                taskId.getTaskId(),
                                result.getAutocheckPartition(),
                                0
                        )
                )
                .autocheckChunkId(result.getChunkHid())
                .oldSuiteId(result.getOldSuiteId())
                .oldTestId(result.getOldId())
                .metrics(result.getMetricsMap())
                .name(result.getName())
                .branch(result.getBranch())
                .path(result.getPath())
                .subtestName(result.getSubtestName())
                .tags(new HashSet<>(result.getTagsList()))
                .uid(result.getUid())
                .revision(result.getRevision())
                .revisionNumber(result.getRevisionNumber())
                .created(ProtoConverter.convert(result.getCreated()))
                .status(result.getTestStatus())
                .resultType(result.getResultType())
                .owners(new ResultOwners(result.getOwners().getLoginsList(), result.getOwners().getGroupsList()))
                .service(result.getService())
                .isRight(result.getIsRight())
                .build();
    }

}
