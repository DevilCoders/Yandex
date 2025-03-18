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
public class UnlockStageCommand implements StageGroupCommand {
    @Nonnull
    StageRef stage;
    @Nonnull
    String flowLaunchId;

    @EqualsAndHashCode.Exclude
    @ToString.Exclude
    @Getter(AccessLevel.NONE)
    FlowLaunchEntity flowLaunch;

    public UnlockStageCommand(@Nonnull StageRef stage, @Nonnull FlowLaunchEntity flowLaunch) {
        this.stage = stage;
        this.flowLaunch = flowLaunch;
        this.flowLaunchId = flowLaunch.getIdString();
    }

    @Override
    public List<FlowCommand> execute(StageGroupState stageGroupState) {
        StageGroupStateCalculator stateCalculator = new StageGroupStateCalculator(stageGroupState, flowLaunch);
        stateCalculator.unlockStage(stage.getId(), flowLaunch.getFlowLaunchId());
        return stateCalculator.getCommands();
    }
}
