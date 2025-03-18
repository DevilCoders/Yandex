package ru.yandex.se.logsng.tool.adapters.dsl.base;

import ru.yandex.se.logsng.tool.adapters.dsl.types.Event;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;

import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by astelmak on 13.07.16.
 */
public class ByFieldAggregator implements EventAggregatorStrategy {
    private final HashMap<Event, RequiredType> events = new HashMap<>();

    @Override
    public void put(Event ev, RequiredType type) {
        events.put(ev, type);
    }

    @Override
    public void putAll(Map<Event, RequiredType> map) {
        events.putAll(map);
    }

    @Override
    public Map<Event, RequiredType> getEvents() {
        return Collections.unmodifiableMap(events);
    }

    public static boolean isFieldCompatible(Field f1, Field f2) {
        return f1.getType().equals(f2.getType()) && (f1.isOptional() || !f2.isOptional());
    }

    public void putEventsWithCompatibleField(Field field, String xsdFieldName, Collection<Event> events) {
        for (Event ev : events) {
            Field f = ev.getFieldByXSDName(xsdFieldName);

            put(ev, f != null && isFieldCompatible(field, f) ? EventAggregator.RequiredType.OPTIONAL :
                    EventAggregator.RequiredType.EXCLUDED);
        }
    }
}
