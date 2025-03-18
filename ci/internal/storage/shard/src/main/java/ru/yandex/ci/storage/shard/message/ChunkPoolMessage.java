package ru.yandex.ci.storage.shard.message;

import lombok.Value;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.ShardIn;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;

@Value
public class ChunkPoolMessage {
    Common.MessageMeta meta;
    ChunkEntity.Id chunkId;
    LbCommitCountdown commitCountdown;
    ShardIn.ChunkMessage chunkMessage;

    public void notifyMessageProcessed() {
        this.commitCountdown.notifyMessageProcessed();
    }
}
