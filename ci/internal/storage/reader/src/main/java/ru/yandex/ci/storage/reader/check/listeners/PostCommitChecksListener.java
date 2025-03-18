package ru.yandex.ci.storage.reader.check.listeners;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.check.tasks.ProcessPostCommitTask;
import ru.yandex.ci.storage.core.clickhouse.ExportToClickhouse;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckEventsListener;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@AllArgsConstructor
public class PostCommitChecksListener implements CheckEventsListener {
    private final BazingaTaskManager bazingaTaskManager;

    @Override
    public void onIterationCompleted(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId
    ) {
        if (cache.iterations().get(iterationId.toMetaId()).isPresent()) {
            return;
        }

        process(cache, iterationId);
    }

    @Override
    public void onMetaIterationCompleted(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId
    ) {
        process(cache, iterationId);
    }

    private void process(ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId) {
        var check = cache.checks().getOrThrow(iterationId.getCheckId());

        if (check.getType().equals(CheckOuterClass.CheckType.BRANCH_POST_COMMIT) ||
                check.getType().equals(CheckOuterClass.CheckType.TRUNK_POST_COMMIT)) {

            var iteration = cache.iterations().getOrThrow(iterationId);

            if (CheckStatusUtils.isCancelled(iteration.getStatus())) {
                log.info("Skipping post commit notifications for {}, status {}", iterationId, iteration.getStatus());
                return;
            }

            var bazingaJobId = bazingaTaskManager.schedule(new ProcessPostCommitTask(iterationId));
            log.info(
                    "Process post commit job scheduled: iteration id: {}, task id {}",
                    iterationId, bazingaJobId
            );

            if (check.getType().equals(CheckOuterClass.CheckType.TRUNK_POST_COMMIT)) {
                bazingaJobId = bazingaTaskManager.schedule(new ExportToClickhouse(iterationId));
                log.info(
                        "Export to clickhouse job scheduled: iteration id: {}, task id {}",
                        iterationId, bazingaJobId
                );
            }
        }
    }
}
