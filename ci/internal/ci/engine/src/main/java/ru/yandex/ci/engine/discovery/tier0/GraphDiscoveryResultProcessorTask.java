package ru.yandex.ci.engine.discovery.tier0;

import java.io.InputStream;
import java.nio.file.Path;
import java.time.Clock;
import java.time.Duration;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Stopwatch;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.sandbox.ProxySandboxClient;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.api.ResourceInfo;
import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommits;
import ru.yandex.ci.engine.discovery.TriggeredProcesses;
import ru.yandex.commune.bazinga.scheduler.ActiveUidBehavior;
import ru.yandex.commune.bazinga.scheduler.ActiveUidDropType;
import ru.yandex.commune.bazinga.scheduler.ActiveUidDuplicateBehavior;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_RESULTS_PROCESSED;

@Slf4j
@NonNullApi
// Class doesn't work with PR's, cause it uses `configurationService.getOrCreateBranchConfig`
public class GraphDiscoveryResultProcessorTask extends AbstractOnetimeTask<GraphDiscoveryResultProcessorTask.Params> {

    private ArcService arcService;
    private CiMainDb db;
    private Clock clock;
    private ConfigurationService configurationService;
    private DiscoveryServicePostCommits discoveryServicePostCommits;
    private ProxySandboxClient proxySandboxClient;
    private RevisionNumberService revisionNumberService;
    private SandboxClient sandboxClient;
    private GraphDiscoveryService graphDiscoveryService;
    private ConfigDiscoveryDirCache configDiscoveryDirCache;

    private Duration timeout;

    private ResultProcessorParameters parameters;

    public GraphDiscoveryResultProcessorTask(
            ArcService arcService,
            CiMainDb db,
            Clock clock,
            ConfigurationService configurationService,
            DiscoveryServicePostCommits discoveryServicePostCommits,
            ProxySandboxClient proxySandboxClient,
            RevisionNumberService revisionNumberService,
            SandboxClient sandboxClient,
            GraphDiscoveryService graphDiscoveryService,
            Duration timeout,
            ConfigDiscoveryDirCache configDiscoveryDirCache,
            ResultProcessorParameters parameters
    ) {
        super(Params.class);
        this.arcService = arcService;
        this.clock = clock;
        this.configurationService = configurationService;
        this.db = db;
        this.discoveryServicePostCommits = discoveryServicePostCommits;
        this.proxySandboxClient = proxySandboxClient;
        this.revisionNumberService = revisionNumberService;
        this.sandboxClient = sandboxClient;
        this.graphDiscoveryService = graphDiscoveryService;
        this.timeout = timeout;
        this.configDiscoveryDirCache = configDiscoveryDirCache;
        this.parameters = parameters;
    }

    public GraphDiscoveryResultProcessorTask(Params params) {
        super(params);
    }


    @Override
    protected void execute(Params params, ExecutionContext context) {
        log.info("Processing sandbox task {}", params.getSandboxTaskId());
        processGraphResults(params.getSandboxTaskId());
        log.info("Processed sandbox task {}", params.getSandboxTaskId());
    }

    void processGraphResults(long sandboxTaskId) {
        var context = new Context(sandboxTaskId);
        var triggeredAYamls = new TriggeredProcessesFinder().run(context);
        runDiscovery(context, triggeredAYamls);
        markAsDiscovered(context);
    }

    private void markAsDiscovered(Context context) {
        db.currentOrTx(() -> {
            var updatedTask = context.getTask().toBuilder()
                    .sandboxResultResourceId(context.getResourceInfo().getId())
                    .sandboxResourceSize(context.getResourceInfo().getSize())
                    .status(GRAPH_RESULTS_PROCESSED)
                    .discoveryFinishedAt(clock.instant())
                    .build();
            db.graphDiscoveryTaskTable().save(updatedTask);
            graphDiscoveryService.markAsDiscovered(context.getOrderedArcRevision());
            log.info("State updated to {} for {}", updatedTask.getStatus(), updatedTask.getId());
        });
        graphDiscoveryService.getProcessedResultCount().incrementAndGet();
    }

    @Value
    private class Context {

        long sandboxTaskId;
        @Nonnull
        GraphDiscoveryTask task;
        @Nonnull
        OrderedArcRevision orderedArcRevision;
        @Nonnull
        ArcCommit commit;
        @Nullable
        ArcRevision previousRevision;
        @Nonnull
        LruCache<Path, Optional<ConfigBundle>> configBundleCache;

