package ru.yandex.ci.client.juggler.model;

import java.util.List;

import lombok.Value;

@Value
public class RawEventsResponse {
    int acceptedEvents;
    boolean success;
    List<Event> events;

    @Value
    public static class Event {
        int code;
    }
}
