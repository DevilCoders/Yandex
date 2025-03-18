package ru.yandex.ci.core.config;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;

import javax.annotation.Nonnull;

public class ConfigUtils {

    private ConfigUtils() {
    }

    public static <T extends ConfigIdEntry<T>> ArrayList<T> toList(LinkedHashMap<String, T> entitiesMap) {
        ArrayList<T> list = new ArrayList<>(entitiesMap.size());
        for (var entry : entitiesMap.entrySet()) {
            list.add(entry.getValue().withId(entry.getKey()));
        }
        return list;
    }

    public static <T extends ConfigIdEntry<T>> LinkedHashMap<String, T> toMap(List<T> entities) {
        LinkedHashMap<String, T> map = new LinkedHashMap<>(entities.size());
        for (T entity : entities) {
            map.put(entity.getId(), entity);
        }
        return map;
    }

    public static <T extends ConfigIdEntry<T>> LinkedHashMap<String, T> update(@Nonnull LinkedHashMap<String, T> map) {
        for (var e : map.entrySet()) {
            e.setValue(e.getValue().withId(e.getKey()));
        }
        return map;
    }
}
