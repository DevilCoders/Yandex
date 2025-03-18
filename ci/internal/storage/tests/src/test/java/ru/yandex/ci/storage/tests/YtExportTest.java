package ru.yandex.ci.storage.tests;

import java.util.concurrent.ExecutionException;

import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.yt.impl.IterationToYtExporterImpl;
import ru.yandex.ci.storage.core.yt.impl.YtClientFactoryImpl;
import ru.yandex.ci.storage.core.yt.model.YtTestDiff;
import ru.yandex.ci.storage.core.yt.model.YtTestResult;
import ru.yandex.yt.ytclient.rpc.RpcCredentials;

import static org.assertj.core.api.AssertionsForInterfaceTypes.assertThat;

public class YtExportTest extends StorageTestsYdbTestBase {
    @Value("${storage.ytClientFactory.username}")
    String username;

    @Value("${storage.ytClientFactory.token}")
    String token;

    @Value("${storage.yt.path://home/ci/storage/local/}")
    String rootPath;

    @Test
    @Disabled
    public void networkTest() throws ExecutionException, InterruptedException {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());

        storageTester.writeAndDeliver(writer -> writer.to(registration.getTasks()).results(exampleBuild(1L)).finish());

        var exporter = new IterationToYtExporterImpl(
                this.db, rootPath, new YtClientFactoryImpl(new RpcCredentials(username, token)), 1
        );

        exporter.export(registration.getIteration().getId());
    }

    @Test
    public void converts() {
        var registration = storageTester.register(registrar -> registrar.fullIteration().leftTask().rightTask());
        storageTester.writeAndDeliver(writer -> writer.to(registration.getTasks())
                .results(exampleBuild(1L), exampleTest(2L, 3L, Common.ResultType.RT_TEST_MEDIUM))
                .finish()
        );

        var results = this.db.currentOrReadOnly(() -> this.db.testResults().findAll());

        assertThat(results).isNotEmpty();

        for (var result : results) {
            assertThat(
                    new YtTestResult(result).toEntity().toBuilder()
                            .owners(result.getOwners())
                            .oldTestId(result.getOldTestId())
                            .build()
            )
                    .isEqualTo(result);
        }

        var diffs = this.db.currentOrReadOnly(() -> this.db.testDiffs().findAll());

        assertThat(diffs).isNotEmpty();

        for (var diff : diffs) {
            assertThat(
                    new YtTestDiff(diff).toEntity().toBuilder()
                            .owners(diff.getOwners())
                            .oldTestId(diff.getOldTestId())
                            .oldSuiteId(diff.getOldSuiteId())
                            .build()
            )
                    .isEqualTo(diff);
        }
    }
}
