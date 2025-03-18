package ru.yandex.ci.core.flow;

import lombok.Value;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.ydb.Persisted;

@Value
@Persisted
public class CiActionReference {
    FlowFullId flowId;
    int launchNumber;
}
