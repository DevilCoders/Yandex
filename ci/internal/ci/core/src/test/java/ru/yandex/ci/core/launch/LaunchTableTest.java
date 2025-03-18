package ru.yandex.ci.core.launch;

import java.nio.file.Path;
import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.time.LocalDate;
import java.time.Month;
import java.time.ZoneOffset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.OrderByDirection;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.core.launch.LaunchState.Status.CANCELED;
import static ru.yandex.ci.core.launch.LaunchState.Status.CLEANING;
import static ru.yandex.ci.core.launch.LaunchState.Status.DELAYED;
import static ru.yandex.ci.core.launch.LaunchState.Status.FAILURE;
import static ru.yandex.ci.core.launch.LaunchState.Status.POSTPONE;
import static ru.yandex.ci.core.launch.LaunchState.Status.RUNNING;
import static ru.yandex.ci.core.launch.LaunchState.Status.STARTING;
import static ru.yandex.ci.core.launch.LaunchState.Status.SUCCESS;
import static ru.yandex.ci.core.launch.LaunchState.Status.WAITING_FOR_MANUAL_TRIGGER;

class LaunchTableTest extends CommonYdbTestBase {

    private static final Clock FIXED_CLOCK =
            Clock.fixed(LocalDate.of(2007, Month.AUGUST, 3).atStartOfDay().toInstant(ZoneOffset.UTC), ZoneOffset.UTC);

    @Test
    void getActiveLaunches() {
        FlowFullId flowId = new FlowFullId("/my/path", "hotfix");
        CiProcessId processId = CiProcessId.ofRelease(flowId.getPath(), "getActiveLaunches");

        db.currentOrTx(() -> db.launches().save(getAllLaunches(processId)));

        db.currentOrReadOnly(() -> {
            List<Launch> activeLaunches = db.launches().getActiveLaunches(processId);
            assertThat(LaunchState.Status.terminalStatuses()).isNotEmpty();
            assertThat(activeLaunches)
                    .extracting(Launch::getStatus)
                    .isNotEmpty()
                    .doesNotContainAnyElementsOf(LaunchState.Status.terminalStatuses())
                    .hasSize(LaunchState.Status.values().length - LaunchState.Status.terminalStatuses().size());

            assertThat(activeLaunches)
                    .extracting(Launch::getLaunchId)
                    .extracting(LaunchId::getProcessId)
                    .containsOnly(processId);

            assertThat(activeLaunches)
                    .extracting(Launch::getLaunchId)
                    .isSortedAccordingTo(Comparator.comparing(LaunchId::getNumber).reversed());
        });
    }

    @Test
    void getProcessReleases() {
        FlowFullId flowId = new FlowFullId("/my/path", "hotfix");
        CiProcessId processId = CiProcessId.ofRelease(flowId.getPath(), "listCancelled");

        List<Launch> launches = List.of(
                createRunningLaunch(LaunchId.of(processId, 5)),
                createRunningLaunch(LaunchId.of(processId, 4)),
                createRunningLaunch(LaunchId.of(processId, 3)),
                createRunningLaunch(LaunchId.of(processId, 2)),
                createRunningLaunch(LaunchId.of(processId, 1))
        );

        db.currentOrTx(() -> db.launches().save(launches));

        db.currentOrReadOnly(() ->
                assertThat(
                        db.launches().getProcessLaunches(List.of(
                                LaunchId.of(processId, 3),
                                LaunchId.of(processId, 1),
                                LaunchId.of(processId, 5))
                        )
                ).extracting(l -> l.getLaunchId().getNumber())
                        .isEqualTo(List.of(1, 3, 5))
        );
    }


    private static Launch createRunningLaunch(LaunchId launchId) {
        return createLaunch(launchId, RUNNING);
    }

    private static Launch createLaunch(LaunchId launchId, LaunchState.Status status) {
        return createLaunch(launchId, status, TestData.TRUNK_R4);
    }

    private static Launch createLaunch(CiProcessId processId, int number, LaunchState.Status status) {
        return createLaunch(LaunchId.of(processId, number), status, TestData.TRUNK_R4);
    }

    private static Launch createLaunchWithRetries(CiProcessId processId, int number, int retries) {
        return createLaunch(LaunchId.of(processId, number), RUNNING, TestData.TRUNK_R4)
                .toBuilder()
                .statistics(LaunchStatistics.builder()
                        .retries(retries)
                        .build())
                .build();
    }

