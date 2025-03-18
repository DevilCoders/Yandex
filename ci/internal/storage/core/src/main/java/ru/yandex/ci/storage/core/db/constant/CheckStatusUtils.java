package ru.yandex.ci.storage.core.db.constant;

import java.util.Set;

import yandex.cloud.repository.kikimr.yql.YqlPredicate;

import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.storage.core.Common.CheckStatus;

public final class CheckStatusUtils {

    // ACTIVE, SUCCESS and FAILURE must describe all available classes and do not intersect
    public static final Set<CheckStatus> ACTIVE = Set.of(
            CheckStatus.CREATED,
            CheckStatus.RUNNING,
            CheckStatus.CANCELLING,
            CheckStatus.CANCELLING_BY_TIMEOUT,
            CheckStatus.COMPLETING
    );
    public static final Set<CheckStatus> SUCCESS = Set.of(
            CheckStatus.COMPLETED_SUCCESS
    );
    public static final Set<CheckStatus> FAILURE = Set.of(
            CheckStatus.CANCELLED,
            CheckStatus.CANCELLED_BY_TIMEOUT,
            CheckStatus.COMPLETED_FAILED,
            CheckStatus.COMPLETED_WITH_FATAL_ERROR
    );

    private static final Set<CheckStatus> CANCELLED = Set.of(
            CheckStatus.CANCELLED,
            CheckStatus.CANCELLED_BY_TIMEOUT,
            CheckStatus.COMPLETED_WITH_FATAL_ERROR
    );

    private static final Set<CheckStatus> RUNNING = Set.of(
            CheckStatus.CREATED, CheckStatus.RUNNING
    );

    private CheckStatusUtils() {
    }

    public static YqlPredicate getIsActive(String fieldName) {
        return YqlPredicateCi.in(fieldName, ACTIVE);
    }

    public static YqlPredicate getRunning(String fieldName) {
        return YqlPredicateCi.in(fieldName, RUNNING);
    }

    public static boolean isActive(CheckStatus status) {
        return ACTIVE.contains(status);
    }

    public static boolean isRunning(CheckStatus status) {
        return RUNNING.contains(status);
    }

    public static boolean isCompleted(CheckStatus status) {
        return !ACTIVE.contains(status);
    }

    public static boolean isCancelled(CheckStatus status) {
        return CANCELLED.contains(status);
    }
}
