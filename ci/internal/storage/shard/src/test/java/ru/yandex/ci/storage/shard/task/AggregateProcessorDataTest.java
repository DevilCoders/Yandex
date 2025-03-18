package ru.yandex.ci.storage.shard.task;

import java.util.List;
import java.util.stream.Collectors;

import com.google.gson.Gson;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.StorageTestUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.export.ExportedTaskResult;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.StorageShardTestBase;
import ru.yandex.ci.util.ResourceUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

@Slf4j
public class AggregateProcessorDataTest extends StorageShardTestBase {
    private static final Gson GSON = new Gson();

    private List<TestResult> loadData(String path, ChunkAggregateEntity.Id aggregateId) {
        return parseData(aggregateId, ResourceUtils.textResource(path));
    }

    private List<TestResult> parseData(ChunkAggregateEntity.Id aggregateId, String json) {
        return List.of(GSON.fromJson(json, ExportedTaskResult[].class)).stream()
                .map(x -> x.toEntity(aggregateId))
                .collect(Collectors.toList());
    }

    @Test
    public void first() {
        var aggregate = StorageTestUtils.createAggregate().toMutable();

        var data = loadData("data/task_results_1.json", aggregate.getId());

        var statistics = mock(ShardStatistics.class);
        var aggregateProcessor = new AggregateProcessor(new TaskDiffProcessor(statistics));

        shardCache.modify(cache -> data.forEach(result -> aggregateProcessor.process(cache, aggregate, result)));

        var result = aggregate.toImmutable();

        var expectedMain = MainStatistics.builder()
                .total(
                        StageStatistics.builder()
                                .total(22)
                                .passed(22)
                                .build()
                )
                .style(
                        StageStatistics.builder()
                                .total(13)
                                .passed(13)
                                .build()
                )
                .mediumTests(
                        StageStatistics.builder()
                                .total(9)
                                .passed(9)
                                .build()
                )
                .build();

        assertThat(result.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
    }

    @Test
    @Disabled("Custom test for large exports that are not in repository, works only with data in user home")
    public void custom() {
        var aggregate = StorageTestUtils.createAggregate().toMutable();
        var statistics = mock(ShardStatistics.class);
        var aggregateProcessor = new AggregateProcessor(new TaskDiffProcessor(statistics));

        var path = System.getProperty("user.home") + "/storage_lots_of_builds.json";
        var data = parseData(
                aggregate.getId(),
                ResourceUtils.textResource(path)
        );

        log.info("Number of results: {}", data.size());
        var left = data.stream()
                .filter(TestResult::isLeft)
                .map(x -> x.getTestId().getSuiteId())
                .collect(Collectors.toSet());

        var right = data.stream()
                .filter(TestResult::isRight)
                .map(x -> x.getTestId().getSuiteId())
                .collect(Collectors.toSet());

        log.info(
                "Number of uniq results, left: {}, right: {}", left.size(), right.size()
        );

        var testIds = data.stream()
                .map(r -> TestStatusEntity.Id.idInTrunk(r.getId().getFullTestId()))
                .collect(Collectors.toSet());

        log.info("Number of tests: {}", testIds);
        // warm up cache for speed
        shardCache.muteStatus().get(testIds);

        var byTestId = data.stream().collect(Collectors.groupingBy(TestResult::getTestId));

        for (var group : byTestId.entrySet()) {
            shardCache.modify(
                    cache -> group.getValue().forEach(result -> aggregateProcessor.process(cache, aggregate, result))
            );
        }

        var result = aggregate.toImmutable();

        var expectedMain = MainStatistics.builder()
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .build()
                )
                .build();

        assertThat(result.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
    }
}
