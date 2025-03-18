package ru.yandex.ci.event.arcanum;

import java.time.Instant;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.arcanum.event.ArcanumEvents;
import ru.yandex.arcanum.event.ArcanumModels;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.config.a.model.CleanupReason;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.pr.PullRequest;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.engine.discovery.task.PullRequestDiffSetDiscoveryTask;
import ru.yandex.ci.engine.discovery.task.PullRequestDiffSetProcessedByCiTask;
import ru.yandex.ci.engine.discovery.util.IgnoredBranches;
import ru.yandex.ci.engine.launch.cleanup.PullRequestDiffSetCompleteTask;
import ru.yandex.commune.bazinga.BazingaTaskManager;

public class ArcanumEventService {
    private static final Logger log = LoggerFactory.getLogger(ArcanumEventService.class);

    private final BazingaTaskManager taskManager;
    private final String authorFilter;
    private final CiMainDb db;

    public ArcanumEventService(BazingaTaskManager taskManager, String authorFilter, CiMainDb db) {
        this.taskManager = taskManager;
        this.authorFilter = authorFilter;
        this.db = db;
    }

    public void processEvent(ArcanumEvents.Event event, @Nullable Instant eventCreated) {
        log.info("Processing event from Arcanum: {}", event.getTypeCase());
        switch (event.getTypeCase()) {
            case ARC_HEADS_UPDATED -> log.info(
                    "Ignore {}, now processing on DIFF_SET_IS_READY_FOR_AUTO_CHECK", event.getTypeCase()
            );
            case REVIEW_REQUEST_CLOSED -> {
                var closed = event.getReviewRequestClosed();
                processDiffSetComplete(closed.getReviewRequest(), closed.getActiveDiffSet(),
                        closed.getActiveArcBranchHeads(), closed.getMergeCommitsCount() == 0);
            }
            case DIFF_SET_IS_READY_FOR_AUTO_CHECK -> processDiffSetReadyForAutoCheck(
                    event.getDiffSetIsReadyForAutoCheck(), eventCreated
            );
            default -> log.info("Ignore {}", event.getTypeCase());
        }

    }

    private void processDiffSetComplete(ArcanumModels.ReviewRequest request,
                                        ArcanumModels.DiffSet diffSet,
                                        ArcanumModels.ArcBranchHeads heads,
                                        boolean discarded) {
        if (isFilteredByAuthorFilter(request)) {
            return;
        }

        var id = PullRequestDiffSet.Id.of(request.getId(), diffSet.getId());
        log.info("processDiffSetComplete on {}", id);

        if (id.getDiffSetId() <= 0) {
            return; // --- no active set
        }

        ArcBranch toBranch = getToBranch(request);
        if (!isSuitableTargetBranch(toBranch)) {
            return;
        }
        if (!isSuitableHeads(heads)) {
            return;
        }

        Preconditions.checkState(toBranch != null, "toBranch cannot be null at this point");

        var reason = discarded
                ? CleanupReason.PR_DISCARDED
                : CleanupReason.PR_MERGED;
        db.currentOrTx(() -> {
            var diffSetOpt = db.pullRequestDiffSetTable().find(id);
            if (diffSetOpt.isEmpty()) {
                log.warn("Diff set {} does not exists, bypass with completion on {}", id, reason);
                var ciDiffSet = convertToDiffSet(request, diffSet, heads, toBranch, null)
                        .withStatus(PullRequestDiffSet.Status.SKIP);
                db.pullRequestDiffSetTable().save(ciDiffSet);
            }
            closeActiveDiffSets(id, true, reason);
        });
    }

    private boolean isFilteredByAuthorFilter(ArcanumModels.ReviewRequest reviewRequest) {
        if (Strings.isNullOrEmpty(authorFilter)) {
            return false;
        }
        String prAuthor = reviewRequest.getAuthor().getName();
        if (!authorFilter.equals(prAuthor)) {
            log.info(
                    "PR {} event filtered cause PR author is {} (not {})",
                    reviewRequest.getId(), prAuthor, authorFilter
            );
            return true;
        }
        return false;
    }

    private void processDiffSetReadyForAutoCheck(
            ArcanumEvents.DiffSetEvent diffSetEvent,
            @Nullable Instant eventCreated
    ) {
        ArcanumModels.ReviewRequest reviewRequest = diffSetEvent.getReviewRequest();
        if (isFilteredByAuthorFilter(reviewRequest)) {
            return;
        }

        var id = PullRequestDiffSet.Id.of(reviewRequest.getId(), diffSetEvent.getDiffSet().getId());
        log.info("processArcHeadsUpdatedEventAndReadyForAutoCheck on {}", id);

        var heads = diffSetEvent.getArcBranchHeads();

        ArcBranch toBranch = getToBranch(reviewRequest);
        var suitableForCi = isSuitableTargetBranch(toBranch) && isSuitableHeads(heads);
        if (!suitableForCi) {
            var jobId = taskManager.schedule(new PullRequestDiffSetProcessedByCiTask(id, true));
            log.info("Bazinga job scheduled for {}, id: {}", id, jobId);
            return;
        }

        Preconditions.checkState(toBranch != null, "toBranch cannot be null at this point");

        db.currentOrTx(() -> {
            closeActiveDiffSets(id, false, CleanupReason.NEW_DIFF_SET);

            var diffSetOpt = db.pullRequestDiffSetTable().find(id);
            if (diffSetOpt.isPresent()) {
                log.warn("Diff set {} already exists, skip double-register", id);
                return; // ---
            }

            var diffSet = convertToDiffSet(reviewRequest,
                    diffSetEvent.getDiffSet(),
                    heads,
                    toBranch,
                    eventCreated
            );

            log.info("Saving diff set for discovery: {}", diffSet.getId());
            db.pullRequestDiffSetTable().save(diffSet);
            taskManager.schedule(new PullRequestDiffSetDiscoveryTask(
                    diffSet.getPullRequestId(), diffSet.getDiffSetId()));
        });
    }