        int fileExistsCacheSize;
        int batchSize;

        @Nonnull
        ResourceInfo resourceInfo;

        Context(long sandboxTaskId) {
            this.sandboxTaskId = sandboxTaskId;
            task = db.currentOrReadOnly(() ->
                    db.graphDiscoveryTaskTable().findBySandboxTaskId(sandboxTaskId)
            ).orElseThrow(() -> new GraphDiscoveryException(
                    "GraphDiscoveryTask not found with sandboxTaskId " + sandboxTaskId
            ));

            orderedArcRevision = revisionNumberService.getOrderedArcRevision(
                    ArcBranch.trunk(), ArcRevision.of(task.getId().getCommitId())
            );
            commit = arcService.getCommit(orderedArcRevision);
            previousRevision = ArcCommitUtils.firstParentArcRevision(commit).orElse(null);

            configBundleCache = GraphDiscoveryHelper.createConfigBundleCache(configurationService, commit,
                    orderedArcRevision, getConfigBundleCacheSize());

            fileExistsCacheSize = GraphDiscoveryResultProcessorTask.this.getFileExistsCacheSize();
            batchSize = GraphDiscoveryResultProcessorTask.this.getBatchSize();

            resourceInfo = fetchResourceInfo(sandboxTaskId, parameters, sandboxClient);
        }

        public ArcRevision getPreviousRevisionOrThrow() {
            return Objects.requireNonNull(previousRevision);
        }

        public void logConfigBundleCacheStat() {
            var cache = configBundleCache;
            log.info("{}, configBundleCache: hit rate {}, miss {}, total {}, size {}, elapsed {}s",
                    orderedArcRevision, cache.cacheHitRate(), cache.getMissCount(),
                    cache.getTotalCount(), cache.getMaxSize(),
                    cache.getStopwatch().elapsed(TimeUnit.SECONDS)
            );
        }

        private static ResourceInfo fetchResourceInfo(
                long sandboxTaskId,
                ResultProcessorParameters parameters,
                SandboxClient sandboxClient
        ) {
            var sandboxResultResourceType = parameters.getSandboxResultResourceType();
            var resources = sandboxClient.getTaskResources(sandboxTaskId, sandboxResultResourceType).getItems();
            if (resources.isEmpty()) {
                throw new GraphDiscoveryException(
                        "Sandbox task %d has no resource of type %s".formatted(
                                sandboxTaskId, sandboxResultResourceType
                        ));
            }
            var resourceInfo = resources.get(0);
            log.info("Got sandbox resource id: {}", resourceInfo.getId());
            return resourceInfo;
        }
    }

    private class TriggeredProcessesFinder {

        TriggeredProcesses run(Context context) {
            if (context.getPreviousRevision() == null) {
                log.info("Exiting, cause commit {} has no parents", context.getOrderedArcRevision());
                return TriggeredProcesses.empty();
            }

            log.info("Processing: commit {}, previousCommit {}", context.getOrderedArcRevision(),
                    context.getPreviousRevision());

            try (var resource = proxySandboxClient.downloadResource(context.getResourceInfo().getId())) {
                var sandboxTaskResultResource = resource.getStream();
                log.info("Got stream of sandbox resource");

                var triggeredAYamls = findTriggeredAYamls(context, sandboxTaskResultResource);
                return TriggeredProcesses.of(triggeredAYamls);
            }
        }

        private Set<TriggeredProcesses.Triggered> findTriggeredAYamls(
                Context context,
                InputStream sandboxTaskResultResource
        ) {
            var discoveredAYamlFilter = new AYamlFilterChecker(
                    context.getOrderedArcRevision(),
                    context.getCommit(),
                    context.getPreviousRevisionOrThrow(),
                    discoveryServicePostCommits,
                    context.getConfigBundleCache()
            );

            var affectedPathMatcher = AffectedPathMatcher.composite(
                    new AffectedAYamlsFinder(
                            context.getOrderedArcRevision(),
                            arcService,
                            context.getFileExistsCacheSize(),
                            discoveredAYamlFilter,
                            context.getBatchSize()
                    ),
                    new AffectedAbsPathMatcher(
                            discoveredAYamlFilter,
                            configDiscoveryDirCache,
                            context.getBatchSize()
                    )
            );

            var triggeredAYamls = findTriggeredAYamls(context.getResourceInfo().getFileName(),
                    sandboxTaskResultResource, affectedPathMatcher);
            context.logConfigBundleCacheStat();
            return triggeredAYamls;
        }

