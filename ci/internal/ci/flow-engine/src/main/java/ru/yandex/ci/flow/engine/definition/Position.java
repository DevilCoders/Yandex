package ru.yandex.ci.flow.engine.definition;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class Position {
    int x;
    int y;
}
