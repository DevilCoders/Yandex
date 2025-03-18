package ru.yandex.ci.engine.timeline;

import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import org.assertj.core.api.ObjectAssert;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.ReleaseVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.launch.LaunchCancelTask;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doReturn;

class CommitRangeServiceTest extends EngineTestBase {

    private static final CiProcessId PROCESS_ID = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;

    @Autowired
    private BranchService branchService;

    @BeforeEach
    void setUp() {
        mockValidationSuccessful();
        doReturn(TestData.RELEASE_BRANCH_2.asString())
                .when(branchNameGenerator).generateName(any(), any(), anyInt());
        discoveryToR7();
        delegateToken(PROCESS_ID.getPath());
    }

    @AfterEach
    void tearDown() {
        ((ArcServiceStub) arcService).resetAndInitTestData();
    }

    @Nested
    class Create {
        /**
         * <pre>
         * o [1] launch
         * │        │
         * │        │
         * o        │
         * │        │
         * │        │
         * o        ▼
         * </pre>
         */
        @Test
        void launch() {
            assertThatLaunch(startReleaseAt(TestData.TRUNK_R5))
                    .hasVersion(1)
                    .hasCancelledExactly()
                    .hasRevision(TestData.TRUNK_R5)
                    .hasNullPreviousRevision()
                    .hasCommitCount(4);
        }

        /**
         * <pre>
         * o
         * │
         * │
         * o────── [1] branch
         * │            │
         * │            │
         * o            ▼
         * </pre>
         */
        @Test
        void branch() {
            assertThatBranch(createBranchAt(TestData.TRUNK_R5))
                    .hasVersion(1)
                    .hasCancelledExactly().hasCancelledInBaseExactly()
                    .hasBaseRevision(TestData.TRUNK_R5)
                    .hasTrunkCommitCount(4)
                    .hasNullPreviousRevision();
        }

        /**
         * <pre>
         * o [2] launch
         * │       │
         * │       ▼       │
         * o───────────────┘ [1] branch
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void launchAfterBranch() {
            createBranchAt(TestData.TRUNK_R5);

            assertThatLaunch(startReleaseAt(TestData.TRUNK_R7))
                    .hasVersion(2)
                    .hasTitle("Woodcutter #2")
                    .hasCancelledExactly()
                    .hasRevision(TestData.TRUNK_R7)
                    .hasPreviousRevision(TestData.TRUNK_R5)
                    .hasCommitCount(2);
        }

        /**
         * <pre>
         * │             │
         * o─────────────┘ [2] branch
         * │                     │
         * │                     │
         * o [1] launch        ◄─┘
         * │
         * │
         * o
         * </pre>
         **/
        @Test
        void branchAfterLaunch() {
            startReleaseAt(TestData.TRUNK_R5);

            assertThatBranch(createBranchAt(TestData.TRUNK_R7))
                    .hasVersion(2)
                    .hasBaseRevision(TestData.TRUNK_R7)
                    .hasTrunkCommitCount(2)
                    .hasPreviousRevision(TestData.TRUNK_R5);

        }

        /**
         * <pre>
         * o [2] launch
         * │       │
         * │       │
         * o       │
         * │       │
         * │       ▼
         * o [1] launch
         * </pre>
         */
        @Test
        void launchAfterLaunch() {
            startReleaseAt(TestData.TRUNK_R5);

            assertThatLaunch(startReleaseAt(TestData.TRUNK_R7))
                    .hasVersion(2)
                    .hasCancelledExactly()
                    .hasRevision(TestData.TRUNK_R7)
                    .hasPreviousRevision(TestData.TRUNK_R5)
                    .hasCommitCount(2);

        }

        /**
         * <pre>
         *           │
         * o─────────┘ [2] branch
         * │                 │
         * │                 │
         * o                 │
         * │                 │
         * │         │       ▼
         * o─────────┘ [1] branch
         * </pre>
         */
        @Test
        void branchAfterBranch() {
            Mockito.reset(branchNameGenerator); // сбрасываем предопределенные имена веток в setUp
            createBranchAt(TestData.TRUNK_R5);

            assertThatBranch(createBranchAt(TestData.TRUNK_R7))
                    .hasVersion(2)
                    .hasBaseRevision(TestData.TRUNK_R7)
                    .hasTrunkCommitCount(2)
                    .hasPreviousRevision(TestData.TRUNK_R5);
        }

