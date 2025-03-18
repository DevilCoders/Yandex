package ru.yandex.ci.engine.discovery.tier0;

import java.time.Duration;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.arc.ArcRevision;
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
public class GraphDiscoveryStartTask extends AbstractOnetimeTask<GraphDiscoveryStartTask.Params> {

    private static final CompoundReschedulePolicy RESCHEDULE_POLICY = new CompoundReschedulePolicy(
            // 5, 10, 15, 20
            new RescheduleLinear(org.joda.time.Duration.standardSeconds(5), 4),
            // 60, 60, 60, ...
            new RescheduleConstant(
                    org.joda.time.Duration.standardMinutes(1),
                    Integer.MAX_VALUE - 4
            )
    );

    private GraphDiscoveryService graphDiscoveryService;

    public GraphDiscoveryStartTask(GraphDiscoveryService graphDiscoveryService) {
        super(Params.class);
        this.graphDiscoveryService = graphDiscoveryService;
    }

    public GraphDiscoveryStartTask(Params params) {
        super(params);
    }

    @Override
    protected void execute(Params params, ExecutionContext context) {
        log.info("Starting for (right: {} ({}), {} ({}), {})", params.getRightSvnRevision(), params.getRightRevision(),
                params.getLeftSvnRevision(), params.getLeftRevision(), params.getPlatforms());
        graphDiscoveryService.startGraphDiscoverySandboxTask(
                ArcRevision.of(params.getRightRevision()),
                ArcRevision.of(params.getLeftRevision()),
                params.getRightSvnRevision(),
                params.getLeftSvnRevision(),
                new HashSet<>(params.getPlatforms())
        );
        log.info("Started for (right: {} ({}), {} ({}), {})", params.getRightSvnRevision(), params.getRightRevision(),
                params.getLeftSvnRevision(), params.getLeftRevision(), params.getPlatforms());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(1);
    }

    @Override
    public ReschedulePolicy reschedulePolicy() {
        return RESCHEDULE_POLICY;
    }

    @Value
    @NonNullApi
    @BenderBindAllFields
    public static class Params {
        String commitHash;    // TODO: delete this field

        String rightRevision;
        String leftRevision;

        long rightSvnRevision;
        long leftSvnRevision;
        // Bender doesn't support NavigableSet, so we use list
        List<GraphDiscoveryTask.Platform> platforms;

        public Params(String rightRevision, String leftRevision, long rightSvnRevision, long leftSvnRevision,
                      Set<GraphDiscoveryTask.Platform> platforms) {
            this.commitHash = rightRevision;
            this.rightRevision = rightRevision;
            this.leftRevision = leftRevision;
            this.rightSvnRevision = rightSvnRevision;
            this.leftSvnRevision = leftSvnRevision;
            this.platforms = platforms.stream().sorted().collect(Collectors.toList());
        }

        public String getRightRevision() {
            return Objects.requireNonNullElse(rightRevision, commitHash);
        }

    }
}
