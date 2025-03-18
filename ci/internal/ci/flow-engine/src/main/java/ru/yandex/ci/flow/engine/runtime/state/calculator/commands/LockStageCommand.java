package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.AccessLevel;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.ToString;
import lombok.Value;

import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupStateCalculator;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;

@Value
public class LockStageCommand implements StageGroupCommand {
    @Nonnull
    StageRef desiredStage;
    @Nonnull
    String flowLaunchId;
    @EqualsAndHashCode.Exclude
    boolean skipStagesAllowed;

    @EqualsAndHashCode.Exclude
    @ToString.Exclude
    @Getter(AccessLevel.NONE)
    FlowLaunchEntity flowLaunch;

    public LockStageCommand(@Nonnull StageRef desiredStage, @Nonnull FlowLaunchEntity flowLaunch,
                            boolean skipStagesAllowed) {
        this.desiredStage = desiredStage;
        this.flowLaunchId = flowLaunch.getIdString();
        this.skipStagesAllowed = skipStagesAllowed;
        this.flowLaunch = flowLaunch;
    }

    @Override
    public List<FlowCommand> execute(StageGroupState stageGroupState) {
        StageGroupStateCalculator stateCalculator = new StageGroupStateCalculator(stageGroupState, flowLaunch);
        stateCalculator.lockStage(desiredStage.getId(), flowLaunch.getFlowLaunchId(), skipStagesAllowed);
        return stateCalculator.getCommands();
    }
}