        /**
         * <pre>
         * o
         * │
         * │          │
         * o──────────┘ [cancelled] [1] launch [2] branch
         * │                              │
         * │                              │
         * o [cancelled]                  ▼
         * </pre>
         */
        @Test
        void branchAtLaunch() {
            var cancelledInTrunk = startReleaseAt(TestData.TRUNK_R2);
            cancelLaunch(cancelledInTrunk);
            var cancelledInBase = startReleaseAt(TestData.TRUNK_R5);
            cancelLaunch(cancelledInBase);

            var launch = startReleaseAt(TestData.TRUNK_R5);

            assertThatBranch(createBranchAt(TestData.TRUNK_R5))
                    .hasVersion(3)
                    .hasNullPreviousRevision()
                    .hasTrunkCommitCount(4)
                    .hasActiveExactly(launch)
                    .hasCancelledExactly(cancelledInBase)
                    .hasCancelledInBaseExactly(cancelledInTrunk);
        }

    }

    @Nested
    class CreateInBranch {
        /**
         * <pre>
         * o
         * │
         * │      │
         * o──────┘ [1] branch   [2] launch
         * │                           │
         * │                           │
         * o                           ▼
         * </pre>
         */
        @Test
        void launchAtTrunkRevisionInBranch() {
            var branch = createBranchAt(TestData.TRUNK_R7);
            var launch = startReleaseAt(TestData.TRUNK_R7, branch.getArcBranch());

            assertThatLaunch(launch)
                    .hasVersion(1)
                    .hasRevision(TestData.TRUNK_R7)
                    .hasSelectedBranch(branch.getArcBranch())
                    .hasNullPreviousRevision()
                    .hasCommitCount(6);

            assertThatBranch(getBranch(branch))
                    .hasActiveExactly(launch);
        }

        /**
         * <pre>
         * o      o [3] launch
         * │      │       │
         * │      │       ▼
         * o──────┘ [1] branch   [2] launch
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void launchInBranchAfterTrunkLaunch() {
            var branch = createBranchAt(TestData.TRUNK_R6);
            startReleaseAt(TestData.TRUNK_R6, branch.getArcBranch());

            discoveryReleaseR5(branch.getArcBranch());
            assertThatLaunch(startReleaseAt(TestData.RELEASE_R6_3, branch.getArcBranch()))
                    .hasVersion(1, 1)
                    .hasRevision(TestData.RELEASE_R6_3)
                    .hasSelectedBranch(branch.getArcBranch())
                    .hasPreviousRevision(TestData.TRUNK_R6)
                    .hasCommitCount(3);
        }

        /**
         * <pre>
         * o      o             [2] launch
         * │      │                   │
         * │      │                   │
         * o──────┘ [1] branch        │
         * │                          │
         * │                          │
         * o                          ▼
         * </pre>
         */
        @Test
        void launchAtBranchRevision() {
            var branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            var launch = startReleaseAt(TestData.RELEASE_R6_3, branch.getArcBranch());
            assertThatLaunch(launch)
                    .hasVersion(1, 1)
                    .hasRevision(TestData.RELEASE_R6_3)
                    .hasSelectedBranch(branch.getArcBranch())
                    .hasNullPreviousRevision()
                    .hasCommitCount(5 + 3);

            assertThatBranch(getBranch(branch))
                    .hasActiveExactly(launch);
        }

        /**
         * <pre>
         * o      o             [3] launch
         * │      │                  │
         * │      │                  │
         * o──────┘ [2] branch       │
         * │                         │
         * │                         │
         * o [1] launch ◄────────────┘
         * </pre>
         */
        @Test
        void launchAtBranchRevisionAfterLaunchInTrunk() {
            startReleaseAt(TestData.TRUNK_R4);

            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            assertThatLaunch(startReleaseAt(TestData.RELEASE_R6_3, branch.getArcBranch()))
                    .hasVersion(2, 1)
                    .hasTitle("Woodcutter #2.1")
                    .hasRevision(TestData.RELEASE_R6_3)
                    .hasSelectedBranch(branch.getArcBranch())
                    .hasPreviousRevision(TestData.TRUNK_R4)
                    .hasCommitCount(2 + 3);
        }

