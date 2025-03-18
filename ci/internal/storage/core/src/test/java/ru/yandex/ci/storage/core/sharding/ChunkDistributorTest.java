package ru.yandex.ci.storage.core.sharding;

import java.nio.charset.StandardCharsets;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import java.util.stream.Stream;

import com.google.common.io.Resources;
import com.google.common.math.Quantiles;
import com.google.common.primitives.UnsignedLongs;
import lombok.extern.slf4j.Slf4j;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.ValueSource;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common.ChunkType;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGenerator;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

@Slf4j
@SuppressWarnings("UnstableApiUsage")
class ChunkDistributorTest {
    @Test
    void distributeV2() {
        var shardingSettings = new ShardingSettings(16, 32, 64, 32, 32, 32, 32, 32, -1, -1);
        var distributor = new ChunkDistributor(mock(CiStorageDb.class), shardingSettings, 1);

        var hids = List.of(
                372119804856956570L,
                372119804856956570L,
                612390585551798937L,
                1234391359776240056L,
                UnsignedLongs.parseUnsignedLong("12782959453053718350"),
                UnsignedLongs.parseUnsignedLong("14345684857426354245"),
                2401902423386636885L

        );

        var shift = 0;

        Assertions.assertThat(distributor.getId(hids.get(0), ChunkType.CT_CONFIGURE, shift, shardingSettings))
                .isEqualTo(ChunkEntity.Id.of(ChunkType.CT_CONFIGURE, 11));

        Assertions.assertThat(distributor.getId(hids.get(1), ChunkType.CT_BUILD, shift, shardingSettings))
                .isEqualTo(ChunkEntity.Id.of(ChunkType.CT_BUILD, 27));

        Assertions.assertThat(distributor.getId(hids.get(2), ChunkType.CT_SMALL_TEST, shift, shardingSettings))
                .isEqualTo(ChunkEntity.Id.of(ChunkType.CT_SMALL_TEST, 26));

        Assertions.assertThat(distributor.getId(hids.get(3), ChunkType.CT_MEDIUM_TEST, shift, shardingSettings))
                .isEqualTo(ChunkEntity.Id.of(ChunkType.CT_MEDIUM_TEST, 25));

        Assertions.assertThat(distributor.getId(hids.get(4), ChunkType.CT_LARGE_TEST, shift, shardingSettings))
                .isEqualTo(ChunkEntity.Id.of(ChunkType.CT_LARGE_TEST, 15));

        Assertions.assertThat(distributor.getId(hids.get(5), ChunkType.CT_STYLE, shift, shardingSettings))
                .isEqualTo(ChunkEntity.Id.of(ChunkType.CT_STYLE, 6));

        Assertions.assertThat(distributor.getId(hids.get(6), ChunkType.CT_TESTENV, shift, shardingSettings))
                .isEqualTo(ChunkEntity.Id.of(ChunkType.CT_TESTENV, 22));
    }

    @ParameterizedTest
    @ValueSource(ints = {8, 16, 32, 64, 128, 256})
    void fairDistribution(int chunksCount) throws Exception {
        var ids = Resources.readLines(
                Resources.getResource("sharding/test_hids.tsv"),
                StandardCharsets.UTF_8
        ).stream().map(Long::parseLong).collect(Collectors.toSet());

        int expectedIdsPerChunk = ids.size() / chunksCount;

        var chunkCounters = new HashMap<Integer, Integer>();

        var iterationId = new CheckIterationEntity.Id(new CheckEntity.Id(CheckEntity.ID_START + 1), 1, 1);
        var chunkShift = iterationId.generateChunkShift();

        for (var id : ids) {
            var chunkNumber = ChunkDistributor.getChunkNumber(id, chunksCount, chunkShift);
            chunkCounters.put(chunkNumber, chunkCounters.getOrDefault(chunkNumber, 0) + 1);
        }

        Assertions.assertThat(chunkCounters.size()).isEqualTo(chunksCount);

        //Чем больше чанков, тем больше позволяем отклонение
        var percentCoefficient = Math.log(chunksCount);

        assertDeviation(0, chunkCounters.values(), expectedIdsPerChunk, 5 * percentCoefficient);
        assertDeviation(5, chunkCounters.values(), expectedIdsPerChunk, 2.5 * percentCoefficient);
        assertDeviation(20, chunkCounters.values(), expectedIdsPerChunk, 1.5 * percentCoefficient);
        assertDeviation(50, chunkCounters.values(), expectedIdsPerChunk, 0.5);
        assertDeviation(80, chunkCounters.values(), expectedIdsPerChunk, 1.5 * percentCoefficient);
        assertDeviation(95, chunkCounters.values(), expectedIdsPerChunk, 2.5 * percentCoefficient);
        assertDeviation(100, chunkCounters.values(), expectedIdsPerChunk, 5 * percentCoefficient);
    }

    @Test
    public void testMarketReportLiteSuiteShift() {
        var shardingSettings = new ShardingSettings(16, 32, 64, 32, 32, 32, 32, 32, -1, -1);
        var distributor = new ChunkDistributor(mock(CiStorageDb.class), shardingSettings, 1);

        var hid = UnsignedLongs.parseUnsignedLong("16348590749792263595");
        var checkIds = sampleChecks();
        var iterationIds = sampleIterationIds(checkIds);

        var chunkShifts = iterationIds.stream()
                .map(CheckIterationEntity.Id::generateChunkShift)
                .collect(Collectors.toSet());

        var chunks = chunkShifts.stream()
                .collect(
                        Collectors.groupingBy(
                                shift -> distributor.getId(hid, ChunkType.CT_MEDIUM_TEST, shift, shardingSettings),
                                Collectors.mapping(x -> x, Collectors.counting())
                        )
                );

        assertThat(chunks.size()).isGreaterThan(checkIds.size()); // 50% distribution
        assertThat(chunks.values().stream().mapToLong(x -> x).max().orElse(0)).isLessThan(5);
    }

