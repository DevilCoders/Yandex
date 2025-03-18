package ru.yandex.ci.core.discovery;

import lombok.Value;

import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.ydb.Persisted;

/**
 * Описывает изменение конфига в репозитории, например добавление, удаление, изменение содержимного и т.д.
 */
@Persisted
@Value
public class ConfigChange {
    ConfigChangeType type;
}
