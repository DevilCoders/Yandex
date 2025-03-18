package ru.yandex.ci.storage.core.yt.impl;

import java.io.IOException;
import java.time.Duration;
import java.time.ZoneOffset;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.ExecutionException;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.readtable.ReadTableParams;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.yt.YtExportEntity;
import ru.yandex.ci.util.ObjectStore;
import ru.yandex.inside.yt.kosher.cypress.YPath;
import ru.yandex.inside.yt.kosher.impl.ytree.object.serializers.YTreeObjectSerializerFactory;
import ru.yandex.yt.ytclient.object.MappedRowSerializer;
import ru.yandex.yt.ytclient.proxy.TableWriter;
import ru.yandex.yt.ytclient.proxy.TransactionalClient;
import ru.yandex.yt.ytclient.proxy.request.CreateNode;
import ru.yandex.yt.ytclient.proxy.request.ExistsNode;
import ru.yandex.yt.ytclient.proxy.request.ObjectType;
import ru.yandex.yt.ytclient.proxy.request.WriteTable;

@Slf4j
@AllArgsConstructor
public abstract class YtExporter<T, KEY> {
    protected static final DateTimeFormatter FORMATTER = DateTimeFormatter
            .ofPattern("yyyy-MM-dd")
            .withZone(ZoneOffset.UTC);

    protected final CiStorageDb db;
    protected final String rootPath;
    protected final int batchSize;
    protected final Class<T> clazz;

    protected void saveExportState(YtExportEntity.Id exportKey, String lastKey) {
        log.info("Saving progress, last key: {}", lastKey);
        this.db.tx(() -> this.db.ytExport().save(new YtExportEntity(exportKey, lastKey)));
    }

    protected void send(TableWriter<T> writer, List<T> batch) throws IOException {
        while (true) {
            log.info("Waiting for writer ready event");
            writer.readyEvent().join();
            log.info("Writing data to YT");
            if (writer.write(batch)) {
                break;
            } else {
                log.info("Results not accepted");
            }
        }
    }

    protected YPath createTable(
            String tableName, Class<?> clazz, CheckIterationEntity iteration, TransactionalClient client
    ) throws ExecutionException, InterruptedException {
        var created = FORMATTER.format(Objects.requireNonNullElse(iteration.getFinish(), iteration.getCreated()));
        var path = rootPath + tableName + "/" + created;

        var schema = MappedRowSerializer.asTableSchema(YTreeObjectSerializerFactory.forClass(clazz).getFieldMap());
        var table = YPath.simple(path).append(true);

        log.info("Table path: {}", table.justPath());

        if (!client.existsNode(new ExistsNode(table)).get()) {
            client.createNode(
                    new CreateNode(table, ObjectType.Table)
                            .addAttribute("schema", schema.toYTree())
                            .addAttribute("optimize_for", "scan")
            ).get();
        }

        return table;
    }

    protected void exportData(
            TransactionalClient client, YPath table,
            YtExportEntity.Id exportKey,
            List<T> data,
            ObjectStore<Integer> numberOfResults
    ) {
        if (data.isEmpty()) {
            return;
        }

        writeData(client, table, data);
        saveExportState(data, exportKey);
        numberOfResults.set(numberOfResults.get() + data.size());
        log.info("{} results written, total: {}", data.size(), numberOfResults.get());
    }

    private void writeData(TransactionalClient client, YPath table, List<T> data) {
        log.info("Initializing YT writer, batch: {}", data.size());
        var writer = writeTable(client, table, clazz);
        try {
            send(writer, data);
        } catch (IOException e) {
            throw new RuntimeException(e);
        } finally {
            writer.close().join();
        }
    }

    protected abstract void saveExportState(List<T> batch, YtExportEntity.Id exportKey);

    protected TableWriter<T> writeTable(TransactionalClient client, YPath table, Class<T> clazz) {
        return client.writeTable(
                new WriteTable<>(table, YTreeObjectSerializerFactory.forClass(clazz))
                        .setChunkSize(32 * 1000 * 1000)
                        .setNeedRetries(true)
        ).join();
    }

    protected ReadTableParams<KEY> getReadTableParams(KEY startKey, KEY endKey, int rowLimit) {
        log.info("Read table, start key: {}, end key: {}", startKey, endKey);

        return ReadTableParams.<KEY>builder()
                .toInclusive(true)
                .fromInclusive(false)
                .timeout(Duration.ofMinutes(2))
                .fromKey(startKey)
                .toKey(endKey)
                .rowLimit(rowLimit)
                .ordered()
                .build();
    }
}
