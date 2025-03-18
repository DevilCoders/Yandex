package ru.yandex.ci.flow.engine.runtime.state.model;

import lombok.Value;
import lombok.With;

@Value(staticConstructor = "of")
public class StageStatistics {
    long started;
    @With
    long finished;
}
