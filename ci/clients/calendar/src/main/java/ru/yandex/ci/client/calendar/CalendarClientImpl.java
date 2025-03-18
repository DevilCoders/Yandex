package ru.yandex.ci.client.calendar;

import java.time.LocalDate;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.PropertyNamingStrategies;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;
import ru.yandex.ci.client.calendar.api.CalendarApi;
import ru.yandex.ci.client.calendar.api.Country;
import ru.yandex.ci.client.calendar.api.Holidays;
import ru.yandex.ci.client.calendar.api.OutMode;

public class CalendarClientImpl implements CalendarClient {
    private final CalendarApi api;

    private CalendarClientImpl(HttpClientProperties httpClientProperties) {
        var objectMapper = RetrofitClient.Builder.defaultObjectMapper()
                .enable(DeserializationFeature.READ_UNKNOWN_ENUM_VALUES_USING_DEFAULT_VALUE)
                .setPropertyNamingStrategy(PropertyNamingStrategies.LOWER_CAMEL_CASE)
                .setSerializationInclusion(JsonInclude.Include.NON_NULL);

        this.api = RetrofitClient.builder(httpClientProperties, getClass())
                .objectMapper(objectMapper)
                .build(CalendarApi.class);
    }

    public static CalendarClientImpl create(HttpClientProperties httpClientProperties) {
        return new CalendarClientImpl(httpClientProperties);
    }

    @Override
    public Holidays getHolidays(LocalDate from, LocalDate to, Country country) {
        return api.getHolidays(from, to, country, 1, OutMode.ALL);
    }
}
