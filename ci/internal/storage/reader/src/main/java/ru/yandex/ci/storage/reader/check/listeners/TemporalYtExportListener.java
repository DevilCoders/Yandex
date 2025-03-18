package ru.yandex.ci.storage.reader.check.listeners;

import lombok.Value;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.yt.YtExportWorkflow;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckEventsListener;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;

@Value
public class TemporalYtExportListener implements CheckEventsListener {
    TemporalService temporalService;

    @Override
    public void onIterationCompleted(
            ReaderCheckService checkService, ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId
    ) {
        temporalService.startDeduplicated(
                YtExportWorkflow.class, wf -> wf::exportIteration, iterationId, YtExportWorkflow.QUEUE
        );
    }
}
