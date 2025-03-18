package ru.yandex.se.logsng.tool.adapters.dsl.base;

import ru.yandex.se.logsng.tool.adapters.dsl.types.Event;

import java.util.Map;

/**
 * Created by astelmak on 13.07.16.
 */
public interface EventAggregatorStrategy {
    enum RequiredType {
        OPTIONAL,
        REQUIRED,
        EXCLUDED
    }

    void put(Event ev, RequiredType type);

    void putAll(Map<Event, RequiredType> map);

    Map<Event, RequiredType> getEvents();
}
