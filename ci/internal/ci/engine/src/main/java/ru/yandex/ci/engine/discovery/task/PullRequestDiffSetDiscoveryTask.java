package ru.yandex.ci.engine.discovery.task;

import java.time.Duration;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.discovery.DiscoveryServicePullRequests;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@NonNullApi
public class PullRequestDiffSetDiscoveryTask extends AbstractOnetimeTask<PullRequestDiffSetDiscoveryTask.Params> {

    private static final Logger log = LoggerFactory.getLogger(PullRequestDiffSetDiscoveryTask.class);

    private final DiscoveryServicePullRequests discoveryServicePullRequests;
    private final CiMainDb db;

    public PullRequestDiffSetDiscoveryTask(DiscoveryServicePullRequests discoveryServicePullRequests, CiMainDb db) {
        super(Params.class);
        this.discoveryServicePullRequests = discoveryServicePullRequests;
        this.db = db;
    }

    public PullRequestDiffSetDiscoveryTask(long pullRequestId, long diffSetId) {
        super(new Params(pullRequestId, diffSetId));
        discoveryServicePullRequests = null;
        db = null;
    }

    @Override
    protected void execute(Params params, ExecutionContext context) {
        log.info("Processing pr {} diffSet {}.", params.getPullRequestId(), params.getDiffSetId());

        db.currentOrTx(() -> {
            var diffSet = db.pullRequestDiffSetTable().getById(params.getPullRequestId(), params.getDiffSetId());
            var launched = discoveryServicePullRequests.processDiffSet(diffSet);
            log.info("Launched total {} pull requests", launched.size());
        });

        log.info("Pr {}, diffSet {} processed.", params.getPullRequestId(), params.getDiffSetId());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(10);
    }

    @NonNullApi
    @BenderBindAllFields
    public static class Params {
        private final long pullRequestId;
        private final long diffSetId;

        public Params(long pullRequestId, long diffSetId) {
            this.pullRequestId = pullRequestId;
            this.diffSetId = diffSetId;
        }

        public long getPullRequestId() {
            return pullRequestId;
        }

        public long getDiffSetId() {
            return diffSetId;
        }
    }
}
