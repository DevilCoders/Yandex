package ru.yandex.se.logsng.tool.adapters.dsl.base;

import ru.yandex.se.logsng.tool.adapters.dsl.types.Event;

import java.util.*;

/**
 * Created by astelmak on 12.07.16.
 */
public class EventAggregator implements EventAggregatorStrategy {
    private final TreeMap<Event, RequiredType> events = new TreeMap<>();

    public void put(Event event, RequiredType type) {
        RequiredType t = events.putIfAbsent(event, type);
        if (t != null) {
            switch (t) {
                case REQUIRED:
                    if (type == RequiredType.EXCLUDED)
                        throw new IllegalStateException("Type '" + event + "' required.");
                    break;
                case OPTIONAL:
                    switch (type) {
                        case EXCLUDED:
                        case REQUIRED:
                            events.put(event, type);
                            break;
                    }
                    break;
                case EXCLUDED:
                    if (type == RequiredType.REQUIRED)
                        throw new IllegalStateException("Type '" + event + "' required.");
                    break;
            }
        }
    }

    @Override
    public void putAll(Map<Event, RequiredType> map) {
        for (Map.Entry<Event, RequiredType> en : map.entrySet())
            put(en.getKey(), en.getValue());
    }

    @Override
    public Map<Event, RequiredType> getEvents() {
        return Collections.unmodifiableMap(events);
    }
}
