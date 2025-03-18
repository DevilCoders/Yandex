package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.UUID;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

@RequiredArgsConstructor
@ExecutorInfo(
        title = "Mark commit Storage discovered",
        description = "Internal job for marking storage commit as discovered, for skipped commits"
)
public class MarkStorageDiscoveredJob implements JobExecutor {

    public static final UUID ID = UUID.fromString("d9daf970-53de-49e4-b3fc-9c82c7650ff7");

    private final DiscoveryProgressService discoveryProgressService;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var revision = context.getFlowLaunch().getTargetRevision();
        discoveryProgressService.markAsDiscovered(revision, DiscoveryType.STORAGE);
    }
}
