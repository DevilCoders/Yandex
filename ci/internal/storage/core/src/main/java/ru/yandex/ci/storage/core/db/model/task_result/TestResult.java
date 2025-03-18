package ru.yandex.ci.storage.core.db.model.task_result;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Builder.Default;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;

@Value
@AllArgsConstructor
@Builder(toBuilder = true)
public class TestResult {
    @Nonnull
    TestResultEntity.Id id;

    @Nonnull
    ChunkEntity.Id chunkId;

    @Nullable
    Long autocheckChunkId;

    @Default
    String oldSuiteId = "";

    @Default
    String oldTestId = "";

    @Default
    Map<String, List<String>> links = Map.of();

    @Default
    Map<String, Double> metrics = Map.of();

    @Default
    Map<String, TestOutput> testOutputs = Map.of();

    @Default
    String name = "";

    @Default
    String requirements = "";

    @Default
    String branch = "";

    @Default
    String path = "";

    @Default
    String snippet = "";

    @Default
    String subtestName = "";

    @Default
    String processedBy = "";

    @Default
    String strongModeAYaml = "";

    @Default
    String service = "";

    @Default
    Set<String> tags = Set.of();

    @Default
    ResultOwners owners = ResultOwners.EMPTY;

    @Default
    String uid = "";

    String revision;
    long revisionNumber;

    boolean isRight;
    boolean isStrongMode;
    boolean isOwner;
    @Nullable
    Boolean isLaunchable;

    @Default
    Instant created = Instant.now();

    @Default
    TestStatus status = TestStatus.TS_UNKNOWN;

    @Default
    Common.ResultType resultType = Common.ResultType.RT_BUILD;

    public TestEntity.Id getTestId() {
        return id.getFullTestId();
    }

    public boolean isLeft() {
        return !isRight;
    }

    public TestDiffEntity.Id getDiffId() {
        return new TestDiffEntity.Id(this.id.getIterationId(), this.resultType, this.path, this.id.getFullTestId());
    }
}
