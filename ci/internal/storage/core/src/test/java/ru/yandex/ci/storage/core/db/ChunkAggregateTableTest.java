package ru.yandex.ci.storage.core.db;

import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;

import static org.assertj.core.api.Assertions.assertThat;

public class ChunkAggregateTableTest extends StorageYdbTestBase {

    @Test
    void loadsByIteration() {
        var chunks = List.of(
                ChunkEntity.Id.of(Common.ChunkType.CT_BUILD, 1),
                ChunkEntity.Id.of(Common.ChunkType.CT_CONFIGURE, 2),
                ChunkEntity.Id.of(Common.ChunkType.CT_STYLE, 3),
                ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 3),
                ChunkEntity.Id.of(Common.ChunkType.CT_MEDIUM_TEST, 3),
                ChunkEntity.Id.of(Common.ChunkType.CT_LARGE_TEST, 3)
        );

        this.db.currentOrTx(() -> {
            chunks.forEach(chunk -> this.db.chunkAggregates().save(
                    ChunkAggregateEntity.builder().id(new ChunkAggregateEntity.Id(sampleIterationId, chunk)).build()
            ));
        });

        this.db.readOnly().run(() -> {
            var result = this.db.chunkAggregates().findByIterationId(sampleIterationId);
            assertThat(result).hasSize(6);
        });
    }
}

