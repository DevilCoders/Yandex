package ru.yandex.ci.storage_tools;

import java.io.IOException;
import java.time.Duration;

import com.google.common.primitives.UnsignedLong;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import yandex.cloud.repository.db.readtable.ReadTableParams;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.spring.StorageYdbConfig;
import ru.yandex.ci.storage.core.yt.YtClientFactory;
import ru.yandex.ci.storage.core.yt.model.YtTestResult;
import ru.yandex.ci.storage.tms.spring.clients.YtClientConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.inside.yt.kosher.cypress.YPath;
import ru.yandex.inside.yt.kosher.impl.ytree.object.serializers.YTreeObjectSerializerFactory;
import ru.yandex.yt.ytclient.proxy.request.WriteTable;

@Slf4j
@Configuration
@Import({
        StorageYdbConfig.class,
        YtClientConfig.class
})
public class ReadTestResultEntity extends AbstractSpringBasedApp {

    @Autowired
    CiStorageDb db;

    @Autowired
    YtClientFactory ytClientFactory;

    @Override
    protected void run() {

        var checkId = CheckEntity.Id.of(83500000002230L);
        var iterationType = 1;
        var iterationNumber = 1;

        var startKey = new TestResultEntity.Id(
                checkId,
                iterationType,
                UnsignedLong.ZERO.longValue(),
                // id tail
                0L, "", 0, "", 0, 0
        );
        var endKey = new TestResultEntity.Id(
                checkId,
                iterationType,
                UnsignedLong.MAX_VALUE.longValue(),
                // id tail
                0L, "", 0, "", 0, 0
        );

        var readParams = ReadTableParams.<TestResultEntity.Id>builder()
                .toInclusive(true)
                .fromInclusive(false)
                .timeout(Duration.ofMinutes(2))
                .fromKey(startKey)
                .toKey(endKey)
                .rowLimit(262144)
                .ordered()
                .build();

        var results = db.currentOrReadOnly(() -> db.testResults().readTable(readParams).toList());

        var toExport = results.stream()
                .filter(x -> x.getId().getIterationNumber() == iterationNumber)
                .map(YtTestResult::new)
                .toList();

        log.info("toExport length: {}", toExport.size());

        var table = YPath.simple("//home/ci/storage/stable/test_result/2022-06-14").append(true);
        ytClientFactory.execute(client -> {
            var writer = client.writeTable(
                    new WriteTable<>(table, YTreeObjectSerializerFactory.forClass(YtTestResult.class))
                            .setChunkSize(32 * 1000 * 1000)
                            .setNeedRetries(true)
            ).join();
            writer.readyEvent().join();
            try {
                writer.write(toExport);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        });
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, "ci-storage", "ci-storage-tms", Environment.STABLE);
    }
}
