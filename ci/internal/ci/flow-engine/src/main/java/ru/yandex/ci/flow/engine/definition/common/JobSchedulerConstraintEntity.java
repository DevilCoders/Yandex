package ru.yandex.ci.flow.engine.definition.common;

import java.util.Map;
import java.util.TreeMap;
import java.util.stream.Collectors;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

/**
 * Расписание запуска.
 * Отсутствие ограничения или пустое ограничение - запрет на выполенение задач для данного типа ограничений
 */
@Persisted
@Value
public class JobSchedulerConstraintEntity {
    TreeMap<TypeOfSchedulerConstraint, WeekSchedulerConstraintEntity> weekConstraints;

    public static JobSchedulerConstraintEntity of() {
        return new JobSchedulerConstraintEntity(new TreeMap<>());
    }

    public static JobSchedulerConstraintEntity of(JobSchedulerConstraintEntity jobSchedulerConstraint) {
        var map = jobSchedulerConstraint.weekConstraints.entrySet().stream()
                .collect(Collectors.toMap(Map.Entry::getKey, e -> WeekSchedulerConstraintEntity.of(e.getValue()),
                        (a, b) -> b, TreeMap::new));
        return new JobSchedulerConstraintEntity(map);
    }

    public WeekSchedulerConstraintEntity getWeekConstraintsForType(
            TypeOfSchedulerConstraint typeOfSchedulerConstraint
    ) {
        return weekConstraints.get(typeOfSchedulerConstraint);
    }

    public boolean isEmptyWeekConstraints() {
        return weekConstraints.values().stream()
                .noneMatch(WeekSchedulerConstraintEntity::isNotEmptyWeekConstraints);
    }
}
