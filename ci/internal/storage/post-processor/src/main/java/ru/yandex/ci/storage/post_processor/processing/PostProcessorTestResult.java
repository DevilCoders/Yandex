package ru.yandex.ci.storage.post_processor.processing;

import java.time.Instant;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.task_result.ResultOwners;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;

@Value
@Builder
public class PostProcessorTestResult {
    @Nonnull
    TestResultEntity.Id id;

    @Nullable
    Long autocheckChunkId;

    @Nonnull
    String oldSuiteId;

    @Nonnull
    String oldTestId;

    @Nonnull
    Map<String, Double> metrics;

    @Nonnull
    String name;

    @Nonnull
    String branch;

    @Nonnull
    String path;

    @Nonnull
    String subtestName;

    @Nonnull
    String service;

    @Nonnull
    Set<String> tags;

    @Nonnull
    String uid;

    @Nonnull
    ResultOwners owners;

    String revision;

    long revisionNumber;

    boolean isRight;

    @Nonnull
    Instant created;

    @Nonnull
    Common.TestStatus status;

    @Nonnull
    Common.ResultType resultType;
}
