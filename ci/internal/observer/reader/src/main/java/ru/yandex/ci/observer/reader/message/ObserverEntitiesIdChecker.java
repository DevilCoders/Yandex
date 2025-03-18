package ru.yandex.ci.observer.reader.message;

import java.util.Set;

import com.google.common.base.Preconditions;

import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;

public class ObserverEntitiesIdChecker {
    private ObserverEntitiesIdChecker() {
    }

    public static boolean isMetaIteration(CheckIteration.IterationId iterationId) {
        return iterationId.getNumber() == 0;
    }

    public static boolean isProcessableIteration(
            CheckIteration.IterationId iterationId,
            Set<CheckIteration.IterationType> processableIterationTypes
    ) {
        return processableIterationTypes.contains(iterationId.getCheckType());
    }

    public static boolean isMetaIteration(CheckIterationEntity.Id iterationId) {
        return iterationId.getNumber() == 0;
    }

    public static void checkFullTaskId(CheckTaskOuterClass.FullTaskId fullTaskId) {
        Preconditions.checkNotNull(fullTaskId, "FullTaskId missing");
        Preconditions.checkNotNull(fullTaskId.getIterationId(), "IterationId missing");
        Preconditions.checkState(!fullTaskId.getTaskId().isEmpty(), "TaskId missing");
    }
}
