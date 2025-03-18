package ru.yandex.ci.storage.core.yt.impl;

import java.time.Duration;

import io.temporal.workflow.Workflow;
import org.slf4j.Logger;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.yt.YtExportActivity;
import ru.yandex.ci.storage.core.yt.YtExportWorkflow;


public class YtExportWorkflowImpl implements YtExportWorkflow {

    private static final Logger log = Workflow.getLogger(YtExportWorkflowImpl.class);

    private final YtExportActivity ytExportActivity = TemporalService.createActivity(
            YtExportActivity.class,
            Duration.ofHours(6)
    );

    @Override
    public void exportIteration(CheckIterationEntity.Id id) {
        log.info("Starting yt export workflow for iteration: {}", id);
        ytExportActivity.exportIteration(id);
        log.info("Finished yt export workflow for iteration: {}", id);

    }
}

