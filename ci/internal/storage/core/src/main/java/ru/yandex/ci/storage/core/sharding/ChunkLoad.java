package ru.yandex.ci.storage.core.sharding;

import java.time.Instant;
import java.util.Map;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Value
@Persisted
public class ChunkLoad {
    Map<Integer, Integer> hourLoad;
    Instant lastWrite;
}
