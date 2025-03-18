package ru.yandex.ci.engine.proto;

import java.io.IOException;
import java.nio.file.Path;
import java.time.Instant;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.protobuf.TextFormat;
import io.github.benas.randombeans.api.EnhancedRandom;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.EnumSource;
import org.junit.jupiter.params.provider.ValueSource;

import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.proto.Common.StatusChangeWrapper;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchUserData;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.core.timeline.BranchState;
import ru.yandex.ci.core.timeline.BranchVcsInfo;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.state.model.StoredStage;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.test.random.TestRandomUtils;
import ru.yandex.ci.util.CiJson;

import static org.assertj.core.api.Assertions.assertThat;

class ProtoMappersTest {
    private static final long SEED = -1828975290L;

    private EnhancedRandom random;

    @BeforeEach
    void setUp() {
        random = TestRandomUtils.enhancedRandom(SEED);
    }

    @Test
    void taskState() {
        List<TaskBadge> original = random.objects(TaskBadge.class, 10)
                .collect(Collectors.toList());

        List<FrontendFlowApi.TaskState> states = original
                .stream()
                .map(ProtoMappers::toProtoTaskState)
                .collect(Collectors.toList());

        assertThat(states)
                .isNotEmpty()
                .doesNotContainNull()
                .extracting(FrontendFlowApi.TaskState::getStatus)
                .containsAll(valuesOfWithExclusions(
                        FrontendFlowApi.TaskState.Status.class,
                        FrontendFlowApi.TaskState.Status.UNRECOGNIZED,
                        FrontendFlowApi.TaskState.Status.UNKNOWN_STATUS
                ));

        assertThat(states)
                .zipSatisfy(original, (proto, model) ->
                        assertThat(proto.getText()).isEqualTo(model.getText()));
    }

    @Test
    void flowLaunch() {

        List<FrontendFlowApi.LaunchState> launches = random.objects(FlowLaunchEntity.class, 10)
                .map(flowLaunchSrc -> {

                    // flowLaunch.project contains anything but actual process description
                    // i.e. CiProcessId.ofString would fail
                    var flowLaunch = flowLaunchSrc.withProcessId(
                            (random.nextBoolean() ?
                                    CiProcessId.ofFlow(Path.of("ci/a.yaml"), "test") :
                                    CiProcessId.ofRelease(Path.of("ci/a.yaml"), "test")).asString());

                    var blockedStageIdToLaunch = Map.of(
                            flowLaunch.getStages()
                                    .stream()
                                    .map(StoredStage::getId)
                                    .findFirst()
                                    .orElse("some-stage-id"),
                            new ProtoMappers.LaunchAndVersion(
                                    LaunchId.of(
                                            CiProcessId.ofRelease(
                                                    flowLaunch.getLaunchId().getProcessId().getPath(),
                                                    flowLaunch.getLaunchId().getProcessId().getSubId()
                                            ),
                                            1
                                    ),
                                    Version.fromAsString("1")));
                    Common.StagesState stagesState = ProtoMappers.toProtoStagesState(flowLaunch, Map.of(),
                            blockedStageIdToLaunch);
                    var launch = random.nextObject(Launch.class)
                            .toBuilder()
                            .launchId(flowLaunch.getLaunchId())
                            .build();
                    return ProtoMappers.toProtoLaunchState(flowLaunch, launch, stagesState);
                })
                .collect(Collectors.toList());

        assertThat(launches)
                .isNotEmpty()
                .doesNotContainNull()
                .extracting(FrontendFlowApi.LaunchState::getFlowId)
                .allSatisfy(flowId -> assertThat(flowId).contains("::"));

        assertThat(launches)
                .extracting(FrontendFlowApi.LaunchState::getProcessId)
                .anyMatch(Common.ProcessId::hasFlowProcessId)
                .anyMatch(Common.ProcessId::hasReleaseProcessId);
    }

    @SafeVarargs
    private static <T extends Enum<T>> Collection<T> valuesOfWithExclusions(Class<T> enumClass, T... exclude) {
        Set<T> excludedValues = Set.of(exclude);
        return Stream.of(enumClass.getEnumConstants())
                .filter(v -> !excludedValues.contains(v))
                .collect(Collectors.toList());
    }

    @Test
    void toLaunchStatus() {
        for (var protoStatus : FrontendOnCommitFlowLaunchApi.FlowLaunchStatus.values()) {
            if (protoStatus == FrontendOnCommitFlowLaunchApi.FlowLaunchStatus.UNKNOWN
                    || protoStatus == FrontendOnCommitFlowLaunchApi.FlowLaunchStatus.UNRECOGNIZED) {
                continue;
            }
            ProtoMappers.toLaunchStatus(protoStatus);
        }
    }

