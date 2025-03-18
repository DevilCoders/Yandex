package ru.yandex.ci.storage.reader.message.main;

import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;

@Value
public class CheckTaskMessage {
    CheckTaskEntity.Id checkTaskId;
    Common.CheckTaskType taskType;
    TaskMessages.TaskMessage message;
    long chunkShift;
    ShardingSettings shardingSettings;
}
