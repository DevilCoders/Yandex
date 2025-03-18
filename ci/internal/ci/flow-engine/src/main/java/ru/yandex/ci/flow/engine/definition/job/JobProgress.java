package ru.yandex.ci.flow.engine.definition.job;

import java.util.LinkedHashMap;
import java.util.Map;

import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

public class JobProgress {
    private final Map<String, TaskBadge> taskStates = new LinkedHashMap<>();

    private String text = "";
    private Float ratio = 0.f;

    public Map<String, TaskBadge> getTaskStates() {
        return taskStates;
    }

    public String getText() {
        return text;
    }

    public void setText(String text) {
        this.text = text;
    }

    public Float getRatio() {
        return ratio;
    }

    public void setRatio(Float ratio) {
        this.ratio = ratio;
    }
}
