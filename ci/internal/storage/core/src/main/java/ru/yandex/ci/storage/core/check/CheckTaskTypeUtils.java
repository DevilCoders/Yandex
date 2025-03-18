package ru.yandex.ci.storage.core.check;

import java.nio.file.Path;
import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.ChunkType;

import static ru.yandex.ci.storage.core.Common.ChunkType.CT_BUILD;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_CONFIGURE;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_LARGE_TEST;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_MEDIUM_TEST;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_NATIVE_BUILD;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_SMALL_TEST;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_TESTENV;

public class CheckTaskTypeUtils {
    private CheckTaskTypeUtils() {

    }

    public static Set<ChunkType> toChunkType(Common.CheckTaskType type) {
        return switch (type) {
            case CTT_AUTOCHECK -> Set.of(CT_CONFIGURE, CT_BUILD, CT_SMALL_TEST, CT_MEDIUM_TEST, CT_LARGE_TEST);
            case CTT_LARGE_TEST -> Set.of(CT_LARGE_TEST);
            case CTT_NATIVE_BUILD -> Set.of(CT_NATIVE_BUILD);
            case CTT_TESTENV -> Set.of(CT_TESTENV);
            case UNRECOGNIZED -> throw new RuntimeException("Unrecognized");
        };
    }

    public static CiProcessId toCiProcessId(
            Common.CheckTaskType type,
            String target,
            @Nullable String nativeTarget,
            String toolchain,
            String testName

    ) {
        String actualTarget;
        VirtualCiProcessId.VirtualType virtualType;
        switch (type) {
            case CTT_LARGE_TEST -> {
                actualTarget = target;
                virtualType = VirtualCiProcessId.VirtualType.VIRTUAL_LARGE_TEST;
            }
            case CTT_NATIVE_BUILD -> {
                Preconditions.checkState(nativeTarget != null, "Internal error. NativeTarget is null for %s", type);
                actualTarget = nativeTarget;
                virtualType = VirtualCiProcessId.VirtualType.VIRTUAL_NATIVE_BUILD;
            }
            default -> throw new IllegalStateException("Unsupported check task type: " + type);
        }
        var ciProcessId = CiProcessId.ofFlow(
                Path.of(actualTarget, AffectedAYamlsFinder.CONFIG_FILE_NAME),
                toolchain + "@" + testName
        );
        return VirtualCiProcessId.toVirtual(ciProcessId, virtualType);
    }
}
