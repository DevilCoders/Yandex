package ru.yandex.ci.flow.engine.definition.stage;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

/**
 * Дефолтная POJO-имплементация {@link StageRef}. Сериализуема, можно персистить в монгу.
 */
@Persisted
@Value
public class StageRefImpl implements StageRef {
    String id;

    public static StageRefImpl fromStageRef(StageRef stage) {
        return new StageRefImpl(stage.getId());
    }

    @Override
    public String getId() {
        return id;
    }
}
