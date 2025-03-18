package ru.yandex.ci.storage.reader.check;

import java.util.Set;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.reader.cache.ReaderCache;

public interface CheckEventsListener {

    default void onCheckCompleted(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckEntity.Id checkId,
            Set<CheckIterationEntity.Id> checkIterations
    ) {
    }

    default void onCheckFatalError(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckEntity.Id checkId,
            Set<CheckIterationEntity.Id> iterationIds,
            CheckIterationEntity.Id brokenIterationId,
            Set<CheckIterationEntity.Id> runningIterationsIds
    ) {
    }

    default void onCheckCancelled(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache, CheckEntity.Id checkId, Set<CheckIterationEntity.Id> iterationIds
    ) {
    }

    default void onChunkTypeFinalized(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId, Common.ChunkType chunkType
    ) {
    }

    default void onBeforeIterationCompleted(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId
    ) {
    }

    default void onIterationCompleted(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId
    ) {
    }

    default void onMetaIterationCompleted(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId
    ) {
    }

    default void onIterationTypeCompleted(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId // metaiteration in case of restart
    ) {
    }

    default void onTaskCompleted(
            ReaderCheckService checkService, ReaderCache.Modifiable cache, CheckTaskEntity.Id taskId
    ) {
    }
}
