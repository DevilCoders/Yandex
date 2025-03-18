package ru.yandex.ci.engine.autocheck;

import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.autocheck.Autocheck;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.pr.RevisionNumberService;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StartAutocheckJob;
import ru.yandex.ci.engine.autocheck.model.AutocheckLaunchConfig;
import ru.yandex.ci.engine.autocheck.model.CheckLaunchParams;
import ru.yandex.ci.engine.autocheck.model.CheckRecheckLaunchParams;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.CheckOuterClass;

@RequiredArgsConstructor
@Slf4j
public class AutocheckService {

    @Nonnull
    AutocheckLaunchProcessor autocheckLaunchProcessor;

    @Nonnull
    AutocheckLaunchProcessorPostCommits autocheckLaunchProcessorPostCommits;

    @Nonnull
    StorageApiClient storageApiClient;

    @Nonnull
    private final CiMainDb db;

    @Nonnull
    private final AutocheckBlacklistService autocheckBlacklistService;

    @Nonnull
    private final RevisionNumberService revisionNumberService;

    public Autocheck.AutocheckLaunch createAutocheckLaunch(CheckLaunchParams launchParams) {
        if (launchParams.isPrecommitCheck()) {
            return autocheckLaunchProcessor.create(launchParams);
        }
        return autocheckLaunchProcessorPostCommits.create(launchParams);
    }

    public Autocheck.AutocheckRecheckLaunch createAutocheckLaunch(CheckRecheckLaunchParams launchParams) {
        return autocheckLaunchProcessor.createRecheck(launchParams);
    }

    public AutocheckLaunchConfig findBranchAutocheckLaunchParams(ArcBranch branch, CommitId leftRevision) {
        return autocheckLaunchProcessor.findBranchAutocheckLaunchParams(branch, leftRevision);
    }

    public AutocheckLaunchConfig findTrunkAutocheckLaunchParams(
            CommitId previousRevision,
            CommitId revision,
            @Nullable String checkAuthor
    ) {
        return autocheckLaunchProcessor.findTrunkAutocheckLaunchParams(previousRevision, revision, checkAuthor);
    }

    public void cancelAutocheckPrecommitFlow(String flowLaunchId, LaunchVcsInfo vcsInfo) {
        var leftRevision = StartAutocheckJob.getLeftRevisionForAutocheckPullRequests(vcsInfo).getCommitId();
        var rightRevision = StartAutocheckJob.getRightRevision(vcsInfo).getCommitId();

        log.info("flowId {}, left {}, right {}, arcanum check id {}",
                flowLaunchId, leftRevision, rightRevision, StartAutocheckJob.getCheckId(vcsInfo));

        var request = StorageApi.FindCheckByRevisionsRequest.newBuilder()
                .setLeftRevision(leftRevision)
                .setRightRevision(rightRevision)
                .addAllTags(List.of(flowLaunchId))
                .build();

        var checkIds = storageApiClient.findChecksByRevisionsAndTags(request)
                .getChecksList()
                .stream()
                .map(CheckOuterClass.Check::getId)
                .toList();

        if (checkIds.isEmpty()) {
            log.warn("No checks found");
            return;
        }

        if (checkIds.size() == 1) {
            log.info("Found single check: {}", checkIds.get(0));
        } else {
            log.warn("Found multiple check: {}", checkIds);
        }

        for (String id : checkIds) {
            log.info("Cancelling check {}", id);
            storageApiClient.cancelCheck(id);
        }
    }

    public OrderedArcRevision getLeftRevisionForAutocheckPostcommits(LaunchVcsInfo vcsInfo) {
        log.info("Searching left revision started");
        var rightRevision = vcsInfo.getRevision();
        var rightCommit = Objects.requireNonNull(vcsInfo.getCommit(), "Commit not present");

        var leftRevision = db.currentOrTx(() -> {
            var currentCommit = rightCommit;
            while (true) {
                var parentRevision = ArcCommitUtils.firstParentArcRevision(currentCommit)
                        .orElseThrow(() -> new IllegalArgumentException(
                                "commit %s should have at least one parent".formatted(rightRevision)
                        ));
                if (!autocheckBlacklistService.skipPostCommit(parentRevision)) {
                    return parentRevision;
                }
                currentCommit = db.arcCommit().get(parentRevision.getCommitId());
            }
        });

        log.info("Searching left revision finished. Left revision is {}", leftRevision);
        return revisionNumberService.getOrderedArcRevision(rightRevision.getBranch(), leftRevision);
    }

}