    private void closeActiveDiffSets(PullRequestDiffSet.Id currentId,
                                     boolean includeCurrent,
                                     CleanupReason cleanupReason) {
        var activeDiffSets = db.pullRequestDiffSetTable().findAllWithStatus(currentId.getPullRequestId(),
                Set.of(PullRequestDiffSet.Status.NEW, PullRequestDiffSet.Status.DISCOVERED));
        for (var diffSet : activeDiffSets) {
            if ((diffSet.getDiffSetId() < currentId.getDiffSetId() ||
                    (includeCurrent && diffSet.getDiffSetId() <= currentId.getDiffSetId()))
                    && !diffSet.getScheduleCompletion()) {

                log.info("Scheduling diff set {} for completion", diffSet.getId());

                db.pullRequestDiffSetTable().save(diffSet.withScheduleCompletion(true));

                var task = new PullRequestDiffSetCompleteTask(
                        diffSet.getPullRequestId(), diffSet.getDiffSetId(), cleanupReason);
                taskManager.schedule(task);
            }
        }
    }

    private static PullRequestDiffSet convertToDiffSet(ArcanumModels.ReviewRequest reviewRequest,
                                                       ArcanumModels.DiffSet diffSet,
                                                       ArcanumModels.ArcBranchHeads heads,
                                                       ArcBranch toBranch,
                                                       @Nullable Instant diffSetEventCreated) {
        return new PullRequestDiffSet(
                toPullRequest(reviewRequest, toBranch),
                diffSet.getId(),
                toVcsInfo(reviewRequest, heads, toBranch),
                null,
                reviewRequest.getIssuesList()
                        .stream()
                        .map(ArcanumModels.Issue::getId)
                        .collect(Collectors.toList()),
                reviewRequest.getLabelsList(),
                diffSetEventCreated,
                diffSet.getType()
        );
    }

    private static PullRequest toPullRequest(ArcanumModels.ReviewRequest reviewRequest, ArcBranch toBranch) {
        return new PullRequest(
                reviewRequest.getId(),
                reviewRequest.getAuthor().getName(),
                reviewRequest.getSummary().getValue(),
                reviewRequest.getDescription().getValue(),
                toBranch
        );
    }

    private static PullRequestVcsInfo toVcsInfo(ArcanumModels.ReviewRequest reviewRequest,
                                                ArcanumModels.ArcBranchHeads heads,
                                                ArcBranch toBranch) {
        return new PullRequestVcsInfo(
                ArcRevision.of(heads.getMergeId()),
                ArcRevision.of(heads.getToId()),
                toBranch,
                // бывают исключения
                ArcRevision.of(heads.getFromId()),
                getFromBranch(reviewRequest)
        );
    }

    private boolean isSuitableHeads(ArcanumModels.ArcBranchHeads heads) {
        if (heads.getMergeId().isEmpty()) {
            log.warn("No arc head in Arcanum event. Skipping...");
            return false;
        }
        return true;
    }

    private boolean isSuitableTargetBranch(@Nullable ArcBranch toBranch) {
        if (toBranch == null) {
            log.warn("No toBranch in Arcanum event. Skipping...");
            return false;
        }
        if (IgnoredBranches.isIgnored(toBranch)) {
            log.info("Branch {} ignored by prefix CI-1567", toBranch);
            return false;
        }

        switch (toBranch.getType()) {
            case TRUNK:
            case RELEASE_BRANCH:
                return true;
            default:
                log.warn("Branch is not suitable for PR {}. Skipping...", toBranch);
                return false;
        }
    }

    @Nullable
    private static ArcBranch getToBranch(ArcanumModels.ReviewRequest reviewRequest) {
        if (reviewRequest.hasToBranch() && !Strings.isNullOrEmpty(reviewRequest.getToBranch().getName())) {
            return ArcBranch.ofBranchName(reviewRequest.getToBranch().getName());
        }
        if (reviewRequest.getRepository().getType() == ArcanumModels.VcsType.SVN) {
            return ArcBranch.trunk();
        }
        return null;
    }


    @Nullable
    private static ArcBranch getFromBranch(ArcanumModels.ReviewRequest reviewRequest) {
        if (reviewRequest.hasFromBranch() && !Strings.isNullOrEmpty(reviewRequest.getFromBranch().getName())) {
            return ArcBranch.ofBranchName(reviewRequest.getFromBranch().getName());
        }
        return null;
    }
}
