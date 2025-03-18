package ru.yandex.ci.storage.core.yt.impl;

import java.util.concurrent.ExecutionException;

import io.temporal.activity.Activity;
import io.temporal.workflow.Workflow;
import org.slf4j.Logger;

import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.yt.IterationToYtExporter;
import ru.yandex.ci.storage.core.yt.YtExportActivity;

public class YtExportActivityImpl implements YtExportActivity {

    private static final Logger log = Workflow.getLogger(YtExportActivityImpl.class);

    private final IterationToYtExporter exporter;

    public YtExportActivityImpl(IterationToYtExporter exporter) {
        this.exporter = exporter;
    }

    @Override
    public void exportIteration(CheckIterationEntity.Id id) {
        try {
            log.info("Starting yt export activity for iteration: {}", id);
            exporter.export(id);
            log.info("Finished yt export activity for iteration: {}", id);
        } catch (ExecutionException | InterruptedException e) {
            throw Activity.wrap(e);
        }
    }
}
