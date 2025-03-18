package ru.yandex.ci.storage.reader.message.main;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import lombok.Getter;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.ShardIn;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.sharding.ChunkDistributor;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;

public class TaskResultDistributor {
    private final ChunkDistributor chunkDistributor;
    private final int maxNumberOfResultsPerWrite;
    private final ReaderStatistics statistics;

    public TaskResultDistributor(
            ChunkDistributor chunkDistributor, ReaderStatistics statistics, int maxNumberOfResultsPerWrite
    ) {
        this.chunkDistributor = chunkDistributor;
        this.statistics = statistics;
        this.maxNumberOfResultsPerWrite = maxNumberOfResultsPerWrite;
    }

    public Result distribute(List<CheckTaskMessage> messages) {
        var chunkToShardMessages = new HashMap<ChunkEntity.Id, ChunkMessages>();

        var affectedChunksByTaskId = new HashMap<CheckTaskEntity.Id, Set<ChunkEntity.Id>>();
        var affectedToolchainsByTaskId = new HashMap<CheckTaskEntity.Id, Set<String>>();

        for (var message : messages) {
            var result = message.getMessage().getAutocheckTestResults();
            var chunkToResultBuilder = new HashMap<ChunkEntity.Id, List<TaskMessages.AutocheckTestResult>>();

            var taskId = CheckProtoMappers.toTaskId(message.getMessage().getFullTaskId());

            processResults(
                    message.getTaskType(),
                    chunkToResultBuilder,
                    result,
                    affectedChunksByTaskId.computeIfAbsent(
                            taskId, key -> new HashSet<>()
                    ),
                    affectedToolchainsByTaskId.computeIfAbsent(
                            taskId, key -> new HashSet<>()
                    ),
                    message.getChunkShift(),
                    message.getShardingSettings()
            );

            for (var entry : chunkToResultBuilder.entrySet()) {
                var shardMessages = chunkToShardMessages.computeIfAbsent(entry.getKey(), ChunkMessages::new);

                var shardMessage = shardMessages.getLastMessage();

                var resultsInEntry = entry.getValue().size();
                if (shardMessage.numberOfResults > 0 &&
                        shardMessage.numberOfResults + resultsInEntry > this.maxNumberOfResultsPerWrite) {
                    shardMessage = shardMessages.createMessage();
                }

                shardMessage.getMessages().add(
                        ShardIn.ShardResultMessage.newBuilder()
                                .setFullTaskId(message.getMessage().getFullTaskId())
                                .setPartition(message.getMessage().getPartition())
                                .setAutocheckTestResults(
                                        TaskMessages.AutocheckTestResults.newBuilder()
                                                .addAllResults(entry.getValue())
                                                .build()

                                )
                                .build()
                );

                shardMessage.numberOfResults += resultsInEntry;
            }
        }

        var messagesToWrite = chunkToShardMessages.values().stream()
                .flatMap(message -> message.build().stream())
                .collect(Collectors.toList());

        var numberOfResults = chunkToShardMessages.values().stream()
                .flatMap(message -> message.getMessages().stream())
                .mapToInt(x -> {
                    this.statistics.getMain().onResultsDistributed(x.numberOfResults);
                    return x.numberOfResults;
                })
                .sum();

        return new Result(
                messagesToWrite,
                affectedChunksByTaskId,
                affectedToolchainsByTaskId,
                numberOfResults,
                chunkToShardMessages.size()
        );
    }

    private void processResults(
            Common.CheckTaskType taskType,
            Map<ChunkEntity.Id, List<TaskMessages.AutocheckTestResult>> chunkToResult,
            TaskMessages.AutocheckTestResults message,
            Set<ChunkEntity.Id> affectedByTaskChunks,
            Set<String> affectedToolchains,
            long chunkShift,
            ShardingSettings shardingSettings
    ) {
        var list = message.getResultsList();
        for (var result : list) {
            // todo temporary hack for migration
            if (taskType == Common.CheckTaskType.CTT_NATIVE_BUILD) {
                result = result.toBuilder().setResultType(Common.ResultType.RT_NATIVE_BUILD).build();
            }

            var chunkType = ResultTypeUtils.toChunkType(result.getResultType());
            // todo temporary hack for migration
            if (taskType == Common.CheckTaskType.CTT_NATIVE_BUILD) {
                chunkType = Common.ChunkType.CT_NATIVE_BUILD;
            }

            var chunkId = chunkDistributor.getId(result.getId().getSuiteHid(), chunkType, chunkShift, shardingSettings);

            if (chunkId == null) {
                continue;
            }

            chunkToResult.computeIfAbsent(chunkId, key -> new ArrayList<>()).add(result);

            affectedByTaskChunks.add(chunkId);
            affectedToolchains.add(result.getId().getToolchain());
        }
    }

    @Value
    private static class ChunkMessages {
        ChunkEntity.Id chunkId;
        List<ResultsAggregateMessage> messages;

        ChunkMessages(ChunkEntity.Id chunkId) {
            this.chunkId = chunkId;
            this.messages = new ArrayList<>();
            this.messages.add(new ResultsAggregateMessage());
        }

        List<ShardIn.ChunkMessage> build() {
            var protoChunkId = CheckProtoMappers.toProtoChunkId(chunkId);

            return messages.stream().flatMap(x -> x.getMessages().stream())
                    .map(
                            m -> ShardIn.ChunkMessage.newBuilder()
                                    .setChunkId(protoChunkId)
                                    .setResult(m)
                                    .build()
                    ).collect(Collectors.toList());
        }

        public ResultsAggregateMessage getLastMessage() {
            return this.messages.get(this.messages.size() - 1);
        }

        public ResultsAggregateMessage createMessage() {
            var result = new ResultsAggregateMessage();
            this.messages.add(result);
            return result;
        }
    }

    private static class ResultsAggregateMessage {
        @Getter
        List<ShardIn.ShardResultMessage> messages = new ArrayList<>();
        int numberOfResults = 0;
    }

    @Value
    public static class Result {
        public static final Result EMPTY = new Result(List.of(), Map.of(), Map.of(), 0, 0);

        List<ShardIn.ChunkMessage> messagesToWrite;
        Map<CheckTaskEntity.Id, Set<ChunkEntity.Id>> affectedChunks;
        Map<CheckTaskEntity.Id, Set<String>> affectedToolchains;
        int numberOfResults;
        int numberOfChunks;
    }
}
