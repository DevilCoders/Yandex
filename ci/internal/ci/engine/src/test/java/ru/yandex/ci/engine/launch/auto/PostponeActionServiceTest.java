package ru.yandex.ci.engine.launch.auto;

import java.nio.file.Path;
import java.util.List;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;

import static org.assertj.core.api.Assertions.assertThat;

class PostponeActionServiceTest extends EngineTestBase {

    public static final ArcCommit COMMIT0 = commit("r0", TestData.TRUNK_COMMIT_1);
    public static final ArcCommit COMMIT1 = commit("r1", COMMIT0);
    public static final ArcCommit COMMIT2 = commit("r2", COMMIT1);
    public static final ArcCommit COMMIT3 = commit("r3", COMMIT2);
    public static final ArcCommit COMMIT4 = commit("r4", COMMIT3);

    public static final Path PATH = AYamlService.dirToConfigPath("test");
    public static final CiProcessId ACTION = CiProcessId.ofFlow(PATH, "test");

    private static final int DEFAULT_DELAY = 60; // 1 minute by default

    @Autowired
    protected PostponeActionService postponeActionService;

    @BeforeEach
    void beforeEach() {
        mockValidationSuccessful();
        mockYavAny();

        arcServiceStub.reset();
        arcServiceStub.addFirstCommit();
        arcServiceStub.addCommitAuto(COMMIT0);
        arcServiceStub.addCommitAuto(COMMIT1);
        arcServiceStub.addCommitAuto(COMMIT2);
        arcServiceStub.addCommitAuto(COMMIT3);
        arcServiceStub.addCommitAuto(COMMIT4);
    }

    @AfterEach
    void afterEach() {
        arcServiceStub.resetAndInitTestData();
    }

    @Test
    void testEmptyList() {
        postponeActionService.executePostponeActions(DEFAULT_DELAY);
        checkStatuses();
    }

    @Test
    void testPostponeSingleLaunch() {
        discovery(COMMIT0);
        delegateToken(PATH);

        checkStatuses();

        discovery(COMMIT1);
        checkStatuses(Status.POSTPONE);

        postponeActionService.executePostponeActions(DEFAULT_DELAY);
        checkStatuses(Status.STARTING);


        // Nothing really changes
        postponeActionService.executePostponeActions(DEFAULT_DELAY);
        checkStatuses(Status.STARTING);
    }

    @Test
    void testPostpone3LaunchesDefaultSettings() {
        discovery(COMMIT0);
        delegateToken(PATH);

        discovery(COMMIT1);
        discovery(COMMIT2);
        discovery(COMMIT3);

        checkStatuses(Status.POSTPONE, Status.POSTPONE, Status.POSTPONE);

        postponeActionService.executePostponeActions(0);

        var launches = checkStatuses(Status.STARTING, Status.STARTING, Status.POSTPONE);

        //
        db.currentOrTx(() -> db.launches().save(launches.get(0).withStatus(Status.FAILURE)));

        postponeActionService.executePostponeActions(0);
        checkStatuses(Status.FAILURE, Status.STARTING, Status.STARTING);

        //
        db.currentOrTx(() -> db.launches().save(launches.get(1).withStatus(Status.FAILURE)));

        postponeActionService.executePostponeActions(0);
        checkStatuses(Status.FAILURE, Status.FAILURE, Status.STARTING);

    }

    @Test
    void testPostpone3LaunchesLimited() {
        discovery(COMMIT0);
        delegateToken(PATH);

        discovery(COMMIT1);
        discovery(COMMIT2);
        discovery(COMMIT3);

        checkStatuses(Status.POSTPONE, Status.POSTPONE, Status.POSTPONE);

        postponeActionService.executePostponeActions(DEFAULT_DELAY);

        var launches = checkStatuses(Status.STARTING, Status.POSTPONE, Status.POSTPONE);

        //
        db.currentOrTx(() -> db.launches().save(launches.get(0).withStatus(Status.FAILURE)));

        postponeActionService.executePostponeActions(DEFAULT_DELAY);
        checkStatuses(Status.FAILURE, Status.STARTING, Status.POSTPONE);

        //
        db.currentOrTx(() -> db.launches().save(launches.get(1).withStatus(Status.FAILURE)));

        postponeActionService.executePostponeActions(DEFAULT_DELAY);
        checkStatuses(Status.FAILURE, Status.FAILURE, Status.STARTING);

        //

        postponeActionService.executePostponeActions(DEFAULT_DELAY);
        checkStatuses(Status.FAILURE, Status.FAILURE, Status.STARTING);

    }

    @Test
    void testPostpone3LaunchesThenResetMaxActiveCount() {
        discovery(COMMIT0);
        delegateToken(PATH);

        discovery(COMMIT1);
        discovery(COMMIT2);
        discovery(COMMIT3);

        checkStatuses(Status.POSTPONE, Status.POSTPONE, Status.POSTPONE);

        postponeActionService.executePostponeActions(DEFAULT_DELAY);

        checkStatuses(Status.STARTING, Status.POSTPONE, Status.POSTPONE);

        // Reset max-active to 0 - means launch everything in postpone at once
        discovery(COMMIT4);

        checkStatuses(Status.STARTING, Status.POSTPONE, Status.POSTPONE, Status.STARTING);

        postponeActionService.executePostponeActions(DEFAULT_DELAY);

        checkStatuses(Status.STARTING, Status.STARTING, Status.STARTING, Status.STARTING);

        // Nothing really changes
        postponeActionService.executePostponeActions(DEFAULT_DELAY);
        checkStatuses(Status.STARTING, Status.STARTING, Status.STARTING, Status.STARTING);

    }

    private List<Launch> checkStatuses(Status... statuses) {
        var launches = db.currentOrReadOnly(() -> db.launches().findAll());
        assertThat(launches.stream()
                .map(Launch::getStatus))
                .isEqualTo(List.of(statuses));
        return launches;
    }

    private static ArcCommit commit(String path, ArcCommit parent) {
        return TestData.toCommit(
                OrderedArcRevision.fromHash(
                        "PostponeActionServiceTest/" + path,
                        ArcBranch.trunk(),
                        parent.getSvnRevision() + 1,
                        parent.getPullRequestId() + 1
                ),
                TestData.CI_USER)
                .withParent(parent);
    }
}