        /**
         * <pre>
         *           o [3] launch
         *           │       │
         *           │       ▼
         * o         o [2] launch
         * │         │
         * │         │
         * o─────────┘ [1] branch
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void launchAtBranchRevisionAfterLaunchInSameBranch() {
            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());
            startReleaseAt(TestData.RELEASE_R6_2, branch.getArcBranch());

            assertThatLaunch(startReleaseAt(TestData.RELEASE_R6_4, branch.getArcBranch()))
                    .hasVersion(1, 2)
                    .hasRevision(TestData.RELEASE_R6_4)
                    .hasSelectedBranch(branch.getArcBranch())
                    .hasPreviousRevision(TestData.RELEASE_R6_2)
                    .hasCommitCount(2);
        }
    }

    @Nested
    class Cancel {
        /**
         * <pre>
         * o [2] launch*
         * │       │
         * │       │
         * o [1] launch [3] cancel
         * │       │
         * │       │
         * o       ▼
         * </pre>
         */
        @Test
        void first() {
            Launch first = startReleaseAt(TestData.TRUNK_R4);
            Launch second = startReleaseAt(TestData.TRUNK_R7);

            cancelLaunch(first);

            assertThatLaunch(getLaunch(second))
                    .hasCancelledExactly(first)
                    .hasRevision(TestData.TRUNK_R7)
                    .hasNullPreviousRevision()
                    .hasCommitCount(6);
        }

        /**
         * <pre>
         * o [5] launch*
         * │
         * │
         * o [2] launch [4] cancel
         * │
         * │
         * o [1] launch [3] cancel
         * </pre>
         */
        @Test
        void collectCancelled() {
            Launch first = startReleaseAt(TestData.TRUNK_R4);
            Launch second = startReleaseAt(TestData.TRUNK_R7);

            cancelLaunch(first);
            cancelLaunch(second);

            assertThatLaunch(startReleaseAt(TestData.TRUNK_R7))
                    .hasCancelledExactly(first, second);
        }

        /**
         * <pre>
         * o [3] launch*
         * │       │
         * │       │
         * o [2] launch [4] cancel
         * │       │
         * │       ▼
         * o [1] launch
         * </pre>
         */
        @Test
        void second() {
            startReleaseAt(TestData.TRUNK_R3);
            Launch second = startReleaseAt(TestData.TRUNK_R4);
            Launch third = startReleaseAt(TestData.TRUNK_R7);

            cancelLaunch(second);

            assertThatLaunch(getLaunch(third))
                    .hasCancelledExactly(second)
                    .hasRevision(TestData.TRUNK_R7)
                    .hasPreviousRevision(TestData.TRUNK_R3)
                    .hasCommitCount(4);
        }

        /**
         * <pre>
         * o [3] launch
         * │
         * │
         * │ [2] launch [4] cancel
         * o [1] launch
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void cancelSameRevisionFirst() {
            var first = startReleaseAt(TestData.TRUNK_R4);
            completeRelease(first);
            var second = startReleaseAt(TestData.TRUNK_R4);
            var third = startReleaseAt(TestData.TRUNK_R6);

            cancelLaunch(second);

            assertThatLaunch(getLaunch(third))
                    .hasCancelledExactly(second)
                    .hasPreviousRevision(TestData.TRUNK_R4);
        }

        /**
         * <pre>
         * o [3] launch*
         * │
         * │
         * o [2] launch [5] cancel
         * │
         * │
         * o [1] launch [4] cancel
         * </pre>
         */
        @Test
        void passCancelledForward() {
            Launch first = startReleaseAt(TestData.TRUNK_R4, TestData.TRUNK_R4.getBranch());
            Launch second = startReleaseAt(TestData.TRUNK_R5, TestData.TRUNK_R5.getBranch());
            Launch third = startReleaseAt(TestData.TRUNK_R6, TestData.TRUNK_R6.getBranch());

            cancelLaunch(first);
            cancelLaunch(second);

            assertThatLaunch(getLaunch(third))
                    .hasCancelledExactly(first, second)
                    .hasNullPreviousRevision()
                    .hasCommitCount(5);
        }

