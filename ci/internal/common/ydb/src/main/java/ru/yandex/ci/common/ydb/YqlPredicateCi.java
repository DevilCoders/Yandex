package ru.yandex.ci.common.ydb;

import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;

import yandex.cloud.binding.schema.Schema.JavaField;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.EntitySchema;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlPredicateParam;

import ru.yandex.ci.common.ydb.YqlPredicateCi.InPredicate.InType;

import static java.lang.String.format;

public class YqlPredicateCi {

    private YqlPredicateCi() {
        //
    }

    /*
    Метод `in` и `notIn` предназначены в первую очередь для выборки полей с использованием индекса.
    Их можно использовать и для выборки по списку полей, но только в том случае, если это простые поля (не объекты).

    Нормально поддерживается выборка по строкам, enum-ам и целым числам.
     */

    @SafeVarargs
    public static <T> YqlPredicate in(String field, T... values) {
        return new InPredicate<>(field, InType.IN, List.of(values));
    }

    public static <T> YqlPredicate in(String field, Collection<T> values) {
        return new InPredicate<>(field, InType.IN, values);
    }

    @SafeVarargs
    public static <T> YqlPredicate notIn(String field, T... values) {
        return new InPredicate<>(field, InType.NOT_IN, List.of(values));
    }

    public static <T> YqlPredicate notIn(String field, Collection<T> values) {
        return new InPredicate<>(field, InType.NOT_IN, values);
    }

    public static <T1, T2> YqlPredicate gt(String field0, String field1, T1 value1, T2 value2) {
        return YqlPredicate.or(
                YqlPredicate.where(field0).gt(value1),
                YqlPredicate.where(field0).eq(value1).and(field1).gt(value2)
        );
    }

    @AllArgsConstructor
    static final class InPredicate<V> extends YqlPredicate {
        @Nonnull
        private final String fieldPath;
        @Nonnull
        private final InType inType;
        @Nonnull
        private final Collection<V> values;

        @Override
        public Stream<YqlPredicateParam<?>> paramStream() {
            return paramList().stream();
        }

        @Override
        public List<YqlPredicateParam<?>> paramList() {
            return values.stream()
                    .map(value -> YqlPredicateParam.of(fieldPath, value))
                    .collect(Collectors.toList());
        }

        @Override
        public <T extends Entity<T>> String toYql(@Nonnull EntitySchema<T> schema) {
            if (isEmpty()) {
                return alwaysFalse().toYql(schema);
            }

            JavaField field = schema.getField(fieldPath);
            Preconditions.checkArgument(field.isFlat(), "Only flat fields are supported for IN/NOT IN queries");

            var in = values.stream()
                    .map(value -> "?")
                    .collect(Collectors.joining(","));

            return format("`%s` %s [%s]", field.getName(), inType.yql, in);
        }

        @Override
        public YqlPredicate negate() {
            return switch (inType) {
                case IN -> new InPredicate<>(fieldPath, InType.NOT_IN, values);
                case NOT_IN -> new InPredicate<>(fieldPath, InType.IN, values);
            };
        }

        private boolean isEmpty() {
            return values.isEmpty();
        }

        @Override
        public String toString() {
            return format("%s %s (%s)", fieldPath, inType, values);
        }

        enum InType {
            IN("IN"),
            NOT_IN("NOT IN");

            private final String yql;

            InType(String yql) {
                this.yql = yql;
            }
        }
    }
}
