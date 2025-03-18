package ru.yandex.ci.flow.engine.runtime.events;


import lombok.ToString;

@ToString
public class DisableFlowGracefullyEvent implements FlowEvent {
    private final boolean ignoreUninterruptableStage;

    public DisableFlowGracefullyEvent(boolean ignoreUninterruptableStage) {
        this.ignoreUninterruptableStage = ignoreUninterruptableStage;
    }

    public boolean shouldIgnoreUninterruptableStage() {
        return ignoreUninterruptableStage;
    }

}