    @Test
    void toBranch() throws TextFormat.ParseException {
        Common.Branch protoBranch = ProtoMappers.toProtoBranch(Branch.of(BranchInfo.builder()
                        .branch("name")
                        .baseRevision(TestData.TRUNK_R6)
                        .created(Instant.ofEpochSecond(1618332361))
                        .createdBy(TestData.USER42)
                        .build(),
                TimelineBranchItem.builder()
                        .vcsInfo(BranchVcsInfo.builder()
                                .trunkCommitCount(78)
                                .branchCommitCount(13)
                                .build())
                        .state(BranchState.builder()
                                .activeLaunches(Set.of(1, 2, 3))
                                .cancelledBaseLaunches(Set.of(5, 6))
                                .cancelledBranchLaunches(Set.of(7, 8, 9, 10, 11, 12, 13)) // 7 штук
                                .completedLaunches(Set.of(14, 15, 16, 17))
                                .freeCommits(5)
                                .build())
                        .build()
        ));

        assertThat(protoBranch).isEqualTo(TestUtils.parseProtoTextFromString("""
                name: "name"
                created_by: "user42"
                created {
                  seconds: 1618332361
                }
                base_revision_hash: "r6"
                trunk_commits_count: 78
                branch_commits_count: 13
                active_launches_count: 3
                completed_launches_count: 4
                cancelled_launches_count: 7
                """, Common.Branch.class));
    }

    @Test
    void toTimelineBranch() throws TextFormat.ParseException {
        Common.TimelineBranch protoBranch = ProtoMappers.toProtoTimelineBranch(Branch.of(BranchInfo.builder()
                        .branch("name")
                        .baseRevision(TestData.TRUNK_R6)
                        .created(Instant.ofEpochSecond(1618332361))
                        .createdBy(TestData.USER42)
                        .build(),
                TimelineBranchItem.builder()
                        .vcsInfo(BranchVcsInfo.builder()
                                .trunkCommitCount(78)
                                .branchCommitCount(13)
                                .build())
                        .state(BranchState.builder()
                                .activeLaunches(Set.of(1, 2, 3))
                                .cancelledBaseLaunches(Set.of(5, 6))
                                .cancelledBranchLaunches(Set.of(7, 8, 9, 10, 11, 12, 13)) // 7 штук
                                .completedLaunches(Set.of(14, 15, 16, 17))
                                .freeCommits(5)
                                .build())
                        .build()
                ),
                Map.of()
        );

        assertThat(protoBranch).isEqualTo(TestUtils.parseProtoTextFromString("""
                branch: {
                    name: "name"
                    created_by: "user42"
                    created {
                      seconds: 1618332361
                    }
                    base_revision_hash: "r6"
                    trunk_commits_count: 78
                    branch_commits_count: 13
                    active_launches_count: 3
                    completed_launches_count: 4
                    cancelled_launches_count: 7
                }
                free_branch_commits_count: 5
                """, Common.TimelineBranch.class));
    }

