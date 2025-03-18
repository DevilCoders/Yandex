package ru.yandex.ci.client.arcanum;

import java.time.LocalTime;
import java.time.format.DateTimeFormatter;

import lombok.AllArgsConstructor;
import lombok.Value;

@Value
@AllArgsConstructor
public class MergeIntervalDto {
    private static DateTimeFormatter format = DateTimeFormatter.ofPattern("HH:mm");
    String from;
    String to;

    public MergeIntervalDto(LocalTime from, LocalTime to) {
        this.from = from.format(format);
        this.to = to.format(format);
    }
}