        /**
         * <pre>
         * o [3] launch*
         * │
         * │
         * o [2] launch [4] cancel
         * │
         * │
         * o [1] launch [5] cancel
         * </pre>
         */
        @Test
        void passCancelledBackward() {
            Launch first = startReleaseAt(TestData.TRUNK_R4, TestData.TRUNK_R4.getBranch());
            Launch second = startReleaseAt(TestData.TRUNK_R5, TestData.TRUNK_R5.getBranch());
            Launch third = startReleaseAt(TestData.TRUNK_R6, TestData.TRUNK_R6.getBranch());

            cancelLaunch(second);
            cancelLaunch(first);

            assertThatLaunch(getLaunch(third))
                    .hasCancelledExactly(first, second)
                    .hasNullPreviousRevision()
                    .hasCommitCount(5);
        }

        @Test
        void cancelReleaseWithoutNewCommits() {
            Launch first = startReleaseAt(TestData.TRUNK_R4, TestData.TRUNK_R4.getBranch());
            db.currentOrTx(() -> db.launches().save(
                    db.launches().get(first.getLaunchId())
                            .toBuilder()
                            .status(LaunchState.Status.SUCCESS)
                            .build()
            ));
            // start at the same revision release without new commits
            Launch second = startReleaseAt(TestData.TRUNK_R4, TestData.TRUNK_R4.getBranch());
            Launch third = startReleaseAt(TestData.TRUNK_R5, TestData.TRUNK_R5.getBranch());

            cancelLaunch(second);

            assertThatLaunch(getLaunch(third))
                    .hasCancelledExactly(second)
                    .hasPreviousRevision(TestData.TRUNK_R4)
                    .hasCommitCount(1);
        }
    }

    @Nested
    class CancelWithBranch {
        /**
         * <pre>
         *         o [3] launch [5] cancel
         *         │
         *         │
         * o       o [2] launch [4] cancel
         * │       │
         * │       │
         * o───────┘ [1] branch*
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void inBranchRevision() {
            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            Launch first = startReleaseAt(TestData.RELEASE_R6_3, branch.getArcBranch());
            Launch second = startReleaseAt(TestData.RELEASE_R6_4, branch.getArcBranch());

            cancelLaunch(first);

            assertThatLaunch(getLaunch(second))
                    .hasCancelledExactly(first);

            cancelLaunch(second);

            assertThatBranch(getBranch(branch))
                    .hasCancelledExactly(first, second)
                    .hasCancelledInBaseExactly();
        }

        /**
         * <pre>
         *         o [3] launch
         *         │
         *         │
         * o       o [2] launch [4] cancel
         * │       │
         * │       │
         * o───────┘ [1] launch*
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void firstBranchLaunch() {
            startReleaseAt(TestData.TRUNK_R6);
            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            Launch first = startReleaseAt(TestData.RELEASE_R6_1, branch.getArcBranch());
            startReleaseAt(TestData.RELEASE_R6_2, branch.getArcBranch());

            cancelLaunch(first);
        }

        /**
         * <pre>
         *         │
         * o       o [2] launch [3] cancel
         * │       │
         * │       │
         * o───────┘ [1] launch*
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void firstOnlyBranchLaunch() {
            startReleaseAt(TestData.TRUNK_R6);
            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            Launch first = startReleaseAt(TestData.RELEASE_R6_1, branch.getArcBranch());

            cancelLaunch(first);
        }

        /**
         * <pre>
         * o       o [4] launch
         * │       │
         * │       │
         * o───────┘ [2] launch [3] launch [5] cancel
         * │
         * │
         * o [1] launch
         * </pre>
         */
        @Test
        void launchWithoutCommitsAtBranch() {
            discoveryToR6();
            startReleaseAt(TestData.TRUNK_R4);
            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            Launch firstInBranch = startReleaseAt(TestData.TRUNK_R6, branch.getArcBranch());
            completeRelease(firstInBranch);
            Launch thirdRelease = startReleaseAt(TestData.TRUNK_R6, branch.getArcBranch());
            Launch topInBranch = startReleaseAt(TestData.RELEASE_R6_4, branch.getArcBranch());


            cancelLaunch(thirdRelease);


            assertThatLaunch(getLaunch(firstInBranch))
                    .hasPreviousRevision(TestData.TRUNK_R4)
                    // именно этот релиз содержит R5, на котором был запущен second
                    // поэтому отмененный прилипает именно нему
                    .hasCancelledExactly(thirdRelease);

            assertThatLaunch(getLaunch(topInBranch))
                    .hasPreviousRevision(TestData.TRUNK_R6)
                    .hasCancelledExactly(/* nothing */);

            assertThatBranch(getBranch(branch))
                    .hasPreviousRevision(TestData.TRUNK_R4)
                    .hasCancelledExactly(thirdRelease);
        }

