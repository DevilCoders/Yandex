package ru.yandex.ci.flow.engine.definition.common;

import java.time.DayOfWeek;
import java.util.Arrays;
import java.util.Map;
import java.util.TreeMap;
import java.util.function.Function;
import java.util.stream.Collectors;

import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

/**
 * Ограничение по дням.
 * allowedDayOfWeekIntervals - список дней в которые разрешён запуск для указанного интервала
 */
@Persisted
@Value
public class WeekSchedulerConstraintEntity {

    TreeMap<DayOfWeek, SchedulerIntervalEntity> allowedDayOfWeekIntervals;

    public static WeekSchedulerConstraintEntity of() {
        return new WeekSchedulerConstraintEntity(new TreeMap<>());
    }

    public static WeekSchedulerConstraintEntity of(SchedulerIntervalEntity allDaysAllowedInterval) {
        var days = Arrays.stream(DayOfWeek.values())
                .collect(Collectors.toMap(Function.identity(), v ->
                        allDaysAllowedInterval, (a, b) -> b, TreeMap::new));
        return new WeekSchedulerConstraintEntity(days);
    }

    public static WeekSchedulerConstraintEntity of(WeekSchedulerConstraintEntity schedulerConstraint) {
        return new WeekSchedulerConstraintEntity(new TreeMap<>(schedulerConstraint.allowedDayOfWeekIntervals));
    }

    public Map<DayOfWeek, SchedulerIntervalEntity> getAllowedDayOfWeekIntervals() {
        return allowedDayOfWeekIntervals;
    }

    public boolean isNotEmptyWeekConstraints() {
        return !allowedDayOfWeekIntervals.isEmpty();
    }

    public WeekSchedulerConstraintEntity addAllowedDayOfWeek(DayOfWeek dayOfWeek,
                                                             SchedulerIntervalEntity schedulerInterval) {
        allowedDayOfWeekIntervals.put(dayOfWeek, schedulerInterval);
        return this;
    }

    public WeekSchedulerConstraintEntity removeAllowedDayOfWeek(DayOfWeek dayOfWeek) {
        allowedDayOfWeekIntervals.remove(dayOfWeek);
        return this;
    }

    @Override
    public String toString() {
        return "WeekSchedulerConstraintEntity{" +
            allowedDayOfWeekIntervals.entrySet().stream()
                .map(e -> e.getKey() + "=" + e.getValue())
                .collect(Collectors.joining(",")) +
            "}";
    }
}
