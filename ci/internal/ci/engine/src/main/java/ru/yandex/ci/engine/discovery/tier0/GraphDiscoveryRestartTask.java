package ru.yandex.ci.engine.discovery.tier0;

import java.time.Duration;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.schedule.CompoundReschedulePolicy;
import ru.yandex.commune.bazinga.scheduler.schedule.RescheduleConstant;
import ru.yandex.commune.bazinga.scheduler.schedule.RescheduleLinear;
import ru.yandex.commune.bazinga.scheduler.schedule.ReschedulePolicy;
import ru.yandex.lang.NonNullApi;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Slf4j
@NonNullApi
public class GraphDiscoveryRestartTask extends AbstractOnetimeTask<GraphDiscoveryRestartTask.Params> {

    private static final CompoundReschedulePolicy RESCHEDULE_POLICY = new CompoundReschedulePolicy(
            // 5, 10, 15, 20
            new RescheduleLinear(org.joda.time.Duration.standardSeconds(5), 4),
            // 60, 60, 60, ...
            new RescheduleConstant(
                    org.joda.time.Duration.standardMinutes(1),
                    Integer.MAX_VALUE - 4
            )
    );

    private CiMainDb db;
    private GraphDiscoveryService graphDiscoveryService;

    public GraphDiscoveryRestartTask(CiMainDb db, GraphDiscoveryService graphDiscoveryService) {
        super(GraphDiscoveryRestartTask.Params.class);
        this.db = db;
        this.graphDiscoveryService = graphDiscoveryService;
    }

    public GraphDiscoveryRestartTask(GraphDiscoveryRestartTask.Params params) {
        super(params);
    }


    @Override
    protected void execute(Params params, ExecutionContext context) throws Exception {
        log.info("Restarting sandboxTaskId {}", params.getSandboxTaskId());
        var task = db.currentOrReadOnly(() ->
                db.graphDiscoveryTaskTable().findBySandboxTaskId(params.getSandboxTaskId())
        ).orElseThrow(() -> new GraphDiscoveryException(
                "GraphDiscoveryTask not found by sandboxTaskId: " + params.getSandboxTaskId()
        ));
        graphDiscoveryService.restartGraphDiscoverySandboxTask(List.of(task));
        log.info("Restarted sandboxTaskId {}", params.getSandboxTaskId());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(1);
    }

    @Override
    public ReschedulePolicy reschedulePolicy() {
        return RESCHEDULE_POLICY;
    }


    @NonNullApi
    @BenderBindAllFields
    public static final class Params {
        private final long sandboxTaskId;
        private final String commitHash;
        private final long leftSvnRevision;
        private final long rightSvnRevision;
        // Bender doesn't support NavigableSet, so we use list
        private final List<GraphDiscoveryTask.Platform> platforms;

        public Params(long sandboxTaskId, String commitHash, long leftSvnRevision, long rightSvnRevision,
                      Set<GraphDiscoveryTask.Platform> platforms) {
            this.sandboxTaskId = sandboxTaskId;
            this.commitHash = commitHash;
            this.rightSvnRevision = rightSvnRevision;
            this.leftSvnRevision = leftSvnRevision;
            this.platforms = platforms.stream().sorted().collect(Collectors.toList());
        }

        public long getSandboxTaskId() {
            return sandboxTaskId;
        }

        public String getCommitHash() {
            return commitHash;
        }

        public long getLeftSvnRevision() {
            return leftSvnRevision;
        }

        public long getRightSvnRevision() {
            return rightSvnRevision;
        }

        public List<GraphDiscoveryTask.Platform> getPlatforms() {
            return platforms;
        }
    }

}
