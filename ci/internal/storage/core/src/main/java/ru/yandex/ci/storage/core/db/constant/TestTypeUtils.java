package ru.yandex.ci.storage.core.db.constant;

import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common;

public class TestTypeUtils {
    private TestTypeUtils() {

    }

    public static Set<Common.ChunkType> toChunkType(
            Actions.TestType testType, @Nullable Actions.TestTypeSizeFinished.Size size
    ) {
        if (testType.equals(Actions.TestType.TEST) && size == null) {
            return Set.of(
                    Common.ChunkType.CT_SMALL_TEST, Common.ChunkType.CT_MEDIUM_TEST, Common.ChunkType.CT_LARGE_TEST
            );
        }

        var result = switch (testType) {
            case CONFIGURE -> Common.ChunkType.CT_CONFIGURE;
            case BUILD -> Common.ChunkType.CT_BUILD;
            case STYLE -> Common.ChunkType.CT_STYLE;
            case TEST -> switch (Objects.requireNonNull(size)) {
                case SMALL -> Common.ChunkType.CT_SMALL_TEST;
                case MEDIUM -> Common.ChunkType.CT_MEDIUM_TEST;
                case LARGE -> Common.ChunkType.CT_LARGE_TEST;
                case UNRECOGNIZED -> throw new RuntimeException("Unrecognized test size");
            };
            case UNRECOGNIZED -> throw new RuntimeException("Unrecognized test type");
        };

        return Set.of(result);
    }
}
