package ru.yandex.ci.ydb;

import java.time.Instant;
import java.util.function.Function;

import javax.annotation.Nullable;
import javax.annotation.ParametersAreNonnullByDefault;

import com.yandex.ydb.table.values.OptionalType;
import com.yandex.ydb.table.values.OptionalValue;
import com.yandex.ydb.table.values.PrimitiveType;
import com.yandex.ydb.table.values.PrimitiveValue;
import com.yandex.ydb.table.values.Type;
import com.yandex.ydb.table.values.Value;

import ru.yandex.ci.util.gson.CiGson;

@ParametersAreNonnullByDefault
public class ComplexValue {
    private ComplexValue() {
    }

    public static OptionalValue optionalInt32(@Nullable Integer value) {
        return optionalValue(value, PrimitiveType.int32(), PrimitiveValue::int32);
    }

    public static OptionalValue optionalInt64(@Nullable Long value) {
        return optionalValue(value, PrimitiveType.int64(), PrimitiveValue::int64);
    }

    public static OptionalValue optionalUtf8(@Nullable String value) {
        return optionalValue(value, PrimitiveType.utf8(), PrimitiveValue::utf8);
    }

    public static OptionalValue optionalJson(@Nullable String value) {
        return optionalValue(value, PrimitiveType.json(), PrimitiveValue::json);
    }

    public static OptionalValue optionalString(@Nullable byte[] value) {
        return optionalValue(value, PrimitiveType.string(), PrimitiveValue::string);
    }

    public static OptionalValue optionalBool(@Nullable Boolean value) {
        return optionalValue(value, PrimitiveType.bool(), PrimitiveValue::bool);
    }

    public static OptionalValue optionalTimestamp(@Nullable Instant value) {
        return optionalValue(value, PrimitiveType.timestamp(), PrimitiveValue::timestamp);
    }

    private static <T> OptionalValue optionalValue(@Nullable T value, Type type, Function<T, Value<?>> producer) {
        OptionalType optionalType = OptionalType.of(type);
        if (value == null) {
            return optionalType.emptyValue();
        }
        return optionalType.newValue(producer.apply(value));
    }

    /**
     * Wrap given object as primitive Gson value
     * use YdbParams.putObject
     *
     * @param object object
     * @return primitive gson value
     */
    public static PrimitiveValue toJson(@Nullable Object object) {
        return PrimitiveValue.json(CiGson.instance().toJson(object));
    }
}
