package ru.yandex.ci.engine.discovery.pci_dss;

import java.time.Duration;
import java.util.List;
import java.util.Map;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.client.pciexpress.PciExpressClient;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.ci.engine.discovery.tier0.ProcessPciDssCommitTask;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressChecker;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;
import ru.yandex.repo.pciexpress.proto.api.Api;

@NonNullApi
public class PciDssDiscoveryProgressCheckerCronTask extends CiEngineCronTask {

    private final DiscoveryProgressChecker checker;
    private final PciExpressClient pciExpressClient;
    private final CiMainDb db;
    private final BazingaTaskManager bazingaTaskManager;

    public PciDssDiscoveryProgressCheckerCronTask(
            DiscoveryProgressChecker checker,
            Duration runDelay,
            Duration timeout,
            PciExpressClient pciExpressClient,
            CiMainDb db,
            BazingaTaskManager bazingaTaskManager,
            CuratorFramework curator) {
        super(runDelay, timeout, curator);
        this.checker = checker;
        this.bazingaTaskManager = bazingaTaskManager;
        this.pciExpressClient = pciExpressClient;
        this.db = db;
    }

    @Override
    protected void executeImpl(ExecutionContext executionContext) throws Exception {
        db.currentOrTx((Runnable) this::execute);
    }

    private void execute() {
        List<String> commitIds = checker.findNotProcessedPciDssCommits();
        log.info("Found [{}] not processed pci dss commits", commitIds.size());

        Map<String, Api.CommitStatus> pciDssCommitIdToStatus = pciExpressClient.getPciDssCommitIdToStatus(commitIds);

        for (String commitId : commitIds) {
            Api.CommitStatus commitStatus = pciDssCommitIdToStatus.getOrDefault(commitId, Api.CommitStatus.UNKNOWN);

            CommitDiscoveryProgress.PciDssState pciDssState;
            if (commitStatus == Api.CommitStatus.PCI) {
                var parameters = new ProcessPciDssCommitTask.Parameters(commitId);
                var bazingaJobId = bazingaTaskManager.schedule(new ProcessPciDssCommitTask(parameters));
                log.info("ProcessPciDssCommitTask job scheduled: bazingaJobId {}, commitId {}",
                        bazingaJobId, commitId);
                pciDssState = CommitDiscoveryProgress.PciDssState.PROCESSING;
            } else if (commitStatus == Api.CommitStatus.PCIFREE) {
                pciDssState = CommitDiscoveryProgress.PciDssState.PROCESSED;
            } else {
                pciDssState = CommitDiscoveryProgress.PciDssState.NOT_PROCESSED;
            }

            CommitDiscoveryProgress updated = db.commitDiscoveryProgress().find(commitId)
                    .orElseThrow()
                    .withPciDssState(pciDssState);
            db.commitDiscoveryProgress().save(updated);
        }
    }
}
