package ru.yandex.ci.storage.reader.check;

import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

@Slf4j
public class CheckStatusResolver {
    private CheckStatusResolver() {

    }

    public static Common.CheckStatus evaluateFinishedCheckStatus(
            CheckEntity.Id checkId,
            Collection<CheckIterationEntity> checkIterations
    ) {
        var iterationTypes = checkIterations.stream().collect(
                Collectors.groupingBy(x -> x.getId().getIterationType())
        );

        var statusesByType = new HashMap<CheckIteration.IterationType, Common.CheckStatus>(3);

        for (var group : iterationTypes.values()) {
            var first = group.stream().min(Comparator.comparing(x -> x.getId().getNumber())).orElseThrow();
            statusesByType.put(first.getId().getIterationType(), first.getStatus());
        }

        var heavy = statusesByType.get(CheckIteration.IterationType.HEAVY);
        if (heavy != null && !heavy.equals(Common.CheckStatus.COMPLETED_SUCCESS)) {
            log.info(
                    "Completing check {} with status from heavy circuit: {}", checkId, heavy
            );
            return heavy;
        }

        var full = statusesByType.get(CheckIteration.IterationType.FULL);
        if (full != null) {
            log.info(
                    "Completing check {} with status from full circuit: {}", checkId, full
            );
            return full;
        }

        var fast = statusesByType.get(CheckIteration.IterationType.FAST);
        if (fast != null) {
            log.info(
                    "Completing check {} with status from fast circuit: {}", checkId, fast
            );
            return fast;
        }

        Preconditions.checkNotNull(heavy, "All statuses are null for " + checkId);

        log.info(
                "Completing check {} with status from heavy circuit: {}", checkId, heavy
        );

        return heavy;
    }
}
