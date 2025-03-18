package ru.yandex.ci.storage.core.export;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.Charset;
import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.util.ArrayList;
import java.util.Properties;
import java.util.stream.Collectors;

import com.google.gson.GsonBuilder;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDbImpl;
import ru.yandex.ci.storage.core.db.CiStorageRepository;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;

import static com.google.common.io.Files.asCharSink;
import static org.assertj.core.api.Assertions.assertThat;

/***
 * Exports data from production for debug and test purpose.
 */
public class ExportTest {
    @Test
    @Disabled("Works only on local machine, connects to PROD YDB")
    public void export() throws IOException {
        var prop = new Properties();

        prop.load(new FileInputStream(System.getProperty("user.home") + "/.ci/ci-local.properties"));

        var kikimrConfig = YdbUtils.createConfig(
                "ydb-ru.yandex.net:2135",
                "/ru/ci/stable/ci-storage",
                prop.getProperty("ydb.kikimrConfig.ydbToken"),
                null
        ).withSessionCreationTimeout(Duration.of(1, ChronoUnit.MINUTES));

        var repository = new CiStorageRepository(kikimrConfig);
        var db = new CiStorageDbImpl(repository);

        var checkId = CheckEntity.Id.of(78100000000071L);
        var iterationId = CheckIterationEntity.Id.of(checkId, CheckIteration.IterationType.FULL, 1);
        var results = new ArrayList<TestResultEntity>();
        db.scan().withMaxSize(Integer.MAX_VALUE).run(() -> {
            results.addAll(
                    db.testResults().find(
                            YqlPredicate
                                    .where("id.iterationId").eq(iterationId)
                                    // Ignores build results because we have a lot of them
                                    .and("chunkId.chunkType").neq(Common.ChunkType.CT_BUILD)
                    )
            );
        });

        assertThat(results.size()).isGreaterThan(0);

        var output = results.stream()
                // reduce size
                .map(ExportedTaskResult::of)
                .collect(Collectors.toList());

        var tempFile = File.createTempFile("storage-export-", ".json");

        asCharSink(
                tempFile, Charset.defaultCharset()).write(new GsonBuilder().setPrettyPrinting().create().toJson(output)
        );

        System.out.println("Exported to: " + tempFile.getAbsolutePath());
    }
}
