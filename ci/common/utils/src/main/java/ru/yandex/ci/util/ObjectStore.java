package ru.yandex.ci.util;

import lombok.AllArgsConstructor;

@AllArgsConstructor
public class ObjectStore<T> {
    private T object;

    public ObjectStore() {
    }

    public static <T> ObjectStore<T> of(T obj) {
        return new ObjectStore<T>(obj);
    }

    public T get() {
        return this.object;
    }

    public T set(T value) {
        this.object = value;
        return value;
    }
}
