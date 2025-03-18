package ru.yandex.ci.engine.launch.auto;

import java.time.DayOfWeek;
import java.time.Instant;
import java.time.ZonedDateTime;
import java.util.Set;
import java.util.TreeSet;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.config.a.model.auto.DayType;
import ru.yandex.ci.core.config.a.model.auto.Schedule;
import ru.yandex.ci.flow.engine.definition.common.TypeOfSchedulerConstraint;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProvider;
import ru.yandex.ci.flow.engine.runtime.calendar.WorkCalendarProviderException;

@RequiredArgsConstructor
public class ScheduleCalculator {
    private final WorkCalendarProvider calendarProvider;
    private static final int MAX_DAY_CHECK = 60;

    public boolean contains(Schedule schedule, Instant point) {
        var dateTime = point.atZone(schedule.getTimezone());
        if (schedule.getDays() != null && !schedule.getDays().contains(dateTime.getDayOfWeek())) {
            return false;
        }
        var localTime = dateTime.toLocalTime();
        var start = schedule.getTime().getStart();
        var end = schedule.getTime().getEnd();
        boolean inTimeWindow = (localTime.isAfter(start) || localTime.equals(start))
                && (localTime.isBefore(end) || localTime.equals(end));

        if (!inTimeWindow) {
            return false;
        }

        return dayTypeMatches(dateTime, schedule.getDayType());
    }

    private boolean dayTypeMatches(ZonedDateTime dateTime, @Nullable DayType permitted) {
        if (permitted == null) {
            return true;
        }

        TypeOfSchedulerConstraint currentDayType;
        try {
            currentDayType = calendarProvider.getTypeOfDay(dateTime.toLocalDate());
        } catch (WorkCalendarProviderException e) {
            throw new RuntimeException(e);
        }
        return switch (permitted) {
            case HOLIDAYS -> currentDayType == TypeOfSchedulerConstraint.HOLIDAY;
            case WORKDAYS -> currentDayType == TypeOfSchedulerConstraint.WORK
                    || currentDayType == TypeOfSchedulerConstraint.PRE_HOLIDAY;
            case NOT_PRE_HOLIDAYS -> currentDayType == TypeOfSchedulerConstraint.WORK;
        };
    }

    public ZonedDateTime nextWindow(Schedule schedule, Instant point) {
        var start = schedule.getTime().getStart();
        var dateTime = point.atZone(schedule.getTimezone());
        if (contains(schedule, point)) {
            return dateTime;
        }

        int tryNum = MAX_DAY_CHECK;
        while (tryNum-- > 0) {
            if (schedule.getDaysOrAll().contains(dateTime.getDayOfWeek()) && !dateTime.toLocalTime().isAfter(start)) {
                dateTime = dateTime.with(start);
            } else {
                var nextDayOfWeek = getNextDayOfWeek(schedule.getDaysOrAll(), dateTime.getDayOfWeek());
                var dayDifference = nextDayOfWeek.getValue() - dateTime.getDayOfWeek().getValue();
                if (dayDifference <= 0) {
                    dayDifference += 7;
                }

                dateTime = dateTime.plusDays(dayDifference).with(start);
            }

            if (dayTypeMatches(dateTime, schedule.getDayType())) {
                return dateTime;
            }
            dateTime = dateTime.plusDays(1);
        }

        // например, в конфиге задана пятница, но не перед выходными. Такая ситуация возможна,
        // если суббота стала рабочей. И любые другие переносы могу приводить к валидным ситуациями,
        // но высчитывать когда они случатся бесконечно долго мы не можем.
        throw new RuntimeException("couldn't find next day to schedule for " + schedule);
    }

    private DayOfWeek getNextDayOfWeek(Set<DayOfWeek> available, DayOfWeek current) {
        var sortedDays = new TreeSet<>(available);
        var next = sortedDays.higher(current);
        if (next != null) {
            return next;
        }
        return sortedDays.first();
    }
}
