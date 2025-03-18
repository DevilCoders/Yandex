package ru.yandex.ci.client.calendar.api;

import java.util.List;

import lombok.Value;

@Value
public class Holidays {
    List<Holiday> holidays;

    public static Holidays of(Holiday... holidays) {
        return new Holidays(List.of(holidays));
    }
}
