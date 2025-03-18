package ru.yandex.ci.flow.engine.runtime.events;

import lombok.EqualsAndHashCode;
import lombok.ToString;

import ru.yandex.ci.flow.engine.definition.stage.StageRef;


@ToString
@EqualsAndHashCode
public class UnlockStageEvent implements FlowEvent {
    private final StageRef stageRef;

    public UnlockStageEvent(StageRef stageRef) {
        this.stageRef = stageRef;
    }

    public StageRef getStageRef() {
        return stageRef;
    }

}
