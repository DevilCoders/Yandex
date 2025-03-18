package ru.yandex.ci.storage.core.yt.impl;

import java.util.List;
import java.util.concurrent.ExecutionException;

import com.google.common.base.Preconditions;
import com.google.common.primitives.UnsignedLong;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.db.model.yt.YtExportEntity;
import ru.yandex.ci.storage.core.db.model.yt.YtExportEntityType;
import ru.yandex.ci.storage.core.yt.model.YtTestDiff;
import ru.yandex.ci.util.CiJson;
import ru.yandex.ci.util.ObjectStore;
import ru.yandex.inside.yt.kosher.cypress.YPath;
import ru.yandex.yt.ytclient.proxy.TransactionalClient;

@Slf4j
public class YtTestDiffsExporter extends YtExporter<YtTestDiff, TestDiffEntity.Id> {
    public YtTestDiffsExporter(CiStorageDb db, String rootPath, int batchSize) {
        super(db, rootPath, batchSize, YtTestDiff.class);
    }

    public void export(CheckIterationEntity.Id iterationId, TransactionalClient client) throws ExecutionException,
            InterruptedException {
        var iteration = db.currentOrReadOnly(() -> db.checkIterations().get(iterationId));
        var table = this.createTable("test_diff", YtTestDiff.class, iteration, client);

        exportResults(client, iteration.getId(), table);
    }

    private void exportResults(TransactionalClient client, CheckIterationEntity.Id id, YPath table) {
        var exportKey = YtExportEntity.Id.of(id, YtExportEntityType.TEST_DIFF);
        var startEntity = db.currentOrReadOnly(() -> db.ytExport().find(exportKey));
        var startKey = startEntity.isEmpty()
                ?
                new TestDiffEntity.Id(
                        id.getCheckId(),
                        id.getIterationType().getNumber(),
                        Common.ResultType.RT_BUILD,
                        "",
                        // id tail
                        "",
                        UnsignedLong.ZERO.longValue(),
                        UnsignedLong.ZERO.longValue(),
                        id.getNumber()
                )
                : CiJson.readValue(startEntity.get().getLastKeyJson(), TestDiffEntity.Id.class);

        var endKey = new TestDiffEntity.Id(
                id.getCheckId(),
                id.getIterationType().getNumber(),
                Common.ResultType.RT_TEST_TESTENV,
                String.valueOf(Character.MAX_VALUE),
                // id tail
                "",
                UnsignedLong.ZERO.longValue(),
                UnsignedLong.ZERO.longValue(),
                id.getNumber()
        );

        var counter = new ObjectStore<>(0);
        while (true) {
            var readParams = getReadTableParams(startKey, endKey, batchSize);
            var diffs = db.currentOrReadOnly(() -> db.testDiffs().readTable(readParams).toList());
            if (diffs.isEmpty()) {
                log.info("Diffs export completed, exported: {}", counter.get());
                break;
            }

            log.info("{} diffs fetched", diffs.size());
            startKey = diffs.get(diffs.size() - 1).getId();

            var toExport = diffs.stream()
                    .filter(x -> x.getId().getIterationNumber() == id.getNumber())
                    .map(YtTestDiff::new)
                    .toList();

            exportData(client, table, exportKey, toExport, counter);
        }
    }


    @Override
    protected void saveExportState(List<YtTestDiff> batch, YtExportEntity.Id exportKey) {
        saveExportState(exportKey, getLastKeyJson(batch));
    }

    private String getLastKeyJson(List<YtTestDiff> batch) {
        Preconditions.checkState(!batch.isEmpty(), "Batch can't be empty");
        return CiJson.writeValueAsString(batch.get(batch.size() - 1).getId().toEntityId());
    }
}
