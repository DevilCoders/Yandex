package ru.yandex.ci.core.db;

import java.util.Collection;
import java.util.stream.Collectors;

import com.yandex.ydb.ValueProtos;

import yandex.cloud.repository.kikimr.yql.YqlType;
import ru.yandex.lang.NonNullApi;

/**
 * The class contains methods which is missing in {@link yandex.cloud.repository.kikimr.yql.YqlUtils}
 */
@NonNullApi
public final class CiYqlUtils {

    private CiYqlUtils() {
    }


    public static <T> ValueProtos.TypedValue listTypedValue(Collection<T> values, YqlType yqlType) {
        return ValueProtos.TypedValue.newBuilder()
                .setType(ValueProtos.Type.newBuilder()
                        .setListType(ValueProtos.ListType.newBuilder()
                                .setItem(ValueProtos.Type.newBuilder()
                                        .setTypeId(yqlType.getYqlTypeBuilder().getTypeId())
                                )
                        )
                )
                .setValue(ValueProtos.Value.newBuilder()
                        .addAllItems(values.stream()
                                .map(it -> yqlType.toYql(it).build())
                                .collect(Collectors.toList())
                        )
                )
                .build();
    }

    /**
     * The method is created cause `YqlUtils.value(String.class, offsetLaunchNumber))`
     * doesn't work with `@Column(dbType = DbType.UTF8)`
     */
    public static ValueProtos.TypedValue utf8TypedValue(String value) {
        return ValueProtos.TypedValue.newBuilder()
                .setType(ValueProtos.Type.newBuilder()
                        .setTypeId(ValueProtos.Type.PrimitiveTypeId.UTF8)
                )
                .setValue(utf8Value(value))
                .build();
    }

    public static <T> ValueProtos.TypedValue typedValue(T value, YqlType yqlType) {
        return ValueProtos.TypedValue.newBuilder()
                .setType(ValueProtos.Type.newBuilder()
                        .setTypeId(yqlType.getYqlTypeBuilder().getTypeId()))
                .setValue(yqlType.toYql(value))
                .build();
    }

    public static ValueProtos.Value utf8Value(String it) {
        return ValueProtos.Value.newBuilder().setTextValue(it).build();
    }

    public static String escapeFieldName(String fieldName) {
        return "`" + fieldName + "`";
    }

}
