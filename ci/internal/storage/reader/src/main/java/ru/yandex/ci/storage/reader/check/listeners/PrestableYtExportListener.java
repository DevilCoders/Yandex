package ru.yandex.ci.storage.reader.check.listeners;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.yt.impl.YtExportTask;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckEventsListener;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.util.OurGuys;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@AllArgsConstructor
public class PrestableYtExportListener implements CheckEventsListener {
    BazingaTaskManager bazingaTaskManager;

    @Override
    public void onIterationCompleted(
            ReaderCheckService checkService, ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId
    ) {
        var check = cache.checks().getOrThrow(iterationId.getCheckId());
        if (!OurGuys.isOurGuy(check.getAuthor())) {
            return;
        }

        var bazingaJobId = bazingaTaskManager.schedule(new YtExportTask(iterationId));
        log.info("YT Export job scheduled: iteration id: {}, task id {}", iterationId, bazingaJobId);
    }
}