        /**
         * <pre>
         * o       o [2] launch*
         * │       │          │
         * │       │          │
         * o───────┘          │
         * │                  │
         * │                  │
         * o [1] launch [3] cancel
         * │                  │
         * │                  │
         * o                  ▼
         * </pre>
         */
        @Test
        void inTrunkHistoryRevision() {
            Launch inTrunk = startReleaseAt(TestData.TRUNK_R4);

            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            Launch inBranch = startReleaseAt(TestData.RELEASE_R6_3, branch.getArcBranch());

            cancelLaunch(inTrunk);

            assertThatBranch(getBranch(branch))
                    .hasCancelledInBaseExactly(inTrunk)
                    .hasNullPreviousRevision()
                    .hasTrunkCommitCount(5);

            assertThatLaunch(getLaunch(inBranch))
                    .hasNullPreviousRevision()
                    .hasCommitCount(5 + 3);
        }

        /**
         * <pre>
         * o       o [4] launch
         * │       │       │
         * │       │       │
         * o───────┘ [2] branch [3] launch [5] cancel
         * │               │
         * │               │
         * o [1] launch ◄──┘
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void inBaseRevision() {
            startReleaseAt(TestData.TRUNK_R4);

            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            Launch inBase = startReleaseAt(TestData.TRUNK_R6, branch.getArcBranch());
            Launch inBranch = startReleaseAt(TestData.RELEASE_R6_3, branch.getArcBranch());

            cancelLaunch(inBase);

            assertThatBranch(getBranch(branch))
                    .hasCancelledExactly(inBase);

            assertThatLaunch(getLaunch(inBranch))
                    .hasPreviousRevision(TestData.TRUNK_R4)
                    .hasCancelledExactly(inBase)
                    .hasCommitCount(2 + 3);
        }

        /**
         * <pre>
         * o       o [4] launch
         * │       │       │
         * │       │       │
         * o───────┘ [2] launch [3] branch [5] cancel
         * │               │
         * │               │
         * o [1] launch ◄──┘
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void inBaseRevisionLaunchFirst() {
            startReleaseAt(TestData.TRUNK_R4);

            Launch inBase = startReleaseAt(TestData.TRUNK_R6);

            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            Launch inBranch = startReleaseAt(TestData.RELEASE_R6_3, branch.getArcBranch());

            cancelLaunch(inBase);

            assertThatBranch(getBranch(branch))
                    .hasCancelledExactly(inBase);

            assertThatLaunch(getLaunch(inBranch))
                    .hasPreviousRevision(TestData.TRUNK_R4)
                    .hasCancelledExactly(inBase)
                    .hasCommitCount(2 + 3);
        }

        /**
         * <pre>
         * o  [3] launch
         * │
         * │
         * o───────┘ [2] launch [4] branch [5] cancel
         * │
         * │
         * o [1] launch
         * │
         * │
         * o
         * </pre>
         */
        @Test
        void inBaseRevisionDoesntAffectFollowingLaunchInTrunk() {
            startReleaseAt(TestData.TRUNK_R4);

            Launch second = startReleaseAt(TestData.TRUNK_R6);
            Launch third = startReleaseAt(TestData.TRUNK_R7);

            Branch branch = createBranchAt(TestData.TRUNK_R6);

            cancelLaunch(second);

            assertThatBranch(getBranch(branch))
                    .hasCancelledExactly(second);

            Launch updatedThird = getLaunch(third);
            assertThat(updatedThird).isEqualTo(third);
            assertThatLaunch(updatedThird)
                    .hasPreviousRevision(TestData.TRUNK_R6);
        }
    }