    private List<CheckIterationEntity.Id> sampleIterationIds(List<CheckEntity.Id> checkIds) {
        return checkIds.stream().flatMap(
                id ->
                        Stream.of(
                                CheckIterationEntity.Id.of(id, CheckIteration.IterationType.FAST, 1),
                                CheckIterationEntity.Id.of(id, CheckIteration.IterationType.FULL, 1)
                        )
        ).toList();
    }

    private List<CheckEntity.Id> sampleChecks() {
        return Stream.of(
                58900000001281L, 59000000001281L, 59700000001281L,
                60000000001281L, 62800000001281L, 64500000001281L,
                68900000001281L, 71100000001281L, 74200000001281L,
                79300000001281L, 81200000001281L, 88600000001281L,
                94100000001281L, 90500000001281L, 99400000001281L,
                53700000001281L, 55900000001281L, 57000000001281L,
                91600000001281L, 94700000001281L, 97100000001281L,
                45400000001281L, 49700000001281L, 54200000001281L
        ).map(CheckEntity.Id::new).toList();
    }

    @Test
    public void diffsDistributedAmongYdbChunksOnDifferentStorageChunk() {
        var checkId = new CheckEntity.Id(58900000001281L);
        var id = new CheckIterationEntity.Id(checkId, 1, 1);

        var chunkTypes = Stream.of(ChunkType.values()).filter(x -> x != ChunkType.UNRECOGNIZED).toList();
        var hashes =
                chunkTypes.stream()
                        .flatMap(chunkType -> IntStream.range(0, 40)
                                .mapToObj(n -> new ChunkAggregateEntity.Id(id, ChunkEntity.Id.of(chunkType, n)))
                                .map(ChunkAggregateEntity.Id::externalHashCode)
                                .map(hash -> hash / 10000)
                        )
                        .collect(Collectors.toSet());

        assertThat(hashes).hasSize(40 * chunkTypes.size());
    }

    @Test
    public void diffsDistributedAmongYdbChunksOnDifferentCheck() {
        var checkIds = sampleChecks();
        var iterationIds = sampleIterationIds(checkIds);

        var chunk = ChunkEntity.Id.of(ChunkType.CT_MEDIUM_TEST, 42);
        var hashes = iterationIds.stream()
                .map(id -> new ChunkAggregateEntity.Id(id, chunk))
                .map(ChunkAggregateEntity.Id::externalHashCode)
                .map(hash -> hash / 10000)
                .collect(Collectors.toSet());

        assertThat(hashes).hasSize(iterationIds.size());
    }

    @Test
    public void diffsDistributedAmongYdbChunksOnMillionChecks() {
        var checkIds = CheckIdGenerator.generate(CheckEntity.ID_START, 1000000).stream()
                .map(CheckEntity.Id::new)
                .toList();

        var chunkTypes = Stream.of(ChunkType.values()).filter(x -> x != ChunkType.UNRECOGNIZED).toList();
        var aggregates = chunkTypes.stream()
                .flatMap(type -> checkIds.stream()
                        .map(id -> new ChunkAggregateEntity.Id(
                                new CheckIterationEntity.Id(id, 1, 1), ChunkEntity.Id.of(type, 42))
                        )
                ).toList();

        var hashes = aggregates.stream()
                .map(ChunkAggregateEntity.Id::externalHashCode)
                .collect(Collectors.toSet());

        assertThat(hashes.size()).isGreaterThan((int) Math.round(aggregates.size() * 0.98));
    }

    @Test
    public void testDiffEntityByHashHasStaticHash() {
        var iterationIdOne = new CheckIterationEntity.Id(new CheckEntity.Id(1L), 1, 1);
        var iterationIdTwo = new CheckIterationEntity.Id(new CheckEntity.Id(2L), 2, 2);
        var testId = new TestEntity.Id(1, "toolchain", 1);

        var idOne = TestDiffByHashEntity.Id.of(
                new ChunkAggregateEntity.Id(iterationIdOne, ChunkEntity.Id.of(ChunkType.CT_MEDIUM_TEST, 1)), testId
        );

        var idTwo = TestDiffByHashEntity.Id.of(
                new ChunkAggregateEntity.Id(iterationIdTwo, ChunkEntity.Id.of(ChunkType.CT_BUILD, 2)), testId
        );

        assertThat(idOne.getAggregateIdHash()).isEqualTo(1781833773);
        assertThat(idTwo.getAggregateIdHash()).isEqualTo(-185893360);
    }

    private void assertDeviation(int percentile, Collection<Integer> values, int bestCaseValue, double allowedPercent) {
        double percentileValue = Quantiles.percentiles().index(percentile).compute(values);
        double deviationPercent = Math.abs(bestCaseValue - percentileValue) * 100 / bestCaseValue;
        log.info(
                "{} percentile is {}, best case {}, deviation percent {}",
                percentile,
                percentileValue,
                bestCaseValue,
                deviationPercent
        );

        Assertions.assertThat(deviationPercent).isLessThanOrEqualTo(allowedPercent);
    }
}
