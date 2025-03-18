package ru.yandex.ci.engine.autocheck.testenv;

import java.util.Optional;
import java.util.concurrent.Callable;
import java.util.concurrent.ThreadLocalRandom;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.arcanum.ArcanumReviewDataDto;
import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.testenv.model.CheckFastModeDto;
import ru.yandex.ci.client.testenv.model.StartCheckV2RequestDto;
import ru.yandex.ci.client.testenv.model.StartCheckV2ResponseDto;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.poller.Poller;
import ru.yandex.ci.engine.autocheck.AffectedDirectoriesHelper;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.util.ObjectStore;

@Slf4j
@RequiredArgsConstructor
public class TestenvStartService {
    private final TestenvClient testenvClient;
    private final ArcanumClientImpl arcanumClient;
    private final ArcService arcService;
    private final AutocheckBlacklistService autocheckBlacklistService;

    public ArcanumReviewDataDto getReviewRequestData(long reviewRequestId) throws TimeoutException,
            InterruptedException {
        return Poller.poll(
                        () -> arcanumClient.getReviewRequestData(
                                reviewRequestId,
                                "id,author(name),commit_description," +
                                        "diff_sets(" +
                                        /* - */ "id,gsid,zipatch(url,svn_base_revision,svn_branch)," +
                                        /* - */ "arc_branch_heads(from_id,to_id,merge_id),patch_url" +
                                        ")," +
                                        "ci_settings(fast_circuit,check_fast_mode)"
                        ))
                .canStopWhen(Optional::isPresent)
                .interval(30, TimeUnit.SECONDS)
                .timeout(2, TimeUnit.HOURS)
                .retryOnExceptionCount(240)
                .run()
                .orElseThrow(() -> new RuntimeException("can't fetch review request %s from Arcanum"
                        .formatted(reviewRequestId)
                ));
    }

    public ArcanumReviewDataDto.DiffSet getDiffSet(
            long reviewRequestId, long diffSetId, ArcanumReviewDataDto reviewRequestData
    ) {
        return reviewRequestData.getDiffSets().stream()
                .filter(it -> it.getId() == diffSetId)
                .findFirst()
                .orElseThrow(() -> new RuntimeException("review request %s has no diff set %s: %s"
                        .formatted(reviewRequestId, diffSetId, reviewRequestData)
                ));
    }

    public StartCheckV2ResponseDto startTestenvCheck(
            long reviewRequestId,
            long diffSetId,
            ArcanumReviewDataDto reviewRequestData,
            ArcanumReviewDataDto.DiffSet diffSet,
            String checkId
    ) throws TimeoutException, InterruptedException {
        var zipatch = diffSet.getZipatch();

        var author = reviewRequestData.getAuthor().getName();
        var updatedGsid = GsidUtils.gsidWithAuthorAndReviewRequestId(reviewRequestId, author, diffSet.getGsid());

        // Original source code of building StartCheckV2RequestDto here: https://nda.ya.ru/t/qVWGBanf4Gc5f3
        var diffPaths = AffectedDirectoriesHelper.collectAndSort(
                ArcRevision.of(diffSet.getArcBranchHeads().getMergeRevision()),
                arcService
        );

        var requestBuilder = StartCheckV2RequestDto.builder()
                .diffSetId(diffSetId)
                .revision(zipatch.getSvnBaseRevision())
                .patch(zipatch.getUrlWithPrefix())
                .patchUrl(diffSet.getPatchUrl())
                .owner(author)
                .gsid(updatedGsid)
                .branch(SvnBranches.branchWithoutPrefixOrNull(zipatch.getSvnBranch()))
                .commitMessage(reviewRequestData.getCommitDescription())
                .diffPaths(diffPaths)
                .recheck(false)
                .forceRecheckId(null)
                .arcCommit(diffSet.getArcBranchHeads().getMergeRevision())
                .arcPrevCommit(diffSet.getArcBranchHeads().getUpstreamRevision())
                .parallelRunFastAndFullCircuit(false)
                .storageCheckId(checkId);


        requestBuilder.checkFastMode(CheckFastModeDto.DISABLED);


        var startCheckViaTestenvRequest = requestBuilder.build();

        log.info("Starting pre-commit check: {}", startCheckViaTestenvRequest);

        var blacklistUpdated = ObjectStore.of(false);
        var testenvResponse = pollerBuilder(
                () -> {
                    log.info("Sending request to testenv: reviewRequest {}, diffSet {}", reviewRequestId, diffSetId);
                    return testenvClient.createPrecommitCheckV2(startCheckViaTestenvRequest);
                })
                .canStopWhen(response -> {
                    if (response.getCheckId() != null || response.getFastCircuitCheckId() != null) {
                        return true;
                    }

                    // No check means that either pr affected only blacklist paths or error occurred in Testenv
                    log.warn("No check started by testenv for reviewRequest {}, diffSet {}: {}",
                            reviewRequestId, diffSetId, response);

                    if (!blacklistUpdated.get()) {
                        // we need to update blacklist to refresh "cache"
                        autocheckBlacklistService.updateBlacklistDirectories();
                        blacklistUpdated.set(true);
                    }

                    if (autocheckBlacklistService.isOnlyBlacklistPathsAffected(diffPaths)) {
                        log.info("Only black list paths affected");
                        return true;
                    }

                    if (response.getMessage() != null
                            && response.getMessage().contains("arcadia tier 1 precommit checks not allowed")) {
                        log.warn("TE assumes that check is in blacklist, we don't");
                        return true;
                    }

                    return false;
                }).build().run();

        log.info(
                "started pre-commit check: reviewRequest {}, diffSet {}, check {}",
                reviewRequestId, diffSetId, testenvResponse
        );

        return testenvResponse;
    }

    private static <T> Poller.PollerBuilder<T> pollerBuilder(Callable<T> action) {
        return Poller.poll(action)
                // 270 seconds == 4.5 minutes
                .interval(retryCount -> 270L + ThreadLocalRandom.current().nextInt(0, 60), TimeUnit.SECONDS)
                .timeout(15, TimeUnit.MINUTES)
                .retryOnExceptionCount(15);
    }
}
