package ru.yandex.ci.storage.core.yt.impl;

import java.util.concurrent.ExecutionException;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.yt.IterationToYtExporter;
import ru.yandex.ci.storage.core.yt.YtClientFactory;

public class IterationToYtExporterImpl implements IterationToYtExporter {

    private final YtClientFactory clientFactory;

    private final YtTestResultsExporter resultsExporter;
    private final YtTestDiffsExporter diffsExporter;

    public IterationToYtExporterImpl(CiStorageDb db, String rootPath, YtClientFactory clientFactory, int batchSize) {
        this.clientFactory = clientFactory;
        this.resultsExporter = new YtTestResultsExporter(db, rootPath, batchSize);
        this.diffsExporter = new YtTestDiffsExporter(db, rootPath, batchSize);
    }

    @Override
    public void export(CheckIterationEntity.Id id) throws ExecutionException, InterruptedException {
        clientFactory.execute(client -> {
            try {
                resultsExporter.export(id, client);
                diffsExporter.export(id, client);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        });
    }
}
