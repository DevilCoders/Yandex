package ru.yandex.ci.client.base.http.retrofit;

import java.lang.annotation.Annotation;
import java.lang.reflect.Type;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import retrofit2.Converter;
import retrofit2.Retrofit;

public class QueryValueEnumConverterFactory extends Converter.Factory {
    @Nullable
    @Override
    public Converter<?, String> stringConverter(
            @Nonnull Type type,
            @Nonnull Annotation[] annotations,
            @Nonnull Retrofit retrofit
    ) {
        if (type instanceof Class && ((Class<?>) type).isEnum()) {
            return value -> serialize((Enum<?>) value);
        }
        return null;
    }

    private static String serialize(Enum<?> value) {
        try {
            var annotation = value.getDeclaringClass().getField(value.name()).getAnnotation(QueryValue.class);
            if (annotation == null) {
                return value.name();
            }

            return annotation.value();
        } catch (NoSuchFieldException e) {
            throw new RuntimeException(e);
        }
    }
}