    @Nested
    class CollectCancelled {
        /**
         * <pre>
         * o [cancelled] [1] launch
         * │
         * │
         * o [cancelled] [cancelled]
         * </pre>
         */
        @Test
        void toBeginning() {
            Launch first = startReleaseAt(TestData.TRUNK_R2);
            cancelLaunch(first);
            Launch second = startReleaseAt(TestData.TRUNK_R2);
            cancelLaunch(second);
            Launch third = startReleaseAt(TestData.TRUNK_R4);
            cancelLaunch(third);

            assertThatLaunch(startReleaseAt(TestData.TRUNK_R4))
                    .hasCancelledExactly(first, second, third);
        }

        /**
         * <pre>
         * o [2] launch
         * │
         * │
         * o [cancelled]
         * │
         * │
         * o [cancelled] [1] launch
         * </pre>
         */
        @Test
        void toLaunch() {
            Launch first = startReleaseAt(TestData.TRUNK_R2);
            cancelLaunch(first);
            startReleaseAt(TestData.TRUNK_R2);
            Launch third = startReleaseAt(TestData.TRUNK_R4);
            cancelLaunch(third);

            assertThatLaunch(startReleaseAt(TestData.TRUNK_R6))
                    .hasCancelledExactly(third);
        }

        /**
         * <pre>
         * o [2] launch
         * │
         * │
         * o [cancelled]
         * │
         * │       │
         * o───────┘ [cancelled] [1] branch
         * </pre>
         */
        @Test
        void toBranch() {
            Launch first = startReleaseAt(TestData.TRUNK_R2);
            cancelLaunch(first);
            createBranchAt(TestData.TRUNK_R2);

            Launch second = startReleaseAt(TestData.TRUNK_R4);
            cancelLaunch(second);

            assertThatLaunch(startReleaseAt(TestData.TRUNK_R6))
                    .hasCancelledExactly(second);
        }

        /**
         * <pre>
         *           o [2] launch
         *           │
         *           │
         * o         o [cancelled]
         * │         │
         * │         │
         * o─────────┘ [1] branch
         * │
         * │
         * o [cancelled]
         * </pre>
         */
        @Test
        void collectFromTrunk() {
            Launch inTrunk = startReleaseAt(TestData.TRUNK_R2);
            cancelLaunch(inTrunk);

            Branch branch = createBranchAt(TestData.TRUNK_R6);
            discoveryReleaseR5(branch.getArcBranch());

            Launch inBranch = startReleaseAt(TestData.RELEASE_R6_2);
            cancelLaunch(inBranch);

            assertThatLaunch(startReleaseAt(TestData.RELEASE_R6_3))
                    .hasCancelledExactly(inTrunk, inBranch);
        }
    }

    private void discoveryReleaseR5(ArcBranch branch) {
        List.of(
                TestData.RELEASE_R6_1,
                TestData.RELEASE_R6_2,
                TestData.RELEASE_R6_3,
                TestData.RELEASE_R6_4
        ).forEach(rev -> discoveryServicePostCommits.processPostCommit(branch, rev.toRevision(), false));
    }

    private Branch getBranch(Branch branch) {
        return db.currentOrTx(() ->
                branchService.getBranch(branch.getArcBranch(), branch.getProcessId()));
    }

    private Branch createBranchAt(OrderedArcRevision revision) {
        return db.currentOrTx(() ->
                branchService.createBranch(PROCESS_ID, revision, TestData.CI_USER));
    }

    private void cancelLaunch(Launch launchToCancel) {
        launchService.cancel(launchToCancel.getLaunchId(), TestData.CI_USER, "");
        executeBazingaTasks(LaunchCancelTask.class);
        Launch cancelled = getLaunch(launchToCancel);

        assertThat(cancelled).isNotNull();
        assertThat(cancelled.getStatus()).isEqualTo(LaunchState.Status.CANCELED);
    }

    private Launch getLaunch(Launch launch) {
        return db.currentOrTx(() -> db.launches().get(launch.getLaunchId()));
    }

    private Launch startReleaseAt(OrderedArcRevision revision, ArcBranch branch) {
        if (!revision.getBranch().isTrunk()) {
            delegateToken(PROCESS_ID.getPath(), branch);
        }
        return launchService.startRelease(PROCESS_ID, revision, branch, TestData.CI_USER, null, false,
                false, null, true, null, null, null);
    }

