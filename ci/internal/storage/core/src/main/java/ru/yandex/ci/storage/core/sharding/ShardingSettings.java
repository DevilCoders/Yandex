package ru.yandex.ci.storage.core.sharding;

import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.ydb.Persisted;

@Value
@Persisted
public class ShardingSettings {
    public static final ShardingSettings DEFAULT = new ShardingSettings(1, 1, 1, 1, 1, 1, 1, 1, 0, 0);
    int configureChunks;
    int buildChunks;
    int styleChunks;
    int smallTestChunks;
    int mediumTestChunks;
    int largeTestChunks;
    int teTestChunks;
    int nativeBuildsChunks;
    int chunkSamplingPercent;
    int numberOfChecksToSkip;

    public int getChunksCount(Common.ChunkType chunkType) {
        return switch (chunkType) {
            case CT_CONFIGURE -> configureChunks;
            case CT_BUILD -> buildChunks;
            case CT_STYLE -> styleChunks;
            case CT_SMALL_TEST -> smallTestChunks;
            case CT_MEDIUM_TEST -> mediumTestChunks;
            case CT_LARGE_TEST -> largeTestChunks;
            case CT_TESTENV -> teTestChunks;
            case CT_NATIVE_BUILD -> nativeBuildsChunks;
            case UNRECOGNIZED -> throw new RuntimeException();
        };
    }
}
