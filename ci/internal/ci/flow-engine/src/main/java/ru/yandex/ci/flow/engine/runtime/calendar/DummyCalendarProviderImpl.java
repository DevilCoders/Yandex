package ru.yandex.ci.flow.engine.runtime.calendar;

import java.time.LocalDate;

import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;

public class DummyCalendarProviderImpl implements WorkCalendarProvider {
    @Override
    public TypeOfSchedulerConstraint getTypeOfDay(LocalDate day) {
        return switch (day.getDayOfWeek()) {
            case FRIDAY -> TypeOfSchedulerConstraint.PRE_HOLIDAY;
            case SATURDAY, SUNDAY -> TypeOfSchedulerConstraint.HOLIDAY;
            default -> TypeOfSchedulerConstraint.WORK;
        };
    }
}
