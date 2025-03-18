package ru.yandex.ci.storage.core.db;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.ResultType;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;

import static org.assertj.core.api.Assertions.assertThat;

public class TestDiffTableByHashTest extends StorageYdbTestBase {
    ChunkAggregateEntity.Id firstAggregateId = new ChunkAggregateEntity.Id(
            sampleIterationId, ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1)
    );

    ChunkAggregateEntity.Id secondAggregateId = new ChunkAggregateEntity.Id(
            sampleIterationId, ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 2)
    );

    @Test
    void getsByChunk() {
        this.db.currentOrTx(() -> {
            this.db.testDiffsByHash().save(
                    TestDiffByHashEntity.builder()
                            .id(TestDiffByHashEntity.Id.of(firstAggregateId, new TestEntity.Id(1L, "a", 1L)))
                            .resultType(ResultType.RT_BUILD)
                            .build()
            );

            this.db.testDiffsByHash().save(
                    TestDiffByHashEntity.builder()
                            .id(TestDiffByHashEntity.Id.of(firstAggregateId, new TestEntity.Id(2L, "Z", 2L)))
                            .resultType(ResultType.RT_BUILD)
                            .build()
            );

            this.db.testDiffsByHash().save(
                    TestDiffByHashEntity.builder()
                            .id(TestDiffByHashEntity.Id.of(firstAggregateId, new TestEntity.Id(3L, "z", 3L)))
                            .resultType(ResultType.RT_BUILD)
                            .build()
            );

            this.db.testDiffsByHash().save(
                    TestDiffByHashEntity.builder()
                            .id(TestDiffByHashEntity.Id.of(secondAggregateId, new TestEntity.Id(1L, "a", 1L)))
                            .resultType(ResultType.RT_BUILD)
                            .build()
            );
        });

        this.db.readOnly().run(() -> {
            var chunk = this.db.testDiffsByHash().getChunk(firstAggregateId);
            assertThat(chunk).hasSize(3);
        });
    }
}
