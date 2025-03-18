package ru.yandex.ci.storage.shard.message;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Value;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;

@Value
public class AggregateMessage {
    ChunkAggregateEntity.Id aggregateId;

    LbCommitCountdown commitCountdown;

    @Nullable
    ChunkMessageWithResults result;

    @Nullable
    Finish finish;

    public AggregateMessage(
            ChunkAggregateEntity.Id aggregateId,
            LbCommitCountdown commitCountdown,
            @Nullable ChunkMessageWithResults result,
            @Nullable Finish finish
    ) {
        Preconditions.checkState(
                (result == null && finish != null) || (result != null && finish == null),
                "Must be one of"
        );
        this.aggregateId = aggregateId;
        this.commitCountdown = commitCountdown;
        this.result = result;
        this.finish = finish;
    }

    @Value
    static class Finish {
        Common.ChunkAggregateState state;
    }
}
