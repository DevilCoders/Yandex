package ru.yandex.ci.core.db.table;

import java.time.Instant;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;
import ru.yandex.ci.core.test.TestData;

import static java.util.stream.Collectors.toList;
import static org.assertj.core.api.Assertions.assertThat;

public class AutoReleaseSettingsHistoryTableTest extends CommonYdbTestBase {
    private static final long SEED = 1330867271L;

    @Test
    void findLatest() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        assertThat(db.currentOrTx(() ->
                db.autoReleaseSettingsHistory().findLatest(processId)
        )).isNull();

        var firstSettings = AutoReleaseSettingsHistory.of(
                processId, false, "login", "message", Instant.parse("2020-01-02T10:00:00.000Z")
        );
        db.currentOrTx(() -> db.autoReleaseSettingsHistory().save(firstSettings));
        assertThat(db.currentOrTx(() ->
                db.autoReleaseSettingsHistory().findLatest(processId)
        )).isEqualTo(firstSettings);

        var secondSettings = AutoReleaseSettingsHistory.of(
                processId, false, "login", "message", Instant.parse("2020-01-02T12:00:00.000Z")
        );
        db.currentOrTx(() -> db.autoReleaseSettingsHistory().save(secondSettings));
        assertThat(db.currentOrTx(() ->
                db.autoReleaseSettingsHistory().findLatest(processId)
        )).isEqualTo(secondSettings);
    }

    @Test
    void latestMultipleProcesses() {
        var itemsPerProcess = 10;
        var start = Instant.parse("2020-01-02T12:00:00.000Z");
        var processes = IntStream.range(0, 10)
                .mapToObj(i -> CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release-" + i))
                .collect(toList());

        var timestamps = new Random(SEED).ints(0, 500)
                .limit((long) processes.size() * itemsPerProcess)
                .mapToObj(i -> start.plusSeconds(i * 17L))
                .collect(Collectors.toCollection(ArrayDeque::new));

        var settingsList = new ArrayList<AutoReleaseSettingsHistory>();
        for (CiProcessId processId : processes) {
            for (int i = 0; i < itemsPerProcess; i++) {
                var settings = AutoReleaseSettingsHistory.of(
                        processId, false, "login", "message", timestamps.pop()
                );
                settingsList.add(settings);
                db.currentOrTx(() -> db.autoReleaseSettingsHistory().save(settings));
            }
        }

        Map<CiProcessId, AutoReleaseSettingsHistory> latest =
                db.currentOrReadOnly(() -> db.autoReleaseSettingsHistory().findLatest(processes));

        Comparator<AutoReleaseSettingsHistory> compareTs = Comparator.comparing(e -> e.getId().getTimestamp());
        Map<CiProcessId, AutoReleaseSettingsHistory> expected = settingsList.stream()
                .collect(Collectors.toMap(
                        s -> CiProcessId.ofString(s.getId().getProcessId()),
                        Function.identity(),
                        (a, b) -> Collections.max(List.of(a, b), compareTs)
                ));

        assertThat(latest)
                .hasSize(10)
                .isEqualTo(expected);

    }
}
