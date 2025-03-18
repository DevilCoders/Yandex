package ru.yandex.ci.storage.shard.task;

import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import com.google.protobuf.ByteString;
import lombok.Value;
import org.apache.commons.lang3.tuple.Pair;

import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.ResultOwners;
import ru.yandex.ci.storage.core.db.model.task_result.TestOutput;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.util.HostnameUtils;

public class TestResultConverter {

    private TestResultConverter() {

    }

    public static List<TestResult> convert(
            Context context,
            TaskMessages.AutocheckTestResults message
    ) {
        return message.getResultsList().stream().map(x -> convert(context, x)).collect(Collectors.toList());
    }

    private static TestResult convert(Context context, TaskMessages.AutocheckTestResult result) {
        var id = new TestResultEntity.Id(
                context.aggregateId.getIterationId(),
                TestEntity.Id.of(result.getId()),
                context.taskId,
                context.partition,
                0
        );

        return TestResult.builder()
                .id(id)
                .status(result.getTestStatus())
                .resultType(result.getResultType())
                .chunkId(context.aggregateId.getChunkId())
                .autocheckChunkId(result.getChunkHid() == 0 ? null : result.getChunkHid())
                .oldSuiteId(result.getOldSuiteId())
                .oldTestId(result.getOldId())
                .links(convertLinks(result.getLinks()))
                .metrics(result.getMetricsMap())
                .testOutputs(convert(result.getTestOutputsMap().entrySet()))
                .name(result.getName())
                .requirements(result.getRequirements())
                .branch(context.branch)
                .path(result.getPath())
                .snippet(convertBytesToString(result.getSnippet()))
                .subtestName(result.getSubtestName())
                .processedBy(HostnameUtils.getShortHostname())
                .tags(new HashSet<>(result.getTagsList()))
                .uid(result.getUid())
                .revision(context.revision)
                .revisionNumber(context.revisionNumber)
                .strongModeAYaml("")
                .isRight(context.isRight)
                .isLaunchable(result.getLaunchable())
                .created(Instant.now())
                .owners(new ResultOwners(result.getOwners().getLoginsList(), result.getOwners().getGroupsList()))
                .build();
    }

    private static String convertBytesToString(ByteString byteString) {
        return new String(byteString.toByteArray(), StandardCharsets.UTF_8);
    }

    private static Map<String, TestOutput> convert(Set<Map.Entry<String, TaskMessages.TestOutput>> outputs) {
        return outputs.stream()
                .map(
                        x -> Pair.of(
                                x.getKey(), new TestOutput(
                                        x.getValue().getHash(), x.getValue().getUrl(), x.getValue().getSize()
                                )
                        )
                )
                .collect(Collectors.toMap(Pair::getKey, Pair::getValue));
    }

    private static Map<String, List<String>> convertLinks(TaskMessages.Links links) {
        var result = new HashMap<String, List<String>>(links.getLinkCount());
        for (var entry : links.getLinkMap().entrySet()) {
            result.put(entry.getKey(), new ArrayList<>(entry.getValue().getLinkList()));
        }
        return result;
    }

    @Value
    public static class Context {
        String taskId;
        String branch;
        String revision;
        long revisionNumber;
        int partition;
        boolean isRight;
        ChunkAggregateEntity.Id aggregateId;
    }
}
