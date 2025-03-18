package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.UUID;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.autocheck.stress_test.StressTest;
import ru.yandex.ci.client.observer.CheckRevisionsDto;
import ru.yandex.ci.client.observer.ObserverClient;
import ru.yandex.ci.client.observer.OrderedRevisionDto;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.storage.core.Common;

@RequiredArgsConstructor
@ExecutorInfo(
        title = "Find target revisions for precommit stress test",
        description = "Internal job for finding target revisions for precommit stress test"
)
@Produces(single = StressTest.StressTestRevisions.class)
@Slf4j
public class StressTestRevisionsFindJob implements JobExecutor {

    public static final UUID ID = UUID.fromString("21a25fc5-0070-42b1-a482-3d5c54fa5724");

    @Nonnull
    private final ObserverClient observerClient;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var flowVars = StressTestFlowVars.getFlowVars(context);
        var durationHours = StressTestFlowVars.getDurationHours(flowVars) + "h";
        var revisionsPerHours = StressTestFlowVars.getRevisionsPerHours(flowVars);
        var namespace = StressTestFlowVars.getNamespace(flowVars);

        log.info("Namespace {}, revisionsPerHours {}, durationHours {}", namespace, revisionsPerHours, durationHours);

        var revision = context.getFlowLaunch().getVcsInfo().getRevision();
        var revisions = observerClient.getNotUsedRevisions(revision.getCommitId(), durationHours,
                revisionsPerHours, namespace);

        var stressTestRevisionsProto = StressTest.StressTestRevisions.newBuilder()
                .addAllRevisions(revisions.stream()
                        .map(StressTestRevisionsFindJob::toProtoCheckRevisions)
                        .toList()
                )
                .build();

        log.info("Found {} revisions", revisions.size());

        context.resources().produce(Resource.of(stressTestRevisionsProto, "revisions"));
    }

    private static StressTest.CheckRevisions toProtoCheckRevisions(CheckRevisionsDto it) {
        return StressTest.CheckRevisions.newBuilder()
                .setLeft(toProtoOrderedRevision(it.getLeft()))
                .setRight(toProtoOrderedRevision(it.getRight()))
                .setDiffSetId(it.getDiffSetId())
                .build();
    }

    private static Common.OrderedRevision toProtoOrderedRevision(OrderedRevisionDto source) {
        return Common.OrderedRevision.newBuilder()
                .setBranch(source.getBranch())
                .setRevision(source.getRevision())
                .setRevisionNumber(source.getRevisionNumber())
                .build();
    }

}
