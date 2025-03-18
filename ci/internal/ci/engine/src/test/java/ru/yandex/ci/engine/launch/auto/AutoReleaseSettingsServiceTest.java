package ru.yandex.ci.engine.launch.auto;

import java.time.Instant;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.Spy;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockito.junit.jupiter.MockitoSettings;
import org.mockito.quality.Strictness;
import org.mockito.stubbing.Answer;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.TestCiDbUtils;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;
import ru.yandex.ci.core.db.table.AutoReleaseSettingsHistoryTable;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.lang.NonNullApi;

import static java.util.function.Function.identity;
import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.mockito.ArgumentMatchers.anyCollection;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.when;

@NonNullApi
@ExtendWith(MockitoExtension.class)
@MockitoSettings(strictness = Strictness.LENIENT)
class AutoReleaseSettingsServiceTest {

    @Spy
    @InjectMocks
    private AutoReleaseSettingsService autoReleaseSettingsService;

    @Mock
    private CiMainDb db;
    @Mock
    private AutoReleaseSettingsHistoryTable autoReleaseSettingsHistoryTable;

    @BeforeEach
    public void resetMocks() {
        reset(db, autoReleaseSettingsHistoryTable);
        mockDatabase();
    }


    @Test
    void findLastForProcessId() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        doReturn(null).when(autoReleaseSettingsHistoryTable).findLatest(eq(processId));
        assertNull(autoReleaseSettingsService.findLastForProcessId(processId));
    }

    @Test
    void findLastForProcessIds_whenListOfIdsIsEmpty() {
        assertThat(autoReleaseSettingsService.findLastForProcessIds(List.of())).isEmpty();
    }

    @Test
    void findLastForProcessIds() {
        CiProcessId cRelease1 = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release-1");
        CiProcessId cRelease2 = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release-2");
        CiProcessId eRelease1 = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "release-1");

        doReturn(null).when(autoReleaseSettingsHistoryTable)
                .findLatest(eq(cRelease1));

        AutoReleaseSettingsHistory cRelease2Settings = AutoReleaseSettingsHistory.of(
                cRelease2, false, "login", "message", Instant.parse("2020-01-02T10:00:00.000Z")
        );
        AutoReleaseSettingsHistory eRelease1Settings = AutoReleaseSettingsHistory.of(
                eRelease1, true, "login2", "message2", Instant.parse("2020-01-02T12:00:00.000Z")
        );


        when(autoReleaseSettingsHistoryTable.findLatest(anyCollection()))
                .thenAnswer((Answer<Map<CiProcessId, AutoReleaseSettingsHistory>>) invocation -> {
                    Collection<CiProcessId> ids = invocation.getArgument(0);
                    return Stream.of(
                            cRelease2Settings,
                            eRelease1Settings
                    )
                            .filter(s -> ids.contains(s.getCiProcessId()))
                            .collect(Collectors.toMap(
                                    AutoReleaseSettingsHistory::getCiProcessId,
                                    identity()
                            ));
                });

        assertThat(
                autoReleaseSettingsService.findLastForProcessIds(List.of(
                        cRelease1, cRelease2, eRelease1
                ))
        ).contains(
                Map.entry(cRelease2, cRelease2Settings),
                Map.entry(eRelease1, eRelease1Settings)
        );
    }


    private void mockDatabase() {
        TestCiDbUtils.mockToCallRealTxMethods(db);
        doReturn(autoReleaseSettingsHistoryTable).when(db).autoReleaseSettingsHistory();
    }

}