        private static Set<TriggeredProcesses.Triggered> findTriggeredAYamls(
                String sandboxResourceFileName,
                InputStream sandboxTaskResultResource,
                AffectedPathMatcher affectedPathMatcher
        ) {
            var parser = new ChangesDetectorSandboxTaskResultParser(affectedPathMatcher);
            log.info("Parsing started");
            parser.parse(sandboxResourceFileName, sandboxTaskResultResource);
            affectedPathMatcher.flush();
            log.info("Parsing finished");
            return affectedPathMatcher.getTriggered();
        }

    }

    private void runDiscovery(Context context, TriggeredProcesses triggeredAYamls) {
        log.info("Started processing affected aYamls");
        var discoveryServiceStopwatch = Stopwatch.createUnstarted();
        triggeredAYamls.groupByAYamlPath().forEach(
                (aYamlPath, triggeredProcesses) -> {
                    log.info("Started discovery for {} {}", aYamlPath, triggeredProcesses);

                    var configBundle = context.getConfigBundleCache().get(aYamlPath).orElse(null);
                    if (configBundle == null) {
                        log.info("No config bundle found for: {} {}", aYamlPath, triggeredProcesses);
                        // no valid config bundle exists
                        return;
                    }

                    var discoveryContext = GraphDiscoveryHelper.discoveryContextBuilder(false)
                            .revision(context.getOrderedArcRevision())
                            .previousRevision(context.getPreviousRevisionOrThrow())
                            .commit(context.getCommit())
                            .configBundle(configBundle)
                            .affectedPathsProvider(arcService)
                            .build();

                    discoveryServiceStopwatch.start();
                    try {
                        discoveryServicePostCommits.processTriggeredProcesses(discoveryContext, triggeredProcesses);
                    } finally {
                        discoveryServiceStopwatch.stop();
                    }
                    log.info("Finished discovery for {} {}", aYamlPath, triggeredProcesses);
                });
        context.logConfigBundleCacheStat();
        log.info("Finished processing affected aYamls, discoveryService.processTriggeredProcesses elapsed {}s",
                discoveryServiceStopwatch.elapsed(TimeUnit.SECONDS));
    }

    private int getFileExistsCacheSize() {
        var fileExistsCacheSize = parameters.getDefaultFileExistsCacheSize();
        return db.currentOrReadOnly(() ->
                db.keyValue().getInt("GraphDiscoveryService", "fileExistsCacheSize", fileExistsCacheSize)
        );
    }

    private int getConfigBundleCacheSize() {
        var configBundleCacheSize = parameters.getDefaultConfigBundleCacheSize();
        return db.currentOrReadOnly(() ->
                db.keyValue().getInt("GraphDiscoveryService", "configBundleCacheSize", configBundleCacheSize)
        );
    }

    private int getBatchSize() {
        var batchSize = parameters.getDefaultBatchSize();
        return db.currentOrReadOnly(() ->
                db.keyValue().getInt("GraphDiscoveryService", "batchSize", batchSize)
        );
    }

    @Override
    public Duration getTimeout() {
        return timeout;
    }

    @Override
    public ActiveUidBehavior activeUidBehavior() {
        return new ActiveUidBehavior(ActiveUidDropType.WHEN_FINISHED, ActiveUidDuplicateBehavior.DO_NOTHING);
    }


    @Value
    @NonNullApi
    @BenderBindAllFields
    public static class Params {
        long sandboxTaskId;

        // fields are visible in bazinga task (for easy debugging)
        String commitId;
        long leftSvnRevision;
        long rightSvnRevision;
        // Bender doesn't support NavigableSet, so we use list
        List<GraphDiscoveryTask.Platform> platforms;

        public Params(String commitId, long leftSvnRevision, long rightSvnRevision, long sandboxTaskId,
                      Set<GraphDiscoveryTask.Platform> platforms) {
            this.commitId = commitId;
            this.leftSvnRevision = leftSvnRevision;
            this.rightSvnRevision = rightSvnRevision;
            this.platforms = platforms.stream().sorted().toList();
            this.sandboxTaskId = sandboxTaskId;
        }

    }

}
