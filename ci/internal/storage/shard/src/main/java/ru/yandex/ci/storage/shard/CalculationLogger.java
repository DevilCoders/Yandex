package ru.yandex.ci.storage.shard;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;

@Slf4j
public class CalculationLogger {
    private CalculationLogger() {

    }

    public static void logStateUpdate(TestDiffByHashEntity old, TestDiffByHashEntity updated) {
        log.info("{} {} -> {}", old.getId(), getState(old), getState(updated));
    }

    private static String getState(TestDiffByHashEntity diff) {
        return "[%s,%s]".formatted(diff.getLeft(), diff.getRight());
    }
}
