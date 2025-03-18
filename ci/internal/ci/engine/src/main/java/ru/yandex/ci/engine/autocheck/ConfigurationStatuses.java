package ru.yandex.ci.engine.autocheck;

import java.util.Map;

import lombok.Value;

import ru.yandex.ci.core.db.model.KeyValue;
import ru.yandex.ci.ydb.Persisted;

@Value
@Persisted
public class ConfigurationStatuses {
    public static final KeyValue.Id KEY = KeyValue.Id.of("autocheck", "configurationStatuses");

    Map<String, Boolean> platforms;
}