    private static Launch createLaunch(
            LaunchId launchId,
            LaunchState.Status status,
            OrderedArcRevision configCommitId
    ) {
        return createLaunch(launchId, status, configCommitId, OrderedArcRevision.fromHash("b2", "trunk", 42, 43));
    }

    private static Launch createLaunch(
            LaunchId launchId,
            LaunchState.Status status,
            OrderedArcRevision configCommitId,
            OrderedArcRevision launchRevision
    ) {
        var created = FIXED_CLOCK.instant().plus(Duration.ofMinutes(launchId.getNumber()));

        return Launch.builder()
                .launchId(launchId)
                .title("Мега релиз #42")
                .project("ci")
                .triggeredBy("andreevdm")
                .created(created)
                .type(Launch.Type.USER)
                .notifyPullRequest(false)
                .flowInfo(
                        LaunchFlowInfo.builder()
                                .configRevision(configCommitId)
                                .flowId(FlowFullId.of(launchId.getProcessId().getPath(), "hotfix"))
                                .stageGroupId("my-stages")
                                .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                                .build()
                )
                .userData(new LaunchUserData(List.of(), false))
                .vcsInfo(launchVcsInfo(launchRevision))
                .status(status)
                .activity(Activity.fromStatus(status))
                .activityChanged(created)
                .statusText("")
                .version(Version.major("1"))
                .build();
    }

    private static LaunchVcsInfo launchVcsInfo(OrderedArcRevision revision) {
        return LaunchVcsInfo.builder()
                .revision(revision)
                .previousRevision(OrderedArcRevision.fromHash("b1", "trunk", 21, 22))
                .pullRequestInfo(
                        new LaunchPullRequestInfo(
                                1L,
                                1L,
                                TestData.CI_USER,
                                null,
                                null,
                                ArcanumMergeRequirementId.of("system", "type"),
                                new PullRequestVcsInfo(
                                        TestData.REVISION,
                                        TestData.SECOND_REVISION,
                                        ArcBranch.trunk(),
                                        TestData.THIRD_REVISION,
                                        TestData.USER_BRANCH
                                ),
                                List.of("CI-1"),
                                List.of("label1"),
                                null
                        )
                )
                .releaseVcsInfo(
                        new ReleaseVcsInfo(TestData.TRUNK_R1, TestData.TRUNK_R2)
                )
                .commitCount(7)
                .build();
    }

