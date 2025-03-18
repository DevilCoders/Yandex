package ru.yandex.ci.storage.shard.message;

import java.util.List;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;

@Value
public class PostProcessMessage {
    ChunkEntity.Id chunkId;
    List<TestResult> results;
    List<TestDiffByHashEntity> diffs;

    @Nullable
    Runnable commitCallback;
}
