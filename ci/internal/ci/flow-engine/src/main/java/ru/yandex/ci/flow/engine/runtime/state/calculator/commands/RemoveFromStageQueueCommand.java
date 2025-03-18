package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupStateCalculator;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;

@Value
public class RemoveFromStageQueueCommand implements StageGroupCommand {
    @Nonnull
    FlowLaunchEntity flowLaunch;

    @Override
    public List<FlowCommand> execute(StageGroupState stageGroupState) {
        StageGroupStateCalculator stateCalculator = new StageGroupStateCalculator(stageGroupState, flowLaunch);
        stateCalculator.removeFromQueue(flowLaunch.getFlowLaunchId());
        return stateCalculator.getCommands();
    }
}