    private Launch startReleaseAt(OrderedArcRevision revision) {
        return launchService.startRelease(PROCESS_ID, revision, revision.getBranch(), TestData.CI_USER, null,
                false, false, null, true, null, null, null);
    }

    private void completeRelease(Launch launch) {
        db.currentOrTx(() -> {
            db.launches().save(launch.toBuilder().status(LaunchState.Status.SUCCESS).build());
        });
    }

    private static LaunchAssert assertThatLaunch(Launch launch) {
        return new LaunchAssert(launch);
    }

    private static BranchAssert assertThatBranch(Branch branch) {
        return new BranchAssert(branch);
    }

    private static class BranchAssert extends ObjectAssert<Branch> {

        private BranchAssert(Branch branch) {
            super(branch);
        }

        public BranchAssert hasBaseRevision(OrderedArcRevision expected) {
            OrderedArcRevision actualRev = actual.getInfo().getBaseRevision();
            if (!Objects.equals(actualRev, expected)) {
                throw failureWithActualExpected(actualRev, expected,
                        "Expected <%s> to have base revision <%s> but actual <%s>",
                        desc(actual), expected, actualRev
                );
            }
            return this;
        }

        public BranchAssert hasTrunkCommitCount(int expected) {
            int actualCount = actual.getVcsInfo().getTrunkCommitCount();
            if (actualCount != expected) {
                throw failureWithActualExpected(actualCount, expected,
                        "Expected <%s> to have trunk commit count <%s> but actual <%s>",
                        desc(actual), expected, actualCount
                );
            }
            return this;
        }

        public BranchAssert hasNullPreviousRevision() {
            return hasPreviousRevision(null);
        }

        public BranchAssert hasPreviousRevision(@Nullable OrderedArcRevision expected) {
            OrderedArcRevision actualRev = actual.getItem().getVcsInfo().getPreviousRevision();

            if (!Objects.equals(actualRev, expected)) {
                throw failureWithActualExpected(actualRev, expected,
                        "Expected <%s> to have previous revision <%s> but actual <%s>",
                        desc(actual), expected, actualRev
                );
            }

            return this;
        }

        public BranchAssert hasVersion(int major) {
            return hasVersion(Version.major(String.valueOf(major)));
        }

        public BranchAssert hasVersion(Version expected) {
            Version actualVersion = actual.getItem().getVersion();

            if (!Objects.equals(actualVersion, expected)) {
                throw failureWithActualExpected(actualVersion, expected,
                        "Expected <%s> to have version <%s> but actual <%s>",
                        desc(actual), expected, actualVersion
                );
            }

            return this;
        }

        public BranchAssert hasCancelledInBaseExactly(Launch... launches) {
            Set<Integer> actualCancelled = actual.getState().getCancelledBaseLaunches();
            Set<Integer> excepted = Stream.of(launches)
                    .map(Launch::getLaunchId)
                    .map(LaunchId::getNumber)
                    .collect(Collectors.toSet());

            assertThat(actualCancelled)
                    .as("Cancelled in base branch releases of %s", desc(actual))
                    .containsExactlyInAnyOrderElementsOf(excepted);

            return this;
        }

        public BranchAssert hasActiveExactly(Launch... launches) {
            Set<Integer> actualActive = actual.getState().getActiveLaunches();
            Set<Integer> excepted = Stream.of(launches)
                    .map(Launch::getLaunchId)
                    .map(LaunchId::getNumber)
                    .collect(Collectors.toSet());

            assertThat(actualActive)
                    .as("Active releases of %s", desc(actual))
                    .containsExactlyInAnyOrderElementsOf(excepted);

            return this;
        }

        public BranchAssert hasCancelledExactly(Launch... launches) {
            Set<Integer> actualCancelled = actual.getState().getCancelledBranchLaunches();
            Set<Integer> excepted = Stream.of(launches)
                    .map(Launch::getLaunchId)
                    .map(LaunchId::getNumber)
                    .collect(Collectors.toSet());

            assertThat(actualCancelled)
                    .as("Cancelled releases of %s", desc(actual))
                    .containsExactlyInAnyOrderElementsOf(excepted);

            return this;
        }

        private static String desc(Branch branch) {
            return "Branch #" + branch.getId().getBranch() + " at " + branch.getInfo().getBaseRevision().getCommitId();
        }
    }

