package ru.yandex.ci.client.calendar;

import java.time.LocalDate;

import ru.yandex.ci.client.calendar.api.Country;
import ru.yandex.ci.client.calendar.api.Holidays;

public interface CalendarClient {
    Holidays getHolidays(LocalDate from, LocalDate to, Country country);
}
