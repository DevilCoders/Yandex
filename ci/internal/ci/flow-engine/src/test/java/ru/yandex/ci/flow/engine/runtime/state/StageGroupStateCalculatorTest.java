package ru.yandex.ci.flow.engine.runtime.state;

import java.nio.file.Path;
import java.time.Instant;
import java.util.Collections;
import java.util.List;

import com.google.common.collect.Sets;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.StageGroupChangeEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.FlowCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.LockAndUnlockStageCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.RemoveFromStageQueueCommand;
import ru.yandex.ci.flow.engine.runtime.state.calculator.commands.StageGroupCommand;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.test.TestFlowId;

import static java.util.Collections.singletonList;

public class StageGroupStateCalculatorTest extends FlowEngineTestBase {
    private static final String TESTING = "testing";
    private static final String PRESTABLE = "prestable";
    private static final String STABLE = "stable";
    private static final String STAGE_GROUP_ID = "test-stages";
    private static final String FIRST_FLOW_ID = "first_flow";
    private static final String SECOND_FLOW_ID = "second_flow";
    private static final String THIRD_FLOW_ID = "third_flow";
    private static final String FOURTH_FLOW_ID = "fourth_flow";

    private final StageGroup stageGroup = new StageGroup(TESTING, PRESTABLE, STABLE);

    private FlowLaunchEntity firstFlowLaunch;
    private FlowLaunchEntity secondFlowLaunch;
    private FlowLaunchEntity thirdFlowLaunch;
    private FlowLaunchEntity rollbackFlowLaunch;

    @BeforeEach
    public void setup() {
        stageGroupSave(StageGroupState.of(STAGE_GROUP_ID));

        firstFlowLaunch = flowLaunch(FIRST_FLOW_ID);

        secondFlowLaunch = flowLaunch(SECOND_FLOW_ID);

        thirdFlowLaunch = flowLaunch(THIRD_FLOW_ID);

        rollbackFlowLaunch = flowLaunch(FOURTH_FLOW_ID);
    }

    private FlowLaunchEntity flowLaunch(String flowId) {
        var flowFullId = TestFlowId.of(flowId);
        return FlowLaunchEntity.builder()
                .id(flowId)
                .launchId(LaunchId.of(CiProcessId.ofFlow(Path.of("ci", AffectedAYamlsFinder.CONFIG_FILE_NAME),
                        "test"), 1))
                .flowInfo(FlowTestUtils.toFlowInfo(flowFullId, "stageGroupId"))
                .launchInfo(FlowTestUtils.SIMPLE_LAUNCH_INFO)
                .vcsInfo(FlowTestUtils.VCS_INFO)
                .createdDate(Instant.now())
                .rawStages(stageGroup.getStages())
                .projectId("prj")
                .build();
    }

    @Test
    public void acquireFreeStage() {

        // act
        List<FlowCommand> commands = unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(TESTING),
                firstFlowLaunch
        );

        // assert
        Assertions.assertEquals(
                Collections.singletonList(stagesUpdatedEvent(firstFlowLaunch.getFlowLaunchId())), commands
        );

