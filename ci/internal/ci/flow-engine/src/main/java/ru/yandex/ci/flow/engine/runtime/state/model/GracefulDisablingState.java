package ru.yandex.ci.flow.engine.runtime.state.model;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value(staticConstructor = "of")
public class GracefulDisablingState {
    boolean inProgress;
    boolean ignoreUninterruptibleStage;
}
