package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupStateCalculator;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;

/**
 * Команда, позволяющая захватить стадию и после взятия отпустить одну или несколько стадий. До момента, пока желаемая
 * стадия не захвачена, stagesToUnlock не будут отпущены. Иными словами, не будет ситуации, когда флоу остался
 * вообще вне стадий.
 */
@Value
public class LockAndUnlockStageCommand implements StageGroupCommand {
    @Nonnull
    Collection<? extends StageRef> stagesToUnlock;
    @Nonnull
    StageRef stageToLock;
    @Nonnull
    FlowLaunchEntity flowLaunch;
    boolean skipStagesAllowed;

    @Override
    public List<FlowCommand> execute(StageGroupState stageGroupState) {
        StageGroupStateCalculator stateCalculator = new StageGroupStateCalculator(stageGroupState, flowLaunch);
        stateCalculator.tryLockStage(
                stagesToUnlock.stream()
                        .map(StageRef::getId)
                        .collect(Collectors.toList()),
                stageToLock.getId(), flowLaunch.getFlowLaunchId(), skipStagesAllowed
        );

        return stateCalculator.getCommands();
    }
}
