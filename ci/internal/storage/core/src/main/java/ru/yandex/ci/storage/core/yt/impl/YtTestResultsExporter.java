package ru.yandex.ci.storage.core.yt.impl;

import java.util.List;
import java.util.concurrent.ExecutionException;

import com.google.common.base.Preconditions;
import com.google.common.primitives.UnsignedLong;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.yt.YtExportEntity;
import ru.yandex.ci.storage.core.db.model.yt.YtExportEntityType;
import ru.yandex.ci.storage.core.yt.model.YtTestResult;
import ru.yandex.ci.util.CiJson;
import ru.yandex.ci.util.ObjectStore;
import ru.yandex.inside.yt.kosher.cypress.YPath;
import ru.yandex.yt.ytclient.proxy.TransactionalClient;

@Slf4j
public class YtTestResultsExporter extends YtExporter<YtTestResult, TestResultEntity.Id> {
    public YtTestResultsExporter(CiStorageDb db, String rootPath, int batchSize) {
        super(db, rootPath, batchSize, YtTestResult.class);
    }

    public void export(CheckIterationEntity.Id iterationId, TransactionalClient client) throws ExecutionException,
            InterruptedException {
        var iteration = db.currentOrReadOnly(() -> db.checkIterations().get(iterationId));
        var table = this.createTable("test_result", YtTestResult.class, iteration, client);

        exportResults(client, iteration.getId(), table);
    }

    private void exportResults(TransactionalClient client, CheckIterationEntity.Id id, YPath table) {
        var exportKey = YtExportEntity.Id.of(id, YtExportEntityType.TEST_RESULT);
        var startEntity = db.currentOrReadOnly(() -> db.ytExport().find(exportKey));

        var startKey = startEntity.isEmpty()
                ?
                new TestResultEntity.Id(
                        id.getCheckId(),
                        id.getIterationType().getNumber(),
                        UnsignedLong.ZERO.longValue(),
                        // id tail
                        0L, "", 0, "", 0, 0

                )
                : CiJson.readValue(startEntity.get().getLastKeyJson(), TestResultEntity.Id.class);

        var endKey = new TestResultEntity.Id(
                id.getCheckId(),
                id.getIterationType().getNumber(),
                UnsignedLong.MAX_VALUE.longValue(),
                // id tail
                0L, "", 0, "", 0, 0
        );

        var counter = new ObjectStore<>(0);
        while (true) {
            var readParams = getReadTableParams(startKey, endKey, batchSize);
            var results = db.currentOrReadOnly(() -> db.testResults().readTable(readParams).toList());
            if (results.isEmpty()) {
                log.info("Result export completed, exported: {}", counter.get());
                break;
            }

            log.info("{} results fetched", results.size());
            startKey = results.get(results.size() - 1).getId();

            var toExport = results.stream()
                    .filter(x -> x.getId().getIterationNumber() == id.getNumber())
                    .map(YtTestResult::new)
                    .toList();

            exportData(client, table, exportKey, toExport, counter);
        }
    }

    @Override
    protected void saveExportState(List<YtTestResult> batch, YtExportEntity.Id exportKey) {
        saveExportState(exportKey, getLastKeyJson(batch));
    }

    private String getLastKeyJson(List<YtTestResult> batch) {
        Preconditions.checkState(!batch.isEmpty(), "Batch can't be empty");
        return CiJson.writeValueAsString(batch.get(batch.size() - 1).getId().toEntityId());
    }
}
