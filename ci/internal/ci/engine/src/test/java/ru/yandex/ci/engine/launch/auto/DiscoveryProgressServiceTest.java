package ru.yandex.ci.engine.launch.auto;

import java.util.Optional;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockito.junit.jupiter.MockitoSettings;
import org.mockito.quality.Strictness;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.core.db.TestCiDbUtils;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgressTable;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.flow.db.CiDb;

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;

@ExtendWith(MockitoExtension.class)
@MockitoSettings(strictness = Strictness.LENIENT)
class DiscoveryProgressServiceTest extends CommonTestBase {

    @Mock
    private CiDb db;

    @Mock
    private CommitDiscoveryProgressTable commitDiscoveryProgressTable;

    private DiscoveryProgressService discoveryProgressService;

    @BeforeEach
    public void resetMocks() {
        mockDatabase();
        discoveryProgressService = new DiscoveryProgressService(db);
    }

    @Test
    void markAsDirDiscovered() {
        var progress = CommitDiscoveryProgress.builder()
                .arcRevision(TestData.TRUNK_R1)
                .dirDiscoveryFinishedForParents(true)
                .build();
        doReturn(Optional.of(progress)).when(commitDiscoveryProgressTable).find(eq("r1"));

        discoveryProgressService.markAsDiscovered(TestData.TRUNK_R1, DiscoveryType.DIR);
        verify(commitDiscoveryProgressTable).save(eq(
                progress.withDirDiscoveryFinished(true)
        ));
    }

    @Test
    void markAsDirDiscovered_whenProgressNotFound() {
        discoveryProgressService.markAsDiscovered(TestData.TRUNK_R1, DiscoveryType.DIR);
        verify(commitDiscoveryProgressTable).save(eq(
                CommitDiscoveryProgress.builder()
                        .arcRevision(TestData.TRUNK_R1)
                        .dirDiscoveryFinished(true)
                        .build()
        ));
    }

    @Test
    void markAsGraphDiscovered() {
        var progress = CommitDiscoveryProgress.builder()
                .arcRevision(TestData.TRUNK_R1)
                .graphDiscoveryFinishedForParents(true)
                .build();
        doReturn(Optional.of(progress)).when(commitDiscoveryProgressTable).find(eq("r1"));

        discoveryProgressService.markAsDiscovered(TestData.TRUNK_R1, DiscoveryType.GRAPH);
        verify(commitDiscoveryProgressTable).save(eq(
                progress.withGraphDiscoveryFinished(true)
        ));
    }

    @Test
    void markAsGraphDiscovered_whenProgressNotFound() {
        discoveryProgressService.markAsDiscovered(TestData.TRUNK_R1, DiscoveryType.GRAPH);
        verify(commitDiscoveryProgressTable).save(eq(
                CommitDiscoveryProgress.builder()
                        .arcRevision(TestData.TRUNK_R1)
                        .graphDiscoveryFinished(true)
                        .build()
        ));
    }

    @Test
    void markAsStorageDiscovered() {
        var progress = CommitDiscoveryProgress.builder()
                .arcRevision(TestData.TRUNK_R1)
                .storageDiscoveryFinishedForParents(true)
                .build();
        doReturn(Optional.of(progress)).when(commitDiscoveryProgressTable).find(eq("r1"));

        discoveryProgressService.markAsDiscovered(TestData.TRUNK_R1, DiscoveryType.STORAGE);
        verify(commitDiscoveryProgressTable).save(eq(
                progress.withStorageDiscoveryFinished(true)
        ));
    }

    @Test
    void markAsStorageDiscovered_whenProgressNotFound() {
        discoveryProgressService.markAsDiscovered(TestData.TRUNK_R1, DiscoveryType.STORAGE);
        verify(commitDiscoveryProgressTable).save(eq(
                CommitDiscoveryProgress.builder()
                        .arcRevision(TestData.TRUNK_R1)
                        .storageDiscoveryFinished(true)
                        .build()
        ));
    }

    @Test
    void createCommitDiscoveryProgress() {
        discoveryProgressService.createCommitDiscoveryProgress(TestData.TRUNK_R1);
        verify(commitDiscoveryProgressTable).save(eq(
                CommitDiscoveryProgress.builder()
                        .arcRevision(TestData.TRUNK_R1)
                        .build()
        ));
    }

    private void mockDatabase() {
        TestCiDbUtils.mockToCallRealTxMethods(db);
        doReturn(commitDiscoveryProgressTable).when(db).commitDiscoveryProgress();
    }

}
