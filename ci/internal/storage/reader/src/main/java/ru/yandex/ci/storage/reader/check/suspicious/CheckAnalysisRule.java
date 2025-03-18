package ru.yandex.ci.storage.reader.check.suspicious;

import java.util.Collection;
import java.util.function.Consumer;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public interface CheckAnalysisRule {

    default String getId() {
        return this.getClass().getSimpleName();
    }

    default void analyzeIteration(CheckIterationEntity iteration, Consumer<String> addAlert) {
    }

    default void analyzeCheck(
            CheckEntity check, Collection<CheckIterationEntity> iterations, Consumer<String> addAlert
    ) {
    }
}