    @Test
    void getLaunches() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "getReleases");

        Launch launch1 = createRunningLaunch(LaunchId.of(processId, 1));
        Launch launch2 = createRunningLaunch(LaunchId.of(processId, 2));
        Launch launch3 = createRunningLaunch(LaunchId.of(processId, 3));

        db.currentOrTx(() -> db.launches().save(List.of(launch1, launch2, launch3)));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getLaunches(processId, -1, 42))
                    .isEqualTo(List.of(launch3, launch2, launch1));

            assertThat(db.launches().getLaunches(processId, 0, 42))
                    .isEqualTo(List.of(launch3, launch2, launch1));

            assertThat(db.launches().getLaunches(processId, 0, 2))
                    .isEqualTo(List.of(launch3, launch2));

            assertThat(db.launches().getLaunches(processId, 3, 42))
                    .isEqualTo(List.of(launch2, launch1));

            assertThat(db.launches().getLaunches(processId, 3, 1))
                    .isEqualTo(List.of(launch2));

            assertThat(db.launches().getLaunches(processId, 2, 42))
                    .isEqualTo(List.of(launch1));

            assertThat(db.launches().getLaunches(processId, 2, 1))
                    .isEqualTo(List.of(launch1));

            assertThat(db.launches().getLaunches(processId, 1, 42))
                    .isEmpty();
        });
    }

    @Test
    void getLaunchesWithoutCanceled() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "getLaunchesWithoutCanceled");

        db.currentOrTx(() ->
                db.launches().save(List.of(
                        createLaunch(LaunchId.of(processId, 8), STARTING),
                        createLaunch(LaunchId.of(processId, 7), CANCELED),
                        createLaunch(LaunchId.of(processId, 6), CANCELED),
                        createLaunch(LaunchId.of(processId, 5), CANCELED),
                        createLaunch(LaunchId.of(processId, 4), FAILURE),
                        createLaunch(LaunchId.of(processId, 3), WAITING_FOR_MANUAL_TRIGGER),
                        createLaunch(LaunchId.of(processId, 2), CANCELED),
                        createLaunch(LaunchId.of(processId, 1), SUCCESS)
                )));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getLaunches(processId, -1, 90, true))
                    .extracting(l -> l.getLaunchId().getNumber())
                    .isEqualTo(List.of(8, 4, 3, 1));

            assertThat(db.launches().getLaunches(processId, -1, 2, true))
                    .extracting(l -> l.getLaunchId().getNumber())
                    .isEqualTo(List.of(8, 4));

            assertThat(db.launches().getLaunches(processId, 8, 3, true))
                    .extracting(l -> l.getLaunchId().getNumber())
                    .isEqualTo(List.of(4, 3, 1));
        });
    }

    @Test
    void getLaunchesWithCanceled() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "getLaunchesWithoutCanceled");

        var launches = List.of(
                createLaunch(LaunchId.of(processId, 8), STARTING),
                createLaunch(LaunchId.of(processId, 7), CANCELED),
                createLaunch(LaunchId.of(processId, 6), CANCELED),
                createLaunch(LaunchId.of(processId, 5), CANCELED),
                createLaunch(LaunchId.of(processId, 4), FAILURE),
                createLaunch(LaunchId.of(processId, 3), WAITING_FOR_MANUAL_TRIGGER),
                createLaunch(LaunchId.of(processId, 2), CANCELED),
                createLaunch(LaunchId.of(processId, 1), SUCCESS)
        );

        db.currentOrTx(() -> db.launches().save(launches));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getLaunches(processId, -1, 90, false))
                    .extracting(l -> l.getLaunchId().getNumber())
                    .isEqualTo(List.of(8, 7, 6, 5, 4, 3, 2, 1));

            assertThat(db.launches().getLaunches(processId, -1, 2, false))
                    .extracting(l -> l.getLaunchId().getNumber())
                    .isEqualTo(List.of(8, 7));

            assertThat(db.launches().getLaunches(processId, 8, 3, false))
                    .extracting(l -> l.getLaunchId().getNumber())
                    .isEqualTo(List.of(7, 6, 5));
        });
    }

    @Test
    void getDelayedLaunches() {
        CiProcessId dProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "getReleases");
        CiProcessId eProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "getReleases");

        db.currentOrTx(() ->
                db.launches().save(List.of(
                        createLaunch(LaunchId.of(dProcess, 1), RUNNING, TestData.TRUNK_R2),
                        createLaunch(LaunchId.of(dProcess, 2), DELAYED, TestData.TRUNK_R3),
                        createLaunch(LaunchId.of(dProcess, 3), DELAYED, TestData.TRUNK_R3),
                        createLaunch(LaunchId.of(dProcess, 31), POSTPONE, TestData.TRUNK_R3),
                        createLaunch(LaunchId.of(eProcess, 4), DELAYED, TestData.TRUNK_R3),
                        createLaunch(LaunchId.of(dProcess, 5), DELAYED, TestData.TRUNK_R4)
                )));

        db.currentOrReadOnly(() ->
                assertThat(db.launches().getDelayedLaunchIds(TestData.CONFIG_PATH_ABD, TestData.TRUNK_R3))
                        .extracting(Launch.Id::getLaunchNumber)
                        .isEqualTo(List.of(2, 3))
        );
    }

    @Test
    void getLastFinishedLaunch() {
        CiProcessId dProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "getReleases");
        CiProcessId eProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "getReleases");
        CiProcessId fProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABF, "getReleases");
        db.currentOrTx(() ->
                db.launches().save(List.of(
                        createLaunch(LaunchId.of(dProcess, 9), RUNNING, TestData.TRUNK_R10),
                        createLaunch(LaunchId.of(dProcess, 8), SUCCESS, TestData.TRUNK_R9),
                        createLaunch(LaunchId.of(dProcess, 7), RUNNING, TestData.TRUNK_R8),
                        createLaunch(LaunchId.of(eProcess, 6), RUNNING, TestData.TRUNK_R7),
                        createLaunch(LaunchId.of(eProcess, 5), SUCCESS, TestData.TRUNK_R6),
                        createLaunch(LaunchId.of(eProcess, 4), RUNNING, TestData.TRUNK_R5)
                )));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getLastFinishedLaunch(dProcess))
                    .matches(l -> l.orElseThrow().getLaunchId().getNumber() == 8);
            assertThat(db.launches().getLastFinishedLaunch(eProcess))
                    .matches(l -> l.orElseThrow().getLaunchId().getNumber() == 5);
            assertThat(db.launches().getLastFinishedLaunch(fProcess))
                    .isEmpty();
        });
    }

    @Test
    void lastNotCanceledRelease() {
        CiProcessId processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "lastNotCanceledRelease");

        Launch old = createLaunch(LaunchId.of(processId, 1), FAILURE);
        Launch actual = createLaunch(LaunchId.of(processId, 2), FAILURE);
        Launch canceled = createLaunch(LaunchId.of(processId, 3), CANCELED);

        db.currentOrTx(() -> db.launches().save(List.of(old, actual, canceled)));

        db.currentOrReadOnly(() ->
                assertThat(db.launches().getLastNotCancelledLaunch(processId))
                        .isPresent()
                        .contains(actual)
        );
    }

    @SuppressWarnings("MethodLength")
    @Test
    void getLaunchesWithFilter() {
        CiProcessId dProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "getReleases");
        CiProcessId eProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "getReleases");

        Launch dLaunch1 = createLaunch(LaunchId.of(dProcess, 1), RUNNING, TestData.TRUNK_R2);
        Launch eLaunch2 = createLaunch(LaunchId.of(eProcess, 2), RUNNING, TestData.TRUNK_R3);
        Launch eLaunch3 = createLaunch(LaunchId.of(eProcess, 3), SUCCESS, TestData.TRUNK_R4);
        Launch eLaunch4 = createLaunch(LaunchId.of(eProcess, 4), SUCCESS, TestData.TRUNK_R5)
                .toBuilder()
                .tags(List.of("tag1", "tag2"))
                .build();
        Launch eLaunch5 = createLaunch(LaunchId.of(eProcess, 5), SUCCESS, TestData.TRUNK_R6)
                .toBuilder()
                .tags(List.of("tag1", "tag3"))
                .pinned(true)
                .build();
        Launch eLaunch6 = createLaunch(LaunchId.of(eProcess, 6), CANCELED, TestData.TRUNK_R7)
                .toBuilder()
                .pinned(true)
                .build();
        Launch eLaunch7 = createLaunch(LaunchId.of(eProcess, 7), SUCCESS, TestData.TRUNK_R8)
                .toBuilder()
                .vcsInfo(launchVcsInfo(
                        OrderedArcRevision.fromRevision(
                                ArcRevision.of("merge_rev"),
                                ArcBranch.ofPullRequest(1L),
                                100L,
                                0)
                ))
                .build();

        db.currentOrTx(() ->
                db.launches().save(List.of(dLaunch1, eLaunch2, eLaunch3, eLaunch4, eLaunch5, eLaunch6, eLaunch7)));

        db.currentOrReadOnly(() -> {
            // ### tests with empty launch filter
            assertThat(db.launches().getLaunches(
                    dProcess, LaunchTableFilter.empty(), -1, -1
            )).isEqualTo(List.of(dLaunch1));

            assertThat(db.launches().getLaunches(
                    eProcess, LaunchTableFilter.empty(), -3, -1
            )).isEqualTo(List.of(eLaunch7, eLaunch6, eLaunch5, eLaunch4, eLaunch3, eLaunch2));

            assertThat(db.launches().getLaunches(
                    eProcess, LaunchTableFilter.empty(), eLaunch4.getId().getLaunchNumber(), 1
            )).isEqualTo(List.of(eLaunch3));

            assertThat(db.launches().getLaunches(
                    eProcess, LaunchTableFilter.empty(), 0, 2
            )).isEqualTo(List.of(eLaunch7, eLaunch6));

            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .sortDirection(OrderByDirection.ASC)
                            .build(),
                    0, 2
            )).isEqualTo(List.of(eLaunch2, eLaunch3));

            // ### tests with only one filled filter field
            // pinned = true
            assertThat(db.launches().getLaunches(
                    eProcess, LaunchTableFilter.builder().pinned(true).build(), -1, -1
            )).isEqualTo(List.of(eLaunch6, eLaunch5));

            // tag = tag2
            assertThat(db.launches().getLaunches(
                    eProcess, LaunchTableFilter.builder().tags(List.of("tag2")).build(), -1, -1
            )).isEqualTo(List.of(eLaunch4));

            // tag = tag1, tag2
            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .tags(List.of("tag2", "tag3"))
                            .build(),
                    -1, -1
            )).isEqualTo(List.of(eLaunch5, eLaunch4));

            // branch = ArcBranch.ofPr(1L)
            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .branch(ArcBranch.ofPullRequest(1L).asString())
                            .build(),
                    -1, -1
            )).isEqualTo(List.of(eLaunch7));

            // status = RUNNING
            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .statuses(List.of(RUNNING))
                            .build(),
                    -1, -1
            )).isEqualTo(List.of(eLaunch2));

            // status = RUNNING, CANCELLED
            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .statuses(List.of(RUNNING, CANCELED))
                            .build(),
                    -1, -1
            )).isEqualTo(List.of(eLaunch6, eLaunch2));

            // ### complex filters
            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .pinned(true)
                            .tags(List.of("tag1"))
                            .build(),
                    -1, -1
            )).isEqualTo(List.of(eLaunch5));

            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .branch(ArcBranch.trunk().asString())
                            .statuses(List.of(RUNNING, CANCELED))
                            .build(),
                    -1, 3
            )).isEqualTo(List.of(eLaunch6, eLaunch2));

            // Default sort
            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .branch(ArcBranch.trunk().asString())
                            .build(),
                    0, 2
            )).isEqualTo(List.of(eLaunch6, eLaunch5)); // eLaunch7 not in trunk

            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .branch(ArcBranch.trunk().asString())
                            .build(),
                    eLaunch5.getId().getLaunchNumber(), 2
            )).isEqualTo(List.of(eLaunch4, eLaunch3)); // eLaunch7 not in trunk

            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .branch(ArcBranch.trunk().asString())
                            .sortDirection(OrderByDirection.ASC)
                            .build(),
                    0, 2
            )).isEqualTo(List.of(eLaunch2, eLaunch3));

            assertThat(db.launches().getLaunches(
                    eProcess,
                    LaunchTableFilter.builder()
                            .branch(ArcBranch.trunk().asString())
                            .sortDirection(OrderByDirection.ASC)
                            .build(),
                    eLaunch3.getId().getLaunchNumber(), 2
            )).isEqualTo(List.of(eLaunch4, eLaunch5));
        });
    }

    @Test
    void getTagsStartsWith() {
        CiProcessId dProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "getReleases");
        CiProcessId eProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "getReleases");

        db.currentOrTx(() ->
                db.launches().save(List.of(
                        createLaunch(LaunchId.of(dProcess, 1), RUNNING, TestData.TRUNK_R2).toBuilder()
                                .tags(List.of("tag10")).build(),
                        createLaunch(LaunchId.of(eProcess, 2), SUCCESS, TestData.TRUNK_R3).toBuilder()
                                .tags(List.of("tag11", "tag12")).build(),
                        createLaunch(LaunchId.of(eProcess, 3), SUCCESS, TestData.TRUNK_R4).toBuilder()
                                .tags(List.of("tag120")).build()
                )));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getTagsStartsWith(eProcess, "", -1, -1))
                    .isEqualTo(List.of("tag11", "tag12", "tag120"));
            assertThat(db.launches().getTagsStartsWith(eProcess, "tag12", -1, -1))
                    .isEqualTo(List.of("tag12", "tag120"));
            assertThat(db.launches().getTagsStartsWith(eProcess, "not-found", -1, -1))
                    .isEmpty();
        });
    }

    @Test
    void getBranchesBySubstring() {
        CiProcessId dProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "getReleases");
        CiProcessId eProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "getReleases");

        db.currentOrTx(() ->
                db.launches().save(List.of(
                        createLaunch(LaunchId.of(dProcess, 1), RUNNING, TestData.TRUNK_R2, TestData.TRUNK_R2),
                        createLaunch(
                                LaunchId.of(eProcess, 2), SUCCESS, TestData.TRUNK_R3,
                                OrderedArcRevision.fromHash(
                                        TestData.DS1_REVISION.getCommitId(), "pr:42", 801, 42
                                )
                        ),
                        createLaunch(LaunchId.of(eProcess, 3), SUCCESS, TestData.TRUNK_R4, TestData.RELEASE_R2_1)
                )));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getBranchesBySubstring(dProcess, "", -1, -1))
                    .isEqualTo(List.of("trunk"));
            assertThat(db.launches().getBranchesBySubstring(eProcess, "", -1, -1))
                    .isEqualTo(List.of("pr:42", "releases/ci/release-component-1"));
            assertThat(db.launches().getBranchesBySubstring(eProcess, "pr", -1, -1))
                    .isEqualTo(List.of("pr:42"));
            assertThat(db.launches().getBranchesBySubstring(eProcess, "r", -1, -1))
                    .isEqualTo(List.of("pr:42", "releases/ci/release-component-1"));
            assertThat(db.launches().getBranchesBySubstring(eProcess, "not-found", -1, -1))
                    .isEmpty();

            // test offset/limit
            assertThat(db.launches().getBranchesBySubstring(eProcess, "", 0, 1))
                    .isEqualTo(List.of("pr:42"));
            assertThat(db.launches().getBranchesBySubstring(eProcess, "", 1, 1))
                    .isEqualTo(List.of("releases/ci/release-component-1"));
        });
    }

    @Test
    void getBranchByName() {
        CiProcessId dProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "getReleases");

        db.currentOrTx(() ->
                db.launches().save(
                        createLaunch(LaunchId.of(dProcess, 1), RUNNING, TestData.TRUNK_R2, TestData.TRUNK_R2)
                ));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getBranchByName(dProcess, "").orElse(null))
                    .isNull();
            assertThat(db.launches().getBranchByName(dProcess, "trunk").orElse(null))
                    .isEqualTo("trunk");
        });
    }


    @Test
    void getBranchesStartsWith() {
        CiProcessId dProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "getReleases");
        CiProcessId eProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "getReleases");

        db.currentOrTx(() ->
                db.launches().save(List.of(
                        createLaunch(LaunchId.of(dProcess, 1), RUNNING, TestData.TRUNK_R2, TestData.TRUNK_R2),
                        createLaunch(
                                LaunchId.of(eProcess, 2), SUCCESS, TestData.TRUNK_R3,
                                OrderedArcRevision.fromHash(
                                        TestData.DS1_REVISION.getCommitId(), "pr:42", 801, 42
                                )
                        ),
                        createLaunch(LaunchId.of(eProcess, 3), SUCCESS, TestData.TRUNK_R4, TestData.RELEASE_R2_1)
                )));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getBranchesStartsWith(dProcess, "", -1, -1))
                    .isEqualTo(List.of("trunk"));
            assertThat(db.launches().getBranchesStartsWith(eProcess, "", -1, -1))
                    .isEqualTo(List.of("pr:42", "releases/ci/release-component-1"));
            assertThat(db.launches().getBranchesStartsWith(eProcess, "pr", -1, -1))
                    .isEqualTo(List.of("pr:42"));
            assertThat(db.launches().getBranchesStartsWith(eProcess, "r", -1, -1))
                    .isEqualTo(List.of("releases/ci/release-component-1"));
            assertThat(db.launches().getBranchesStartsWith(eProcess, "not-found", -1, -1))
                    .isEmpty();

            // test offset/limit
            assertThat(db.launches().getBranchesStartsWith(eProcess, "", 0, 1))
                    .isEqualTo(List.of("pr:42"));
            assertThat(db.launches().getBranchesStartsWith(eProcess, "", 1, 1))
                    .isEqualTo(List.of("releases/ci/release-component-1"));
        });
    }

    @Test
    void getLaunchesByVcsInfo() {
        CiProcessId dProcess = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "getReleases");
        var launch = createLaunch(LaunchId.of(dProcess, 1), RUNNING,
                TestData.TRUNK_R2, TestData.TRUNK_R2);

        db.currentOrTx(() -> db.launches().save(launch));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getPullRequestLaunches(1, 1))
                    .isEqualTo(List.of(launch.getId()));

            assertThat(db.launches().getPullRequestLaunches(1, 2))
                    .isEmpty();

            assertThat(db.launches().getPullRequestLaunches(2, 1))
                    .isEmpty();
        });
    }

    @Test
    void launchStats() {
        CiProcessId white = CiProcessId.ofRelease(Path.of("white/a.yaml"), "my-release");
        CiProcessId green = CiProcessId.ofRelease(Path.of("green/a.yaml"), "my-release");

        db.currentOrTx(() ->
                db.launches().save(List.of(
                        createLaunch(white, 1, RUNNING),
                        createLaunch(green, 2, FAILURE),
                        createLaunch(white, 3, RUNNING),
                        createLaunch(white, 4, WAITING_FOR_MANUAL_TRIGGER),
                        createLaunch(green, 5, RUNNING),
                        createLaunch(white, 6, SUCCESS),
                        createLaunch(white, 7, SUCCESS),
                        createLaunch(green, 8, RUNNING)
                )));


        assertThat(getStats(FIXED_CLOCK.instant()))
                .isEqualTo(Set.of(
                        stat(green, Activity.ACTIVE, 2, 0),
                        stat(white, Activity.ACTIVE, 3, 0),
                        stat(green, Activity.FAILED, 1, 0),
                        stat(white, Activity.FINISHED, 2, 0)
                ));

        assertThat(getStats(FIXED_CLOCK.instant().plus(Duration.ofMinutes(4))))
                .isEqualTo(Set.of(
                        stat(green, Activity.ACTIVE, 2, 0),
                        stat(white, Activity.FINISHED, 2, 0)
                ));
    }

    @Test
    void launchStatsCountRetries() {
        CiProcessId white = CiProcessId.ofRelease(Path.of("white/a.yaml"), "my-release");
        CiProcessId green = CiProcessId.ofRelease(Path.of("green/a.yaml"), "my-release");

        db.currentOrTx(() ->
                db.launches().save(List.of(
                        createLaunchWithRetries(white, 1, 0),
                        createLaunchWithRetries(white, 2, 7),
                        createLaunchWithRetries(white, 3, 0),
                        createLaunchWithRetries(green, 4, 0),
                        createLaunchWithRetries(white, 5, 0),
                        createLaunchWithRetries(green, 6, 14),
                        createLaunchWithRetries(green, 7, 0),
                        createLaunchWithRetries(white, 8, 999)
                )));

        assertThat(getStats(FIXED_CLOCK.instant()))
                .isEqualTo(Set.of(
                        stat(green, Activity.ACTIVE, 3, 1),
                        stat(white, Activity.ACTIVE, 5, 2)
                ));
    }

    @Test
    void getActiveReleaseLaunchesCount() {
        FlowFullId flowId = new FlowFullId("/my/path", "hotfix");
        CiProcessId processId = CiProcessId.ofRelease(flowId.getPath(), "getActiveLaunches");

        db.currentOrTx(() -> db.launches().save(getAllLaunches(processId)));

        db.currentOrReadOnly(() -> {
            var activeLaunchesCount = db.launches().getActiveReleaseLaunchesCount("ci");

            var terminalStatuses = LaunchState.Status.terminalStatuses();
            assertThat(terminalStatuses).isNotEmpty();
            assertThat(activeLaunchesCount)
                    .extracting(LaunchTable.CountByProcessIdAndStatus::getStatus)
                    .isNotEmpty()
                    .doesNotContainAnyElementsOf(terminalStatuses)
                    .hasSize(LaunchState.Status.values().length - terminalStatuses.size());

            assertThat(Set.copyOf(activeLaunchesCount)).isEqualTo(
                    Arrays.stream(LaunchState.Status.values())
                            .filter(it -> !it.isTerminal())
                            .map(status -> new LaunchTable.CountByProcessIdAndStatus(
                                    processId.asString(), status, "trunk", 1
                            ))
                            .collect(Collectors.toSet())
            );
        });
    }

    @Test
    void getActiveReleaseLaunchesCountForProcessId() {
        FlowFullId flowId = new FlowFullId("/my/path", "hotfix");
        CiProcessId processId = CiProcessId.ofRelease(flowId.getPath(), "getActiveLaunches");

        db.currentOrTx(() -> db.launches().save(getAllLaunches(processId)));

        db.currentOrReadOnly(() -> {
            var activeLaunchesCount = db.launches().getActiveReleaseLaunchesCount("ci", processId);

            var terminalStatuses = LaunchState.Status.terminalStatuses();
            assertThat(terminalStatuses).isNotEmpty();
            assertThat(activeLaunchesCount)
                    .extracting(LaunchTable.CountByProcessIdAndStatus::getStatus)
                    .isNotEmpty()
                    .doesNotContainAnyElementsOf(terminalStatuses)
                    .hasSize(LaunchState.Status.values().length - terminalStatuses.size());

            assertThat(Set.copyOf(activeLaunchesCount)).isEqualTo(
                    Arrays.stream(LaunchState.Status.values())
                            .filter(it -> !it.isTerminal())
                            .map(status -> new LaunchTable.CountByProcessIdAndStatus(
                                    processId.asString(), status, "trunk", 1
                            ))
                            .collect(Collectors.toSet())
            );
        });
    }

    @Test
    void getLaunchesWithStatus() {
        FlowFullId flowId = new FlowFullId("/my/path", "hotfix");
        CiProcessId processId = CiProcessId.ofRelease(flowId.getPath(), "getLaunchesWithStatus");

        var launches = List.of(
                createLaunch(LaunchId.of(processId, 1), STARTING),
                createLaunch(LaunchId.of(processId, 2), CANCELED),
                createLaunch(LaunchId.of(processId, 3), CANCELED),
                createLaunch(LaunchId.of(processId, 4), POSTPONE),
                createLaunch(LaunchId.of(processId, 5), FAILURE),
                createLaunch(LaunchId.of(processId, 6), POSTPONE)
        );

        db.currentOrTx(() -> db.launches().save(launches));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getLaunchIds(processId, ArcBranch.trunk(), POSTPONE, 1))
                    .extracting(Launch.Id::getLaunchNumber)
                    .isEqualTo(List.of(4));

            assertThat(db.launches().getLaunchIds(processId, ArcBranch.trunk(), POSTPONE, 3))
                    .extracting(Launch.Id::getLaunchNumber)
                    .isEqualTo(List.of(4, 6));

            assertThat(db.launches().getLaunchIds(processId, ArcBranch.trunk(), POSTPONE, 0)) // All
                    .extracting(Launch.Id::getLaunchNumber)
                    .isEqualTo(List.of(4, 6));

            assertThat(db.launches().getLaunchIds(processId, ArcBranch.trunk(), CLEANING, 1))
                    .extracting(Launch.Id::getLaunchNumber)
                    .isEmpty();

            assertThat(db.launches().getLaunchIds(processId, ArcBranch.ofPullRequest(123), POSTPONE, 1))
                    .isEmpty();

            assertThat(db.launches().getLaunchIds(processId, ArcBranch.ofString("release/test"), POSTPONE, 1))
                    .isEmpty();
        });
    }

    @Test
    void getLaunchesCount() {
        FlowFullId flowId = new FlowFullId("/my/path", "hotfix");
        CiProcessId processId = CiProcessId.ofRelease(flowId.getPath(), "getLaunchesCountExcept");

        db.currentOrTx(() -> db.launches().save(getAllLaunches(processId)));

        db.currentOrReadOnly(() -> {
            assertThat(db.launches().getLaunchesCountExcept(processId, ArcBranch.trunk(), Set.of(
                    LaunchState.Status.CANCELED,
                    LaunchState.Status.SUCCESS,
                    LaunchState.Status.FAILURE))
            ).isEqualTo(LaunchState.Status.values().length - 3); // 12

            assertThat(db.launches().getLaunchesCountExcept(processId, ArcBranch.ofPullRequest(123), Set.of(
                    LaunchState.Status.CANCELED,
                    LaunchState.Status.SUCCESS,
                    LaunchState.Status.FAILURE))
            ).isEqualTo(0);

            assertThat(db.launches().getLaunchesCountExcept(processId, ArcBranch.ofString("release/test"), Set.of(
                    LaunchState.Status.CANCELED,
                    LaunchState.Status.SUCCESS,
                    LaunchState.Status.FAILURE))
            ).isEqualTo(0);
        });
    }

    private Set<ProcessStat> getStats(Instant instant) {
        return db.currentOrReadOnly(() -> {
            var stats = new HashSet<ProcessStat>();
            db.launches().findStats(instant, stats::add);
            return stats;
        });
    }

    private List<Launch> getAllLaunches(CiProcessId processId) {
        int number = 1;
        List<Launch> launches = new ArrayList<>();
        for (LaunchState.Status state : LaunchState.Status.values()) {
            launches.add(createLaunch(LaunchId.of(processId, number++), state));
        }
        return launches;
    }

    private static ProcessStat stat(CiProcessId processId, Activity activity, long count, long withRetries) {
        return new ProcessStat(processId.asString(), "ci", activity, count, withRetries);
    }
}
