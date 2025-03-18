package ru.yandex.ci.engine.discovery.pci_dss;

import java.time.Duration;
import java.util.List;
import java.util.Map;

import org.apache.curator.framework.CuratorFramework;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.client.pciexpress.PciExpressClient;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.discovery.tier0.ProcessPciDssCommitTask;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressChecker;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.repo.pciexpress.proto.api.Api;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

class PciDssDiscoveryProgressCheckerCronTaskTest extends EngineTestBase {

    PciDssDiscoveryProgressCheckerCronTask task;
    @Autowired
    DiscoveryProgressChecker checker;
    @Autowired
    PciExpressClient pciExpressClient;
    @Autowired
    BazingaTaskManager bazingaTaskManager;
    @Autowired
    CuratorFramework curatorFramework;

    @BeforeEach
    void setUp() {
        task = new PciDssDiscoveryProgressCheckerCronTask(
                checker,
                Duration.ofSeconds(10L),
                Duration.ofSeconds(120L),
                pciExpressClient,
                db,
                bazingaTaskManager,
                curatorFramework
        );
    }

    @Test
    void executeImpl() throws Exception {
        Map<OrderedArcRevision, CommitDiscoveryProgress.PciDssState> map = Map.of(
                TestData.TRUNK_R1, CommitDiscoveryProgress.PciDssState.NOT_PROCESSED,
                TestData.TRUNK_R2, CommitDiscoveryProgress.PciDssState.NOT_PROCESSED,
                TestData.TRUNK_R3, CommitDiscoveryProgress.PciDssState.NOT_PROCESSED,
                TestData.TRUNK_R4, CommitDiscoveryProgress.PciDssState.PROCESSING,
                TestData.TRUNK_R5, CommitDiscoveryProgress.PciDssState.PROCESSED
        );
        List<CommitDiscoveryProgress> commitDiscoveryProgresses = map.entrySet().stream()
                .map(e -> CommitDiscoveryProgress.builder()
                        .arcRevision(e.getKey())
                        .pciDssState(e.getValue())
                        .build())
                .toList();
        db.currentOrTx(() -> db.commitDiscoveryProgress().save(commitDiscoveryProgresses));

        List<ArcCommit> commits = List.of(
                TestData.TRUNK_COMMIT_1,
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_COMMIT_3,
                TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5
        );
        db.currentOrTx(() -> db.arcCommit().save(commits));

        String commitIdr0 = TestData.TRUNK_R1.getCommitId();
        String commitIdr1 = TestData.TRUNK_R2.getCommitId();
        String commitIdr2 = TestData.TRUNK_R3.getCommitId();
        List<String> notProcessedCommitIds = List.of(
                commitIdr0,
                commitIdr1,
                commitIdr2
        );
        when(pciExpressClient.getPciDssCommitIdToStatus(notProcessedCommitIds))
                .thenReturn(
                        Map.of(
                                commitIdr0, Api.CommitStatus.PCI,
                                commitIdr1, Api.CommitStatus.PCIFREE,
                                commitIdr2, Api.CommitStatus.UNKNOWN
                        )
                );

        task.executeImpl(mock(ExecutionContext.class));

        db.currentOrReadOnly(() -> {
            check(TestData.TRUNK_R1.getCommitId(), CommitDiscoveryProgress.PciDssState.PROCESSING);
            check(TestData.TRUNK_R2.getCommitId(), CommitDiscoveryProgress.PciDssState.PROCESSED);
            check(TestData.TRUNK_R3.getCommitId(), CommitDiscoveryProgress.PciDssState.NOT_PROCESSED);
            check(TestData.TRUNK_R4.getCommitId(), CommitDiscoveryProgress.PciDssState.PROCESSING);
            check(TestData.TRUNK_R5.getCommitId(), CommitDiscoveryProgress.PciDssState.PROCESSED);
        });

        verify(bazingaTaskManager, times(1))
                .schedule(any(ProcessPciDssCommitTask.class));
    }

    private void check(String commitId, CommitDiscoveryProgress.PciDssState state) {
        var pciDssState = db.commitDiscoveryProgress().find(commitId)
                .orElseThrow()
                .getPciDssState();
        assertThat(pciDssState)
                .withFailMessage("%s commit should be %s but %s", commitId, state, pciDssState)
                .isEqualTo(state);
    }
}
