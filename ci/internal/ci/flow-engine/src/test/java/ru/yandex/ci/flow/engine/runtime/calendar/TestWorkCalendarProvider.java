package ru.yandex.ci.flow.engine.runtime.calendar;

import java.time.LocalDate;
import java.util.HashMap;
import java.util.Map;

import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;

public class TestWorkCalendarProvider implements WorkCalendarProvider {
    private final Map<LocalDate, TypeOfSchedulerConstraint> manual = new HashMap<>();

    public void reset() {
        manual.clear();
    }

    public void setDayType(LocalDate date, TypeOfSchedulerConstraint type) {
        manual.put(date, type);
    }

    @Override
    public TypeOfSchedulerConstraint getTypeOfDay(LocalDate day) {
        var manualType = this.manual.get(day);
        if (manualType != null) {
            return manualType;
        }
        return switch (day.getDayOfWeek()) {
            case FRIDAY -> TypeOfSchedulerConstraint.PRE_HOLIDAY;
            case SATURDAY, SUNDAY -> TypeOfSchedulerConstraint.HOLIDAY;
            default -> TypeOfSchedulerConstraint.WORK;
        };
    }
}
