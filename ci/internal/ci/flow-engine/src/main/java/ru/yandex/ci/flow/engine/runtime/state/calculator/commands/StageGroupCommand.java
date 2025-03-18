package ru.yandex.ci.flow.engine.runtime.state.calculator.commands;

import java.util.List;

import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;

public interface StageGroupCommand extends RecalcCommand {
    /**
     * Пересчитывает состоение синхронизирующей очереди пайлпайнов {@link StageGroupState}.
     *
     * @param stageGroupState Состояние, которое будет пересчитано и мутировано в ходе пересчёта.
     * @return список команд
     */
    List<FlowCommand> execute(StageGroupState stageGroupState);
}
