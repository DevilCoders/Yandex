package ru.yandex.ci.flow.engine.runtime.events;


import lombok.ToString;

@ToString
public class StageGroupChangeEvent implements FlowEvent {
    public static final StageGroupChangeEvent INSTANCE = new StageGroupChangeEvent();

    private StageGroupChangeEvent() {
    }
}
