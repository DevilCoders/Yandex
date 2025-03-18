package ru.yandex.ci.core.pr;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.db.CiMainDb;

@Slf4j
@RequiredArgsConstructor
public class RevisionNumberService {
    private static final int MAX_BRANCH_COMMITS_EXPECTED = 1000;

    private final ArcService arcService;
    private final CiMainDb db;

    /**
     * Возвращает нумерованный коммит в зависимости от того, находится ли он в trunk или в ветке.
     *
     * @param commit коммит для нумерации
     * @param branch ветка, в которой будет произведена нумерация, если коммит не из транка.
     * @return нумерованный коммит. Возвращается номер trunk во всех случаях, если коммит в транке. При этом branch
     * в результате также будет другой.
     */
    public OrderedArcRevision getOrderedArcRevision(ArcBranch branch, ArcCommit commit) {
        if (commit.isTrunk()) {
            return commit.toOrderedTrunkArcRevision();
        }
        return getOrderedArcRevisionOutsideTrunk(branch, commit);
    }

    public OrderedArcRevision getOrderedArcRevision(ArcBranch branch, CommitId commitId) {
        if (commitId instanceof OrderedArcRevision) {
            return (OrderedArcRevision) commitId;
        }
        ArcCommit arcCommit = arcService.getCommit(commitId);
        if (arcCommit.isTrunk()) {
            return arcCommit.toOrderedTrunkArcRevision();
        }
        return getOrderedArcRevisionOutsideTrunk(branch, commitId);
    }

    private OrderedArcRevision getOrderedArcRevisionOutsideTrunk(ArcBranch branch, CommitId commit) {
        log.info("#getOrderedArcRevisionOutsideTrunk, branch = {}, commit = {}", branch, commit);
        return db.currentOrTx(() -> {
            Optional<RevisionNumber> commitNumber = db.revisionNumbers().findById(branch, commit);
            if (commitNumber.isPresent()) {
                var revision = commitNumber.get().toOrderedArcRevision();
                log.info("Revision number from table = {}", revision);
                return revision;
            }

            Optional<RevisionNumber> lastKnown = db.revisionNumbers().findLastKnown(branch);
            lastKnown.ifPresent(revisionNumber ->
                    log.info("Last known revision number is {}", revisionNumber));
            return lastKnown
                    .map(revisionNumber -> calculateBranchToLastKnown(branch, commit, revisionNumber))
                    .orElseGet(() -> calculateBranchToTrunk(branch, commit));
        });
    }

    private OrderedArcRevision calculateBranchToTrunk(ArcBranch branch, CommitId commit) {
        log.info("#calculateBranchToTrunk, branch = {}, commit = {}", branch, commit);

        var commits = arcService.getBranchCommits(commit, null);
        log.info("Fetched {} commits", commits.size());

        Preconditions.checkState(commits.size() > 0, "Not found branch commits for %s from commit %s", branch, commit);
        Preconditions.checkState(
                commits.get(0).getCommitId().equals(commit.getCommitId()),
                "Branch first commit %s doesn't equal supplied",
                commits.get(0),
                commit
        );

        saveCommits(commits);

        ArcCommit earliest = commits.get(commits.size() - 1);
        if (commits.size() == 1) {
            return commits.get(0).toOrderedTrunkArcRevision();
        }
        Preconditions.checkState(earliest.isTrunk(),
                "The earliest commit %s of branch %s should be in trunk", earliest, branch
        );
        Collections.reverse(commits);
        return enumerateAndGetLast(branch, commits.subList(1, commits.size()), 1);
    }

    private OrderedArcRevision calculateBranchToLastKnown(
            ArcBranch branch,
            CommitId laterCommit,
            RevisionNumber lastKnown
    ) {
        ArcRevision earlierRevision = lastKnown.getId().getArcRevision();
        log.info("#calculateBranchToLastKnown, branch = {}, laterCommit = {}, lastKnown = {}",
                branch, laterCommit, lastKnown);

        var commits = arcService.getCommits(laterCommit, earlierRevision, MAX_BRANCH_COMMITS_EXPECTED + 1);
        log.info("Fetched {} commits", commits.size());

        Preconditions.checkState(
                commits.size() <= MAX_BRANCH_COMMITS_EXPECTED,
                "Found more then expected commits between %s..%s",
                earlierRevision,
                laterCommit
        );

        Preconditions.checkState(
                commits.size() > 0,
                "Found zero commits between %s..%s",
                earlierRevision,
                laterCommit
        );

        saveCommits(commits);

        Collections.reverse(commits);

        int lastKnownNumber = Math.toIntExact(lastKnown.getNumber());
        return enumerateAndGetLast(branch, commits, lastKnownNumber + 1);
    }

    private void saveCommits(Collection<ArcCommit> commits) {
        db.currentOrTx(() -> db.arcCommit().save(commits));
    }

    private OrderedArcRevision enumerateAndGetLast(ArcBranch branch, List<ArcCommit> commits, int lastNumber) {
        log.info("#enumerateAndGetLast, branch = {}, commit size = {}, lastNumber = {}",
                branch, commits.size(), lastNumber);
        List<RevisionNumber> numbers = new ArrayList<>(commits.size());
        for (int i = 0; i < commits.size(); i++) {
            var commit = commits.get(i);
            var id = RevisionNumber.Id.of(branch.asString(), commit.getCommitId());
            int number = i + lastNumber;
            RevisionNumber revisionNumber = new RevisionNumber(id, number, commit.getPullRequestId());
            numbers.add(revisionNumber);
            db.revisionNumbers().save(revisionNumber);
        }
        return numbers.get(numbers.size() - 1).toOrderedArcRevision();
    }
}
