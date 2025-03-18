package ru.yandex.ci.client.calendar.api;

import java.time.LocalDate;

import javax.annotation.Nullable;

import retrofit2.http.GET;
import retrofit2.http.Query;

// https://wiki.yandex-team.ru/calendar/api/new-web
public interface CalendarApi {
    @GET("get-holidays")
    Holidays getHolidays(@Query("from") LocalDate from,
                         @Query("to") LocalDate to,
                         @Query("for") Country country,
                         @Query("for_yandex") int forYandex,
                         @Nullable @Query("outMode") OutMode mode);
}