    private static class LaunchAssert extends ObjectAssert<Launch> {

        private LaunchAssert(Launch launch) {
            super(launch);
        }

        public LaunchAssert hasCancelledExactly(Launch... launches) {
            Set<Integer> actualCancelled = actual.getCancelledReleases();
            Set<Integer> excepted = Stream.of(launches)
                    .map(Launch::getLaunchId)
                    .map(LaunchId::getNumber)
                    .collect(Collectors.toSet());

            assertThat(actualCancelled)
                    .as("Cancelled releases of %s", actual.getLaunchId())
                    .containsExactlyInAnyOrderElementsOf(excepted);

            return this;
        }

        public LaunchAssert hasRevision(OrderedArcRevision expected) {
            OrderedArcRevision actualRev = actual.getVcsInfo().getRevision();
            if (!Objects.equals(actualRev, expected)) {
                throw failureWithActualExpected(actualRev, expected,
                        "Expected <%s> to have revision <%s> but actual <%s>",
                        desc(actual), expected, actualRev
                );
            }
            return this;
        }

        public LaunchAssert hasSelectedBranch(ArcBranch expected) {
            ArcBranch actualBranch = actual.getVcsInfo().getSelectedBranch();
            if (!Objects.equals(actualBranch, expected)) {
                throw failureWithActualExpected(actualBranch, expected,
                        "Expected <%s> to have selected branch <%s> but actual <%s>",
                        desc(actual), expected, actualBranch
                );
            }
            return this;
        }

        public LaunchAssert hasCommitCount(int expected) {
            int actualCount = actual.getVcsInfo().getCommitCount();
            if (actualCount != expected) {
                throw failureWithActualExpected(actualCount, expected,
                        "Expected <%s> to have commit count <%s> but actual <%s>",
                        desc(actual), expected, actualCount
                );
            }
            return this;
        }

        public LaunchAssert hasVersion(int major) {
            return hasVersion(Version.major(String.valueOf(major)));
        }

        public LaunchAssert hasVersion(int major, int minor) {
            return hasVersion(Version.majorMinor(String.valueOf(major), String.valueOf(minor)));
        }

        private LaunchAssert hasVersion(Version expected) {
            Version actualVersion = actual.getVersion();
            if (!Objects.equals(expected, actualVersion)) {
                throw failureWithActualExpected(actualVersion, expected,
                        "Expected <%s> to have version <%s> but actual <%s>",
                        desc(actual), expected, actualVersion
                );
            }
            return this;
        }

        private LaunchAssert hasTitle(String expected) {
            var actualValue = actual.getTitle();
            if (!Objects.equals(expected, actualValue)) {
                throw failureWithActualExpected(actualValue, expected,
                        "Expected <%s> to have title <%s> but actual <%s>",
                        desc(actual), expected, actualValue
                );
            }
            return this;
        }

        public LaunchAssert hasNullPreviousRevision() {
            return hasPreviousRevision(null);
        }

        public LaunchAssert hasPreviousRevision(@Nullable OrderedArcRevision expected) {
            OrderedArcRevision actualRev = actual.getVcsInfo().getPreviousRevision();
            if (!Objects.equals(actualRev, expected)) {
                throw failureWithActualExpected(actualRev, expected,
                        "Expected <%s> to have previous revision <%s> but actual <%s>",
                        desc(actual), expected, actualRev
                );
            }


            ReleaseVcsInfo releaseVcsInfo = actual.getVcsInfo().getReleaseVcsInfo();
            if (releaseVcsInfo == null) {
                throw failure("Expected <%s> to have ReleaseVcsInfo but it is null", actual);
            }

            OrderedArcRevision actualReleaseRev = releaseVcsInfo.getPreviousRevision();
            if (!Objects.equals(actualReleaseRev, expected)) {
                throw failureWithActualExpected(actualReleaseRev, expected,
                        "Expected <%s> to have previous revision in ReleaseVcsInfo <%s> but actual <%s>",
                        desc(actual), expected, actualReleaseRev
                );
            }


            return this;
        }

        private static String desc(Launch launch) {
            return "Launch #" + launch.getLaunchId().getNumber() +
                    " at " + launch.getVcsInfo().getRevision().getCommitId();
        }
    }
}
