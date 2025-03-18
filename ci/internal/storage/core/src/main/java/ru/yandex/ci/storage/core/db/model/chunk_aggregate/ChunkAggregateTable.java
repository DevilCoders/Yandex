package ru.yandex.ci.storage.core.db.model.chunk_aggregate;

import java.time.Duration;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.stream.Collectors;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;

public class ChunkAggregateTable extends KikimrTableCi<ChunkAggregateEntity> {

    private final Common.ChunkType lastChunkType;

    public ChunkAggregateTable(QueryExecutor executor) {
        super(ChunkAggregateEntity.class, executor);

        this.lastChunkType = Arrays.stream(Common.ChunkType.values()).max(Comparator.naturalOrder()).orElseThrow();
    }

    public List<ChunkAggregateEntity> findByIterationId(CheckIterationEntity.Id id) {
        return this.readTable(
                ReadTableParams.<ChunkAggregateEntity.Id>builder()
                        .fromKey(new ChunkAggregateEntity.Id(id, null))
                        .toKey(
                                new ChunkAggregateEntity.Id(
                                        id, ChunkEntity.Id.of(lastChunkType, Integer.MAX_VALUE)
                                )
                        )
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .timeout(Duration.ofMinutes(5))
                        .build()
        ).collect(Collectors.toList());
    }

    public List<ChunkAggregateEntity.Id> findIdsByCheckId(CheckEntity.Id id) {
        return this.readTableIds(
                ReadTableParams.<ChunkAggregateEntity.Id>builder()
                        .fromKey(
                                new ChunkAggregateEntity.Id(
                                        new CheckIterationEntity.Id(id, Integer.MIN_VALUE, Integer.MIN_VALUE), null
                                )
                        )
                        .toKey(
                                new ChunkAggregateEntity.Id(
                                        new CheckIterationEntity.Id(id, Integer.MAX_VALUE, Integer.MAX_VALUE), null
                                )
                        )
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .build()
        ).collect(Collectors.toList());
    }
}
