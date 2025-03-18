package ru.yandex.ci.flow.engine.definition.stage;

import javax.annotation.Nullable;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public final class StageGroupHelper {

    private static final String DELIMITER = "::";

    private StageGroupHelper() {
    }


    @Nullable
    public static String createStageGroupId(CiProcessId processId, @Nullable ArcBranch branch) {
        if (processId.getType() != CiProcessId.Type.RELEASE) {
            return null;
        }
        if (branch == null || branch.isUnknown() || branch.isTrunk()) {
            return processId.asString();
        } else {
            return processId.asString() + DELIMITER + branch.asString();
        }
    }

    @Nullable
    public static String createStageGroupId(CiProcessId processId) {
        return createStageGroupId(processId, null);
    }

}
