package ru.yandex.ci.storage.core.clickhouse;

import java.sql.SQLException;
import java.util.List;
import java.util.Set;
import java.util.function.Consumer;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.clickhouse.change_run.ChangeRunEntity;
import ru.yandex.ci.storage.core.db.clickhouse.change_run.ChangeRunTable;
import ru.yandex.ci.storage.core.db.clickhouse.last_run.LastRunEntity;
import ru.yandex.ci.storage.core.db.clickhouse.last_run.LastRunTable;
import ru.yandex.ci.storage.core.db.clickhouse.old_test.OldTestEntity;
import ru.yandex.ci.storage.core.db.clickhouse.old_test.OldTestTable;
import ru.yandex.ci.storage.core.db.clickhouse.run_link.RunLinkEntity;
import ru.yandex.ci.storage.core.db.clickhouse.run_link.RunLinkTable;
import ru.yandex.ci.storage.core.db.clickhouse.runs.TestRunEntity;
import ru.yandex.ci.storage.core.db.clickhouse.runs.TestRunTable;
import ru.yandex.ci.storage.core.db.clickhouse.test_event.TestEventEntity;
import ru.yandex.ci.storage.core.db.clickhouse.test_event.TestEventTable;
import ru.yandex.ci.util.Retryable;

@Slf4j
@AllArgsConstructor
public class AutocheckClickhouse {
    private final OldTestTable oldTestTable;
    private final ChangeRunTable changeRunTable;
    private final LastRunTable lastRunTable;
    private final TestRunTable testRunTable;
    private final TestEventTable testEventTable;
    private final RunLinkTable runLinkTable;

    public List<OldTestEntity> getTests(Set<String> ids) throws SQLException, InterruptedException {
        if (ids.isEmpty()) {
            return List.of();
        }

        return oldTestTable.getTests(ids);
    }

    public void insert(
            List<OldTestEntity> newTests,
            List<TestRunEntity> runs,
            List<ChangeRunEntity> changeRuns,
            List<LastRunEntity> lastRuns,
            List<RunLinkEntity> runLinks
    ) throws InterruptedException {
        var maxRetries = 10;

        Consumer<Throwable> throwableConsumer = t -> {
        };

        log.info("Inserting {} tests", newTests.size());
        Retryable.retry(() -> oldTestTable.save(newTests), throwableConsumer, false, 1, 1, maxRetries);

        log.info("Inserting {} runs", runs.size());
        Retryable.retry(() -> testRunTable.save(runs), throwableConsumer, false, 1, 1, maxRetries);

        log.info("Inserting {} change runs", changeRuns.size());
        Retryable.retry(() -> changeRunTable.save(changeRuns), throwableConsumer, false, 1, 1, maxRetries);

        log.info("Inserting {} last runs", lastRuns.size());
        Retryable.retry(() -> lastRunTable.save(lastRuns), throwableConsumer, false, 1, 1, maxRetries);

        log.info("Inserting {} run links", runLinks.size());
        Retryable.retry(() -> runLinkTable.save(runLinks), throwableConsumer, false, 1, 1, maxRetries);
    }

    public void insert(List<TestEventEntity> events) throws InterruptedException {
        log.info("Inserting {} test events", events.size());
        testEventTable.save(events);
    }
}