        assertAcquiredStages(firstFlowLaunch.getFlowLaunchId(), TESTING);
    }

    @Test
    public void acquireAlreadyAcquiredStage() {
        Assertions.assertThrows(IllegalArgumentException.class, () -> {

            // act
            unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(TESTING), firstFlowLaunch);
            unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(TESTING), firstFlowLaunch);
        });
    }

    @Test
    public void releaseStageThatFlowDoesNotHold() {
        Assertions.assertThrows(IllegalArgumentException.class, () -> {
            // act
            unlockAndLockStage(
                    Collections.singletonList(stageGroup.getStage(TESTING)), stageGroup.getStage(PRESTABLE),
                    firstFlowLaunch
            );
        });
    }

    @Test
    public void acquireBusyStage() {
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(TESTING), firstFlowLaunch);

        // act
        List<FlowCommand> commands = unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(TESTING),
                secondFlowLaunch
        );

        // assert
        Assertions.assertTrue(commands.isEmpty());

        assertAcquiredStages(firstFlowLaunch.getFlowLaunchId(), TESTING);
        assertNoAcquiredStages(secondFlowLaunch.getFlowLaunchId());
    }

    @Test
    public void acquireBusyStageWithDifferentStagesFlows() {
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(PRESTABLE), firstFlowLaunch);
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(STABLE), firstFlowLaunch);

        // act
        List<FlowCommand> commands = unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(STABLE),
                rollbackFlowLaunch
        );

        // assert
        Assertions.assertTrue(commands.isEmpty());

        assertAcquiredStages(firstFlowLaunch.getFlowLaunchId(), PRESTABLE, STABLE);
        assertNoAcquiredStages(rollbackFlowLaunch.getFlowLaunchId());
    }

    @Test
    public void shouldHoldPreviousStageUntilNextAcquired() {
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(PRESTABLE), firstFlowLaunch);
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(TESTING), secondFlowLaunch);

        // act
        unlockAndLockStage(singletonList(stageGroup.getStage(TESTING)), stageGroup.getStage(PRESTABLE),
                secondFlowLaunch);

        // assert
        assertAcquiredStages(firstFlowLaunch.getFlowLaunchId(), PRESTABLE);
        assertAcquiredStages(secondFlowLaunch.getFlowLaunchId(), TESTING);
        StageGroupState stageGroupState = stageGroupState();
        Assertions.assertEquals(PRESTABLE,
                stageGroupState.getQueueItem(secondFlowLaunch.getFlowLaunchId()).orElseThrow().getDesiredStageId());
    }

    @Test
    public void removeFlowFromQueueWhenFinished() {
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(TESTING), firstFlowLaunch);
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(TESTING), secondFlowLaunch);

        // act
        execCommand(new RemoveFromStageQueueCommand(firstFlowLaunch));

        // assert
        assertAcquiredStages(secondFlowLaunch.getFlowLaunchId(), TESTING);
        StageGroupState stageGroupState = stageGroupState();
        Assertions.assertFalse(stageGroupState.getQueueItem(firstFlowLaunch.getFlowLaunchId()).isPresent());
    }

    @Test
    public void skipsStagesOnFlag() {
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(TESTING), firstFlowLaunch);
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(STABLE), secondFlowLaunch, true);
        unlockAndLockStage(Collections.emptyList(), stageGroup.getStage(STABLE), thirdFlowLaunch, true);

        // assert
        assertAcquiredStages(secondFlowLaunch.getFlowLaunchId(), STABLE);
        Assertions.assertEquals(thirdFlowLaunch.getFlowLaunchId(),
                stageGroupState().getQueue().get(1).getFlowLaunchId());
    }

    private void assertNoAcquiredStages(FlowLaunchId flowLaunchId) {
        Assertions.assertEquals(
                Collections.emptySet(),
                stageGroupState().getQueueItem(flowLaunchId).orElseThrow().getAcquiredStageIds()
        );
    }

    private void assertAcquiredStages(FlowLaunchId flowLaunchId, String... stages) {
        Assertions.assertEquals(
                Sets.newHashSet(stages),
                stageGroupState().getQueueItem(flowLaunchId).orElseThrow().getAcquiredStageIds()
        );
    }

    private List<FlowCommand> unlockAndLockStage(List<StageRef> stagesToUnlock, StageRef desiredStage,
                                                 FlowLaunchEntity flowLaunch) {
        return execCommand(new LockAndUnlockStageCommand(stagesToUnlock, desiredStage, flowLaunch, false));
    }

    private List<FlowCommand> unlockAndLockStage(List<StageRef> stagesToUnlock, StageRef desiredStage,
                                                 FlowLaunchEntity flowLaunch, boolean skipStagesAllowed) {
        return execCommand(new LockAndUnlockStageCommand(stagesToUnlock, desiredStage, flowLaunch, skipStagesAllowed));
    }

    private List<FlowCommand> execCommand(StageGroupCommand command) {
        StageGroupState stageGroupState = stageGroupState();
        List<FlowCommand> commands = command.execute(stageGroupState);
        stageGroupSave(stageGroupState);
        return commands;
    }

    private FlowCommand stagesUpdatedEvent(FlowLaunchId flowLaunchId) {
        return new FlowCommand(flowLaunchId, StageGroupChangeEvent.INSTANCE);
    }

    private StageGroupState stageGroupState() {
        return stageGroupGet(STAGE_GROUP_ID);
    }

}
