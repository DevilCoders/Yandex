package ru.yandex.ci.engine.discovery.arc_reflog;

import java.util.List;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.base.Stopwatch;
import lombok.AllArgsConstructor;
import lombok.Value;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.discovery.task.ProcessPostCommitTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;
import ru.yandex.ci.engine.discovery.util.IgnoredBranches;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@AllArgsConstructor
public class ReflogProcessorService {

    private static final Logger log = LoggerFactory.getLogger(ReflogProcessorService.class);

    private static final String REFLOG_PROCESSOR_SERVICE_NS = "ReflogProcessorService";
    private static final int COMMIT_LIMIT_IN_REFLOG = 10_000;

    private final BazingaTaskManager taskManager;
    private final DiscoveryProgressService discoveryProgressService;
    private final GraphDiscoveryService graphDiscoveryService;
    private final ArcService arcService;
    private final CiMainDb db;

    public void processReflogRecord(ArcBranch branch, ArcRevision revision, @Nullable ArcRevision previousRevision) {
        var commits = collectCommits(branch, revision, previousRevision);
        for (ArcCommit commit : commits) {
            db.tx(() -> {
                db.arcCommit().save(commit); // Отслеживаем процессинг каждого коммита
                switch (branch.getType()) {
                    case TRUNK -> processTrunkCommit(commit);
                    case RELEASE_BRANCH -> processBranchCommit(commit, branch);
                    default -> throw new IllegalStateException("unreachable");
                }
            });
        }
    }

    public List<ArcCommit> collectCommits(ArcBranch branch,
                                          ArcRevision revision,
                                          @Nullable ArcRevision previousRevision) {
        log.info(
                "Processing reflog record in branch {}, revision {}, previousRevision {}",
                branch, revision, previousRevision
        );

        if (!acceptBranch(branch)) {
            return List.of();
        }

        if (branch.isTrunk()) {
            Preconditions.checkState(previousRevision != null,
                    "Previous revision must be provided for trunk commit %s", revision);
        } else {
            @Nullable
            String mergeBase = getMergeBase(branch);
            if (mergeBase == null) {
                log.info("Skipping branch {} cause it does not have merge base", branch);
                return List.of();
            }
            var mergeBaseCommit = arcService.getCommit(ArcRevision.of(mergeBase));
            if (!mergeBaseCommit.isTrunk()) {
                log.info("Skipping branch {} cause it has merge base with trunk ({}) not in trunk",
                        branch, mergeBase);
                return List.of();
            }
            if (previousRevision == null) {
                previousRevision = ArcRevision.of(mergeBase);
            }
        }

        List<ArcCommit> commits = arcService.getCommits(revision, previousRevision, COMMIT_LIMIT_IN_REFLOG);
        if (commits.size() >= COMMIT_LIMIT_IN_REFLOG) {
            log.warn(
                    "More than {} commit in reflog record (between {} and {}). Processing only first {}.",
                    COMMIT_LIMIT_IN_REFLOG, revision, previousRevision, COMMIT_LIMIT_IN_REFLOG
            );
        }

        return commits;
    }

    private void processBranchCommit(ArcCommit commit, ArcBranch branch) {
        taskManager.schedule(
                new ProcessPostCommitTask(new ProcessPostCommitTask.Params(branch, commit.getRevision()))
        );
    }

    private void processTrunkCommit(ArcCommit commit) {
        log.info("Processing trunk commit: {}", commit);

        if (commit.getParents().isEmpty()) {
            log.warn("Commit without parents, skipping. {}", commit);
            return;
        }

        OrderedArcRevision revision = commit.toOrderedTrunkArcRevision();

        discoveryProgressService.createCommitDiscoveryProgress(revision);

        taskManager.schedule(
                new ProcessPostCommitTask(new ProcessPostCommitTask.Params(ArcBranch.trunk(), revision.toRevision()))
        );

        ArcCommit previousCommit = arcService.getCommit(ArcCommitUtils.firstParentArcRevision(commit).orElseThrow());
        graphDiscoveryService.scheduleGraphDiscovery(
                previousCommit, commit, commit.toOrderedTrunkArcRevision()
        );

        log.info("Processed trunk commit: {}", commit);
    }


    @Nullable
    private String getMergeBase(ArcBranch branch) {

        if (branch.isTrunk()) {
            return null;
        }

        var branchBase = db.tx(() ->
                db.keyValue().findObject(REFLOG_PROCESSOR_SERVICE_NS, branch.asString(), BranchBase.class)
                        .orElseGet(() -> calculateBranchBase(branch))
        );

        log.info("Branch {}, {}", branch, branchBase);
        return branchBase.baseRevision;
    }

    private BranchBase calculateBranchBase(ArcBranch branch) {
        Preconditions.checkState(arcService != null);
        Preconditions.checkState(db != null);

        ArcRevision trunkHead = arcService.getLastRevisionInBranch(ArcBranch.trunk());
        ArcRevision branchHead = arcService.getLastRevisionInBranch(branch);

        Stopwatch stopwatch = Stopwatch.createStarted();
        log.info("Calculating base with {} and {}", trunkHead.getCommitId(), branchHead.getCommitId());
        BranchBase calculated = arcService.getMergeBase(trunkHead, branchHead)
                .map(rev -> new BranchBase(rev.getCommitId()))
                .orElse(BranchBase.WITHOUT_BASE);

        stopwatch.stop();
        log.info("Calculated in {}s", stopwatch.elapsed().toSeconds());

        db.keyValue().setValue(REFLOG_PROCESSOR_SERVICE_NS, branch.asString(), calculated);
        return calculated;
    }

    public static boolean acceptBranch(ArcBranch branch) {
        if (!branch.isTrunk() && !branch.isRelease()) {
            log.info("Skipping unsupported branch type {} branch: {}", branch.getType(), branch);
            return false;
        }

        if (IgnoredBranches.isIgnored(branch)) {
            log.info("Branch {} ignored", branch);
            return false;
        }

        return true;
    }

    @Value
    private static class BranchBase {
        public static final BranchBase WITHOUT_BASE = new BranchBase(null);

        @Nullable
        String baseRevision;

    }


}
