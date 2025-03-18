package ru.yandex.ci.client.juggler.model;

import java.util.List;

import lombok.Value;

@Value
public class RawEvents {
    List<RawEvent> events;

    public static RawEvents of(RawEvent... events) {
        return of(List.of(events));
    }

    public static RawEvents of(List<RawEvent> events) {
        return new RawEvents(events);
    }
}
