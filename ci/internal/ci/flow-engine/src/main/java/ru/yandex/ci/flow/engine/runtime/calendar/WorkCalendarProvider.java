package ru.yandex.ci.flow.engine.runtime.calendar;

import java.time.LocalDate;

import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;

public interface WorkCalendarProvider {
    TypeOfSchedulerConstraint getTypeOfDay(LocalDate day) throws WorkCalendarProviderException;
}
