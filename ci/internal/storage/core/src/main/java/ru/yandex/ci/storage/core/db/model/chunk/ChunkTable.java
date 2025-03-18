package ru.yandex.ci.storage.core.db.model.chunk;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import yandex.cloud.repository.kikimr.table.KikimrTable.QueryExecutor;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;

@Slf4j
public class ChunkTable extends KikimrTableCi<ChunkEntity> {
    public ChunkTable(QueryExecutor executor) {
        super(ChunkEntity.class, executor);
    }

    public List<ChunkEntity> readAll() {
        return this.readTable().collect(Collectors.toList());
    }

    public void createInitialDistribution(
            int numberOfPartitions, ShardingSettings shardingSettings
    ) {
        log.info(
                "Creating chunk distribution, partitions: {}, settings: {}",
                numberOfPartitions, shardingSettings
        );

        var chunks = new ArrayList<ChunkEntity>();
        var partition = new AtomicInteger();
        chunks.addAll(
                generateChunks(
                        Common.ChunkType.CT_CONFIGURE,
                        shardingSettings.getConfigureChunks(),
                        numberOfPartitions,
                        partition
                )
        );

        chunks.addAll(
                generateChunks(
                        Common.ChunkType.CT_BUILD,
                        shardingSettings.getBuildChunks(),
                        numberOfPartitions,
                        partition
                )
        );

        chunks.addAll(
                generateChunks(
                        Common.ChunkType.CT_STYLE,
                        shardingSettings.getStyleChunks(),
                        numberOfPartitions,
                        partition
                )
        );

        chunks.addAll(
                generateChunks(
                        Common.ChunkType.CT_SMALL_TEST,
                        shardingSettings.getSmallTestChunks(),
                        numberOfPartitions,
                        partition
                )
        );

        chunks.addAll(
                generateChunks(
                        Common.ChunkType.CT_MEDIUM_TEST,
                        shardingSettings.getMediumTestChunks(),
                        numberOfPartitions,
                        partition
                )
        );

        chunks.addAll(
                generateChunks(
                        Common.ChunkType.CT_LARGE_TEST,
                        shardingSettings.getLargeTestChunks(),
                        numberOfPartitions,
                        partition
                )
        );

        chunks.addAll(
                generateChunks(
                        Common.ChunkType.CT_TESTENV,
                        shardingSettings.getLargeTestChunks(),
                        numberOfPartitions,
                        partition
                )
        );

        chunks.addAll(
                generateChunks(
                        Common.ChunkType.CT_NATIVE_BUILD,
                        shardingSettings.getNativeBuildsChunks(),
                        numberOfPartitions,
                        partition
                )
        );

        this.bulkUpsert(chunks, Integer.MAX_VALUE);
    }

    private List<ChunkEntity> generateChunks(
            Common.ChunkType chunkType, int numberOfChunks, int numberOfPartitions, AtomicInteger partition
    ) {
        return IntStream.rangeClosed(1, numberOfChunks).mapToObj(i -> {
            var chunk = new ChunkEntity(
                    ChunkEntity.Id.of(chunkType, i),
                    Common.ChunkStatus.CS_DIRECT,
                    partition.get() % numberOfPartitions
            );
            partition.incrementAndGet();
            return chunk;
        }).collect(Collectors.toList());
    }

    public List<ChunkEntity> getByType(Common.ChunkType chunkType) {
        return this.readTable(
                ReadTableParams.<ChunkEntity.Id>builder()
                        .fromKey(ChunkEntity.Id.of(chunkType, 1))
                        .toKey(ChunkEntity.Id.of(chunkType, Integer.MAX_VALUE))
                        .toInclusive(true)
                        .fromInclusive(true)
                        .ordered()
                        .build()
        ).collect(Collectors.toList());
    }
}
