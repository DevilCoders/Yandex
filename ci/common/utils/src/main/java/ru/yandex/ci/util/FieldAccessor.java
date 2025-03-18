package ru.yandex.ci.util;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;

import lombok.NonNull;
import lombok.RequiredArgsConstructor;

@RequiredArgsConstructor
public class FieldAccessor<T> {
    @NonNull
    private final MethodHandle method;

    public MethodHandle handler(T object) {
        return method.bindTo(object);
    }

    public static <T, F> FieldAccessor<T> of(Class<T> clazz, String field) {
        try {
            var actualField = clazz.getDeclaredField(field);
            actualField.setAccessible(true);
            return new FieldAccessor<>(MethodHandles.lookup().unreflectGetter(actualField));
        } catch (NoSuchFieldException | IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }
}
