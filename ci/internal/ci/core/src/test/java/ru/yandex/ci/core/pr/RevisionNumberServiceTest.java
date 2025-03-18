package ru.yandex.ci.core.pr;

import java.util.Optional;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommitTable;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceCanonImpl;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.TestCiDbUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

class RevisionNumberServiceTest {

    private RevisionNumberService revisionNumberService;

    private static final ArcBranch RELEASE = ArcBranch.ofBranchName("releases/ci/first-release-branch-CI-914");
    private RevisionNumberTable revisionNumberTable;

    /*
     * $ arc log --oneline trunk..releases/ci/first-release-branch-CI-914
     * af6dc46e07ac79c364bf1219181d4642d3ed892b (arcadia/releases/ci/first-release-branch-CI-914) CI-914 third change
     * 09759a29821fbf8c771ecaae4db80f22f208cf79 CI-914 one more change
     * 864e0957a702767fe6e5b210c9807d31ab69d20a CI-914 more changes to release branch
     * 9b388eb3b850d4b6c8900d633db0c47e9ba80280 CI-914 trigger
     */

    @BeforeEach
    public void setUp() {
        ArcService arcService = new ArcServiceCanonImpl(false);

        CiMainDb ciMainDb = mock(CiMainDb.class);
        TestCiDbUtils.mockToCallRealTxMethods(ciMainDb);

        revisionNumberTable = mock(RevisionNumberTable.class);
        when(ciMainDb.revisionNumbers()).thenReturn(revisionNumberTable);
        when(ciMainDb.arcCommit()).thenReturn(mock(ArcCommitTable.class));

        revisionNumberService = new RevisionNumberService(arcService, ciMainDb);
    }

    @Test
    void processAllBranch() {
        ArcRevision releaseHead = ArcRevision.of("09759a29821fbf8c771ecaae4db80f22f208cf79");
        OrderedArcRevision ordered = revisionNumberService.getOrderedArcRevision(RELEASE, releaseHead);
        assertThat(ordered.getBranch()).isEqualTo(RELEASE);
        assertThat(ordered.getCommitId()).isEqualTo(releaseHead.getCommitId());
        assertThat(ordered.getNumber()).isEqualTo(3);

        ArgumentCaptor<RevisionNumber> saveCapture = ArgumentCaptor.forClass(RevisionNumber.class);
        verify(revisionNumberTable, times(3)).save(saveCapture.capture());
        assertThat(saveCapture.getAllValues())
                .containsExactlyInAnyOrder(
                        number("9b388eb3b850d4b6c8900d633db0c47e9ba80280", 1, 1374008),
                        number("864e0957a702767fe6e5b210c9807d31ab69d20a", 2, 1380088),
                        number("09759a29821fbf8c771ecaae4db80f22f208cf79", 3, 1380094)
                );
    }

    @Test
    void processBranchHead() {
        ArcRevision releaseHead = ArcRevision.of("09759a29821fbf8c771ecaae4db80f22f208cf79");
        when(revisionNumberTable.findLastKnown(RELEASE))
                .thenReturn(Optional.of(number("864e0957a702767fe6e5b210c9807d31ab69d20a", 2, 0)));

        OrderedArcRevision ordered = revisionNumberService.getOrderedArcRevision(RELEASE, releaseHead);
        assertThat(ordered.getBranch()).isEqualTo(RELEASE);
        assertThat(ordered.getCommitId()).isEqualTo(releaseHead.getCommitId());
        assertThat(ordered.getNumber()).isEqualTo(3);

        verify(revisionNumberTable).save(number("09759a29821fbf8c771ecaae4db80f22f208cf79", 3, 1380094));
    }

    @Test
    void processKnown() {
        ArcRevision releaseHead = ArcRevision.of("864e0957a702767fe6e5b210c9807d31ab69d20a");
        when(revisionNumberTable.findById(RELEASE, releaseHead))
                .thenReturn(Optional.of(number("864e0957a702767fe6e5b210c9807d31ab69d20a", 3, 0)));

        OrderedArcRevision ordered = revisionNumberService.getOrderedArcRevision(RELEASE, releaseHead);
        assertThat(ordered.getBranch()).isEqualTo(RELEASE);
        assertThat(ordered.getCommitId()).isEqualTo(releaseHead.getCommitId());
        assertThat(ordered.getNumber()).isEqualTo(3);

        verify(revisionNumberTable, never()).save(any(RevisionNumber.class));
    }

    @Test
    void inTrunkButBranchIsKnown() {
        ArcRevision trunkRevision = ArcRevision.of("8f0be43c8ca39539f34a5638970ebe4b8459d33f");
        when(revisionNumberTable.findLastKnown(RELEASE))
                .thenReturn(Optional.of(number("864e0957a702767fe6e5b210c9807d31ab69d20a", 2, 0)));

        OrderedArcRevision ordered = revisionNumberService.getOrderedArcRevision(RELEASE, trunkRevision);
        assertThat(ordered.getBranch()).isEqualTo(ArcBranch.trunk());
        assertThat(ordered.getCommitId()).isEqualTo(trunkRevision.getCommitId());
        assertThat(ordered.getNumber()).isEqualTo(7551228);

        verify(revisionNumberTable, never()).save(any(RevisionNumber.class));
    }

    @Test
    void firstCommitInBranchIsTrunk() {
        ArcRevision releaseHead = ArcRevision.of("ee90c73435f7f1703b720222a9a2ec26ac7cdab9");

        OrderedArcRevision ordered = revisionNumberService.getOrderedArcRevision(RELEASE, releaseHead);

        assertThat(ordered.getBranch()).isEqualTo(ArcBranch.trunk());
        assertThat(ordered.getCommitId()).isEqualTo(releaseHead.getCommitId());
        assertThat(ordered.getNumber()).isEqualTo(7173378);
    }

    @Test
    void commitIdNotInBranch() {
        ArcRevision trunkRevision = ArcRevision.of("8f0be43c8ca39539f34a5638970ebe4b8459d33f");

        OrderedArcRevision ordered = revisionNumberService.getOrderedArcRevision(RELEASE, trunkRevision);

        assertThat(ordered.getBranch()).isEqualTo(ArcBranch.trunk());
        assertThat(ordered.getCommitId()).isEqualTo(trunkRevision.getCommitId());
        assertThat(ordered.getNumber()).isEqualTo(7551228);
    }

    private static RevisionNumber number(String commitId, int number, long pullRequestId) {
        return new RevisionNumber(RevisionNumber.Id.of(RELEASE.asString(), commitId), number, pullRequestId);
    }
}