    @Test
    void toProtoFlowLaunch_withNullValuesAndEmptyFields() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "flow-id");
        LaunchId launchId = LaunchId.of(processId, 1);
        Launch launch = Launch.builder()
                .launchId(launchId)
                .title("Мега релиз #42")
                .project("ci")
                .triggeredBy("andreevdm")
                .created(Instant.now())
                .type(Launch.Type.USER)
                .notifyPullRequest(true)
                .flowInfo(
                        LaunchFlowInfo.builder()
                                .configRevision(TestData.TRUNK_R4)
                                .flowId(FlowFullId.ofFlowProcessId(
                                        launchId.getProcessId()
                                ))
                                .stageGroupId("my-stages")
                                .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                                .build()
                )
                .userData(new LaunchUserData(List.of(), false))
                .vcsInfo(FlowTestUtils.VCS_INFO)
                .status(LaunchState.Status.RUNNING)
                .statusText("")
                //.started(Instant.now())
                //.finished(Instant.now())
                //.tag("tag1")
                .build();

        var flowProcessId = Common.FlowProcessId.newBuilder()
                .setId(processId.getSubId())
                .setDir(processId.getDir());

        assertThat(ProtoMappers.toProtoFlowLaunch(launch, null)).isEqualTo(
                FrontendOnCommitFlowLaunchApi.FlowLaunch.newBuilder()
                        .setFlowProcessId(flowProcessId)
                        .setNumber(1)
                        .setTitle(launch.getTitle())
                        .setTriggeredBy(launch.getTriggeredBy())
                        .setTriggeredBy(launch.getTriggeredBy())
                        .setCreated(ProtoConverter.convert(launch.getCreated()))
                        .setStatus(ProtoMappers.toProtoLaunchStatus(launch.getStatus()))
                        .setLaunchStatus(ProtoMappers.toProtoFlowLaunchStatus(launch.getStatus()))
                        .setStatusText(launch.getStatusText())
                        .setCancelable(!launch.getStatus().isTerminal())
                        .setRevisionHash(FlowTestUtils.VCS_INFO.getRevision().getCommitId())
                        .setBranch(
                                FrontendOnCommitFlowLaunchApi.Branch.newBuilder()
                                        .setName(FlowTestUtils.VCS_INFO.getRevision().getBranch().asString())
                                        .build()
                        )
                        .setFlowDescription(
                                Common.FlowDescription.newBuilder()
                                        .setFlowProcessId(flowProcessId))
                        .build()
        );
    }

    @Test
    void toProtoFlowLaunchStatus() {
        Stream.of(LaunchState.Status.values())
                .filter(it -> it != LaunchState.Status.WAITING_FOR_STAGE)
                .forEach(ProtoMappers::toProtoFlowLaunchStatus);
    }


    @ParameterizedTest
    @ValueSource(booleans = {true, false})
    void findBlockedStages(boolean direct) throws IOException {
        var id = FlowLaunchId.of("3bad2fbd064905ee268c9f4f73ed0f80a978a5b960172d9bb89bc4b2130ad049");

        var reader = CiJson.mapper().reader();
        var item1 = reader.readValue("""
                {
                    "acquiredStageIds": [
                        "build"
                    ],
                    "flowLaunchId": {
                        "value": "3bad2fbd064905ee268c9f4f73ed0f80a978a5b960172d9bb89bc4b2130ad049"
                    },
                    "lockIntent": {
                        "desiredStageId": "release",
                        "stageIdsToUnlock": [
                            "build"
                        ]
                    }
                }
                """, StageGroupState.QueueItem.class);
        var item2 = reader.readValue("""
                {
                    "acquiredStageIds": [
                        "testing"
                    ],
                    "flowLaunchId": {
                        "value": "5583dd775ac73683aa08a13cec75ed6fcc32299bd244263758003b1423b7270e"
                    }
                }
                """, StageGroupState.QueueItem.class);
        var queue = direct ? List.of(item1, item2) : List.of(item2, item1);
        var state = StageGroupState.from("junk/miroslav2/ci/mixed-stages/a.yaml:r:main", 0, queue);
        var blockedStages = ProtoMappers.findBlockedStages(id, state);
        assertThat(blockedStages)
                .isEqualTo(Map.of("release",
                        FlowLaunchId.of("5583dd775ac73683aa08a13cec75ed6fcc32299bd244263758003b1423b7270e")));
    }

    @ParameterizedTest
    @EnumSource(StatusChangeType.class)
    void toProtoStatusChangeType(StatusChangeType type) {
        assertThat(ProtoMappers.toProtoStatusChangeType2(type))
                .isEqualTo(StatusChangeWrapper.StatusChangeType.valueOf(type.name()));
    }

    @Test
    void toProtoStatusChangeTypeNull() {
        assertThat(ProtoMappers.toProtoStatusChangeType2(null))
                .isEqualTo(StatusChangeWrapper.StatusChangeType.NO_STATUS);
    }

    @Test
    void toProtoLaunchState_whenLaunchIsCancelable() {
        CiProcessId ciProcessId = random.nextBoolean() ?
                CiProcessId.ofFlow(Path.of("ci/a.yaml"), "test") :
                CiProcessId.ofRelease(Path.of("ci/a.yaml"), "test");
        var flowLaunchEntity = random.nextObject(FlowLaunchEntity.class)
                .withProcessId(ciProcessId.asString());
        var launch = random.nextObject(Launch.class)
                .toBuilder()
                .status(LaunchState.Status.RUNNING)
                .launchId(LaunchId.of(ciProcessId, 1))
                .build();

        var resultProto = ProtoMappers.toProtoLaunchState(flowLaunchEntity, launch,
                Common.StagesState.getDefaultInstance());
        assertThat(resultProto).extracting(FrontendFlowApi.LaunchState::getCancelable)
                .isEqualTo(true);
    }
}
