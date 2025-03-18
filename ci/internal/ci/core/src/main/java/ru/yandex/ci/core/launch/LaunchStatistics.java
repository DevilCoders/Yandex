package ru.yandex.ci.core.launch;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Value
@Builder(toBuilder = true)
@Persisted
public class LaunchStatistics {
    int retries;
}

