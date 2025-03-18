package ru.yandex.ci.storage.reader.check.listeners.util;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.ChunkType;
import ru.yandex.ci.storage.core.check.ArcanumCheckStatus;
import ru.yandex.ci.storage.core.db.constant.ChunkTypeUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.TestTypeStatistics;

import static ru.yandex.ci.storage.core.Common.ChunkType.CT_BUILD;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_CONFIGURE;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_LARGE_TEST;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_MEDIUM_TEST;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_NATIVE_BUILD;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_SMALL_TEST;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_STYLE;
import static ru.yandex.ci.storage.core.Common.ChunkType.CT_TESTENV;

public class CheckReportUtils {
    private static final Map<ArcanumCheckType, Set<ChunkType>> REQUIRED_CHUNKS_BY_ARCANUM_MERGE_REQUIREMENT =
            Map.of(
                    ArcanumCheckType.CI_BUILD, Set.of(CT_CONFIGURE, CT_BUILD),
                    ArcanumCheckType.CI_TESTS, Set.of(CT_BUILD, CT_STYLE, CT_SMALL_TEST, CT_MEDIUM_TEST),
                    ArcanumCheckType.CI_LARGE_TESTS, Set.of(CT_LARGE_TEST),
                    ArcanumCheckType.CI_BUILD_NATIVE, Set.of(CT_NATIVE_BUILD),
                    ArcanumCheckType.TE_JOBS, Set.of(CT_TESTENV)
            );

    private CheckReportUtils() {
    }

    public static Map<ArcanumCheckType, ArcanumCheckStatus> evaluateRequirementStatusMap(
            TestTypeStatistics testTypeStatistics,
            MainStatistics mainStatistics
    ) {
        var checkTypes = ArcanumCheckType.values();
        var requirementStatusMap = new HashMap<ArcanumCheckType, ArcanumCheckStatus>(checkTypes.length);
        for (var requirement : checkTypes) {
            var requirementStatus = ArcanumCheckStatus.success();
            var chunks = REQUIRED_CHUNKS_BY_ARCANUM_MERGE_REQUIREMENT.get(requirement);

            if (chunks == null) {
                requirementStatus = ArcanumCheckStatus.skipped();
            } else {
                for (var requiredChunk : chunks) {
                    var testType = testTypeStatistics.get(requiredChunk);
                    if (!testType.isCompleted()) {
                        requirementStatus = ArcanumCheckStatus.pending();
                        break;
                    }

                    var statistics = mainStatistics.getByChunkTypeOrEmpty(requiredChunk);

                    if (statistics.toCompletedStatus() != Common.CheckStatus.COMPLETED_SUCCESS) {
                        requirementStatus = ArcanumCheckStatus.failure(
                                "New failure found in " + ChunkTypeUtils.toHumanReadable(requiredChunk)
                        );
                        break;
                    }
                }
            }

            requirementStatusMap.put(requirement, requirementStatus);
        }
        return requirementStatusMap;
    }
}
