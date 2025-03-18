package ru.yandex.ci.storage.shard.message;


import java.util.List;

import lombok.Value;
import lombok.With;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;

@Value
public class ChunkMessageWithResults {
    CheckEntity check;
    CheckTaskEntity task;
    ArcBranch branch;
    long revisionNumber;

    ChunkAggregateEntity.Id aggregateId;
    LbCommitCountdown lbCommitCountdown;
    @With
    List<TestResult> results;
}
